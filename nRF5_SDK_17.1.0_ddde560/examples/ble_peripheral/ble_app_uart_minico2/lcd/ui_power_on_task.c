
#include "app_timer.h"
#include "battery.h"
#include "co2.h"
#include "ttask.h"
#include "ui.h"

APP_TIMER_DEF(power_on_screen_timer);
uint8_t power_on_screen_time_sec = 0;

extern void button_stop(void);
extern void sleep_device(void);

void power_on_screen_timer_handler(void *pcontext)
{
    power_on_screen_time_sec += 1;
    print("power on screen tick %d sec\n", power_on_screen_time_sec);

    if (power_on_screen_time_sec >= 10)
    {
        print("power on screen timeout\n");
        EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);
    }
}

TaskDefine(task_power_on)
{
    static uint8_t button_held_time = 0;

    TTS
    {

        // wait for the
        // First check if the button is still held
        // if then if it is held for more than 250ms then go to sleep mode and exit
        // if it is not held then continue with the normal power on sequence

        if (nrf_gpio_pin_read(BUTTON_0) == 0)
        {
            button_held_time = 0;
            while (button_held_time <= 25)
            {
                TaskDelay(10 / TICK_RATE_MS);
                button_held_time += 1;
                print("Button held for %d\n", button_held_time);

                if (nrf_gpio_pin_read(BUTTON_0) == 1)
                {
                    print("Button released\n");
                    button_held_time = 0;
                    break;
                }
            }

            if (button_held_time >= 25)
            {
                print("Button held for more than 250ms, waiting for release\n");

                // wait for the button to be released
                while (nrf_gpio_pin_read(BUTTON_0) == 0)
                {
                    TaskDelay(10 / TICK_RATE_MS);
                }

                // Enter sleep mode immediately
                sleep_device();

                TaskExit();
            }
        }

        ui_set_disp_page(UI_PAGE_POWER_ON);
        print("st7301_init\n");
        lcd_gpio_init();
        LCD_PWR_ON();
        LCD_RESET_LOW();
        TaskDelay(20 / TICK_RATE_MS);
        LCD_RESET_HIGH();
        TaskDelay(120 / TICK_RATE_MS);
        TaskWait(spi_get_usage() == SPI_NOT_USE, TICK_MAX);

        print("st7301_init done\n");
        spi_config(SPI_LCD);
        st7301_config();
        spi_config(SPI_NOT_USE);

        app_timer_create(&power_on_screen_timer, APP_TIMER_MODE_REPEATED, power_on_screen_timer_handler);
        app_timer_start(power_on_screen_timer, APP_TIMER_TICKS(1000), NULL);

        print("draw power on screen\n");

        ui_draw_bkg();
        ui_draw_power_on();

        LCD_REFRESH(&rect_full_screen);
        LCD_RELEASE();
        ui_rect_t rect = {
            .y = LCD_HIGHT - 1,
            .h = 1,
        };
        LCD_REFRESH(&rect);
        LCD_RELEASE();
        LCD_BL_ON();

        print("Wait for event\n");

        TaskWait(EventGroupCheckBits(event_group_system, EVT_BTN_PRESS_2S | EVT_UI_OFF_SCREEN), TICK_MAX);

        if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS_2S))
        {
            print("power on screen: button pressed for 2 seconds, reset\n");
            EventGroupClearBits(event_group_system, EVT_BTN_PRESS_2S);
            button_stop();
            app_timer_stop(power_on_screen_timer);
            power_on_screen_time_sec = 0;
            NVIC_SystemReset();
            TaskExit();
        }

        if (EventGroupCheckBits(event_group_system, EVT_UI_OFF_SCREEN))
        {
            EventGroupClearBits(event_group_system, EVT_UI_OFF_SCREEN);
            button_stop();
            app_timer_stop(power_on_screen_timer);
            power_on_screen_time_sec = 0;

            print("power on screen timeout: go to sleep\n");
            LCD_BL_OFF();
            ui_draw_off_screen();
            LCD_REFRESH(&rect_full_screen);
            LCD_RELEASE();
            LCD_RESET_LOW();
            TaskDelay(20 / TICK_RATE_MS);
            LCD_RESET_HIGH();
            TaskDelay(120 / TICK_RATE_MS);
            LCD_PWR_OFF();
            lcd_gpio_uninit();

            sleep_device();
            TaskExit();
        }
    }
}
}
