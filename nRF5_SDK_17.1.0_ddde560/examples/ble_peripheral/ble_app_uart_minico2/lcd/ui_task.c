#include "app_timer.h"
#include "battery.h"
#include "co2.h"
#include "ttask.h"
#include "ui.h"
#include "user.h"

extern void    ble_delete_bonds(void);
extern void    advertising_start(void);
extern void    advertising_stop(void);
extern uint8_t get_ble_state(void);
extern void    reset_power_mode(void);
extern void    co2_start_calibration(void);
extern int16_t get_calibration_correction(void);
extern int16_t get_cc_value(void);
extern void    change_timer_power_mode(bool is_sleep);

extern uint8_t ttask_slow_mode;

// 要闪烁的图标
static ui_blink_t ui_blink_slc = UI_BLINK_NONE;
// 闪烁项目的显示状态
static ui_bool_t ui_blink_state = UI_BOOL_TRUE;
// 闪烁图标定时器, 重复模式@500ms
APP_TIMER_DEF(ui_blink_timer);
// 闪烁图标超时计时,5秒
static uint16_t ui_blink_timeout_ms = 0;
void            ui_blink_timer_handler(void *pcontext);

// 息屏定时器
APP_TIMER_DEF(off_screen_timer);
void            off_screen_timer_handler(void *pcontext);
static uint16_t screen_on_time_sec = 0;
void            reset_off_screen_timer(void)
{
    print("restart off screen timer\n");
    app_timer_stop(off_screen_timer);
    app_timer_start(off_screen_timer, APP_TIMER_TICKS(1000), NULL);
    screen_on_time_sec = 0;
}

// passkey
static uint8_t  passkey[6];
static uint16_t cali_time_sec    = 0;
static int16_t  correction_value = 0;
static int16_t  cc_new           = 0;
/**设置屏幕常亮 */
void set_screen_const_on(ui_bool_t state)
{
    // screen_const_on = state;
    if (state == UI_BOOL_TRUE)
    {
        // print("set screen constan on\n");
        EventGroupSetBits(event_group_system, EVT_SCREEN_FORCE_ON);
    }
    else
    {
        // print("chancel screen constan on\n");
        EventGroupClearBits(event_group_system, EVT_SCREEN_FORCE_ON);
        EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);
    }
}

/**获取屏幕常亮状态 */
ui_bool_t get_screen_const_on_state(void)
{
    // print("screen constan on %d\n", EventGroupGetBits(event_group_system, EVT_SCREEN_FORCE_ON));
    if (EventGroupGetBits(event_group_system, EVT_SCREEN_FORCE_ON))
    {
        return UI_BOOL_TRUE;
    }
    else
    {
        return UI_BOOL_FALSE;
    }
}

void ui_draw_off_screen(void)
{
    ui_set_disp_page(UI_PAGE_OFF);
    ui_draw_bkg();
}

void ui_draw_main_page(void)
{

    // 内存画布绘制主页
    ui_set_disp_page(UI_PAGE_MAIN);
    ui_draw_bkg();

    if (EventGroupCheckBits(event_group_system, EVT_CO2_SENSOR_ERROR))
    {
        ui_set_disp_page(UI_PAGE_SENSOR_ERROR);
        ui_draw_sensor_error();
        return;
    }

    // Draw common elements regardless of view mode
    uint8_t hour_now = get_time_now_hour24();
    uint8_t min_now  = get_time_now_minute();

    ui_bool_t alarm_state    = (ui_bool_t)get_buzzer_state();
    ui_bool_t vibrator_state = (ui_bool_t)get_vibrator_state();
    ui_bool_t ble_state      = (ui_bool_t)get_ble_state();
    ui_bool_t dnd_mode       = (ui_bool_t)cfg_fstorage_get_dnd_mode();
    ui_bool_t dnd_time       = is_dnd_time();
    uint8_t   ui_mode        = cfg_fstorage_get_ui_mode();
    uint16_t  co2_value_now  = get_co2_value();
    if (co2_value_now < 400 && co2_value_now > 0)
    {
        co2_value_now = 400;
    }

    if (EventGroupCheckBits(event_group_system, EVT_TOGGLE_UI_MODE))
    {
        EventGroupClearBits(event_group_system, EVT_TOGGLE_UI_MODE);
        update_elements_position(ui_mode);
    }

    // Draw status icons
    if (dnd_time)
    {
        ui_draw_dnd(dnd_time);
    }
    else
    {
        if (ui_blink_slc == UI_BLINK_ALARM)
            ui_draw_buzzer(alarm_state, ui_blink_state);
        else
            ui_draw_buzzer(alarm_state, UI_BOOL_TRUE);

        if (ui_blink_slc == UI_BLINK_VIBRATOR)
            ui_draw_vibrator(vibrator_state, ui_blink_state);
        else
            ui_draw_vibrator(vibrator_state, UI_BOOL_TRUE);
    }

    if (dnd_mode && !dnd_time)
    {
        ui_draw_dnd(dnd_time);
    }

    if (ui_blink_slc == UI_BLINK_HOUR)
        ui_draw_hour(hour_now, ui_blink_state);
    else
        ui_draw_hour(get_time_set() ? hour_now : 99, UI_BOOL_TRUE);

    if (ui_blink_slc == UI_BLINK_MINUTE)
        ui_draw_minute(min_now, ui_blink_state);
    else
        ui_draw_minute(get_time_set() ? min_now : 99, UI_BOOL_TRUE);

    // Draw normal view elements
    if (ui_blink_slc == UI_BLINK_CO2_VALUE)
        ui_draw_co2_ppm(co2_value_now, ui_blink_state);
    else
        ui_draw_co2_ppm(co2_value_now, UI_BOOL_TRUE);

    if (ui_blink_slc == UI_BLINK_CO2_FREQUENCY)
        ui_draw_co2_frequency(get_power_mode(), ui_blink_state);
    else
        ui_draw_co2_frequency(get_power_mode(), UI_BOOL_TRUE);

    if (ui_blink_slc == UI_BLINK_CO2_PPM_LABEL)
        ui_draw_co2_ppm_label(ui_blink_state);
    else
        ui_draw_co2_ppm_label(UI_BOOL_TRUE);

    // Draw battery status
    ui_draw_battery(get_battery_level(), UI_BOOL_TRUE);

    if (ui_blink_slc == UI_BLINK_BLE)
        ui_draw_ble(ble_state, ui_blink_state);
    else
        ui_draw_ble(ble_state, UI_BOOL_TRUE);

    if (ui_mode == 0)
    {
        // ui_draw_bar_bg();
        ui_draw_bar_graph();
    }
    else if (ui_mode == 1)
    {
        if (get_co2_value() != 0)
        {
            ui_draw_indicator(get_co2_value());
        }
        ui_draw_indicator_rect();
    }

    if (cfg_fstorage_get_auto_calibrate() == 0)
    {
        ui_draw_elem_inverted(&elem_calib_off);
    }
    else
    {
        ui_draw_elem_inverted(&elem_calib_on);
    }

    if (cfg_fstorage_get_flight_mode() == 1)
    {
        ui_draw_elem(&elem_flight_mode);
    }

    if (EventGroupCheckBits(event_group_system, EVT_UI_GRAPH_UPDATE))
    {
        EventGroupClearBits(event_group_system, EVT_UI_GRAPH_UPDATE);
    }

    if (EventGroupCheckBits(event_group_system, EVT_FLIGHT_MODE_UPDATE))
    {
        EventGroupClearBits(event_group_system, EVT_FLIGHT_MODE_UPDATE);
    }
}

/**设置passkey */
void ui_set_passkey(const char *key)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        passkey[i] = key[i] - 0x30;
    }
}

TaskDefine(task_ui)
{
    TTS
    {
        EventGroupClearAllBits(event_group_system);
        EventGroupSetBits(event_group_system, EVT_SCREEN_ON_ONETIME); // 上电亮屏

        // LCD屏幕初始化
        print("st7301_init\n");
        lcd_gpio_init();
        LCD_PWR_ON();
        LCD_RESET_LOW();
        TaskDelay(20 / TICK_RATE_MS);
        LCD_RESET_HIGH();
        TaskDelay(120 / TICK_RATE_MS);
        print("spi_config wait spi not use\n");
        TaskWait(spi_get_usage() == SPI_NOT_USE, TICK_MAX);
        print("spi_config spi_lcd\n");
        spi_config(SPI_LCD);
        print("st7301_config\n");
        st7301_config();
        print("spi_config spi_not_use\n");
        spi_config(SPI_NOT_USE);

        // 创建闪烁图标定时器
        app_timer_create(&ui_blink_timer, APP_TIMER_MODE_REPEATED, ui_blink_timer_handler);
        // 创建息屏定时器
        app_timer_create(&off_screen_timer, APP_TIMER_MODE_REPEATED, off_screen_timer_handler);

        // 更新电量
        battery_state_update();

        update_elements_position(cfg_fstorage_get_ui_mode());

        while (1)
        {

        show_off_screen:

            // 息屏
            print("close screen\n");
            LCD_BL_OFF(); // 关闭背光
            ui_draw_off_screen();
            LCD_REFRESH(&rect_full_screen);
            LCD_RELEASE();
            LCD_RESET_LOW();
            TaskDelay(20 / TICK_RATE_MS);
            LCD_RESET_HIGH();
            TaskDelay(120 / TICK_RATE_MS);
            LCD_PWR_OFF();
            lcd_gpio_uninit();
            app_timer_stop(ui_blink_timer);
            app_timer_stop(off_screen_timer);

            // if it is bat low mode we need to change the timer power mode back to delay mode
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW))
            {
                change_timer_power_mode(true);
            }

            if (EventGroupCheckBits(event_group_system, EVT_UI_OFF_SCREEN))
            {
                EventGroupClearBits(event_group_system, EVT_UI_OFF_SCREEN);
            }

            if (EventGroupCheckBits(event_group_system, EVT_DEVICE_SLEEP))
            {
                sleep_device();
                break;
            }

            // 等待亮屏,按键/配对完成/强制常亮/充电常亮
            EventGroupWaitBits(event_group_system,
                               EVT_SCREEN_ON_ONETIME |
                                   EVT_BTN_PRESS |
                                   EVT_SCREEN_FORCE_ON |
                                   EVT_CHARGING |
                                   EVT_SHOW_PASSKEY |
                                   EVT_CO2_CALIB_TIMING |
                                   EVT_DEVICE_SLEEP,
                               TICK_MAX);

            if (EventGroupCheckBits(event_group_system, EVT_DEVICE_SLEEP))
            {
                EventGroupClearBits(event_group_system, EVT_DEVICE_SLEEP);
                sleep_device();
            }

            // if it is bat low mode, we need to change the timer powe mode to normal mode to avoid delay
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW))
            {
                change_timer_power_mode(false);
            }

            // LCD屏幕初始化
            lcd_gpio_init();
            LCD_PWR_ON();
            LCD_RESET_LOW();
            TaskDelay(20 / TICK_RATE_MS);
            LCD_RESET_HIGH();
            TaskDelay(120 / TICK_RATE_MS);
            TaskWait(spi_get_usage() == SPI_NOT_USE, TICK_MAX);
            spi_config(SPI_LCD);
            st7301_config();
            spi_config(SPI_NOT_USE);

            // 提示充电
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
            {
            show_low_bat:
                // Clear all event bits except for EVT_CHARGING, EVT_BAT_LOW, and EVT_TIMEBASE_UP
                EventGroupClearBits(event_group_system, event_group_system & ~(EVT_CHARGING | EVT_BAT_LOW | EVT_TIMEBASE_UP | EVT_BAT_LOW_WARNING));

                print("enter low power mode\n");

                ui_set_disp_page(UI_PAGE_BAT_LOW);
                ui_draw_bkg();
                ui_draw_battery_low();

                LCD_REFRESH(&rect_full_screen);
                LCD_RELEASE();
                LCD_BL_ON();

                print("Wait 5 seconds for LB mode");

                if (ttask_slow_mode)
                {
                    // Wait for 500ms or until charging event occurs
                    TaskWait(EventGroupCheckBits(event_group_system, EVT_CHARGING), 500 / TICK_RATE_MS);
                }
                else
                {
                    // Wait for 5 seconds or until charging event occurs
                    TaskWait(EventGroupCheckBits(event_group_system, EVT_CHARGING), 5000 / TICK_RATE_MS);
                }
                print("Done, go to off_screen");
                // EventGroupClearBits(event_group_system, EVT_SCREEN_ON_ONETIME);
                goto show_off_screen;
            }

            // 显示配对码
            if (EventGroupCheckBits(event_group_system, EVT_SHOW_PASSKEY))
            {
            show_passkey:
                EventGroupClearBits(event_group_system, EVT_SHOW_PASSKEY);
                print("pairing start\n");

                ui_set_disp_page(UI_PAGE_PASSKEY);
                ui_draw_bkg();
                ui_draw_passkey(passkey);

                LCD_REFRESH(&rect_full_screen);
                LCD_RELEASE();
                LCD_BL_ON();

                EventGroupWaitBits(event_group_system, EVT_CHANCEL_SHOW_PASSKEY, TICK_MAX);
                EventGroupClearBits(event_group_system, EVT_CHANCEL_SHOW_PASSKEY);

                print("pairing end\n");
            }

            // 运行校准任务
            if (EventGroupCheckBits(event_group_system, EVT_CO2_CALIB_TIMING))
            {
            show_calib:

                print("task recali run\n");
                proto_send_recali_start();
                ui_set_disp_page(UI_PAGE_RECALIBRATING);
                ui_draw_bkg();
                ui_draw_text(-1, 15, "CALIBRATING", true);
                ui_draw_text(-1, 150, "Calibrate  outdoors", true);
                LCD_BL_ON();

                if (get_power_mode() == PWR_MODE_LOW)
                {
                    cali_time_sec = 541U;
                }
                else
                {
                    cali_time_sec = 241U;
                }

                correction_value = 0;
                cc_new           = 0;
                while (1)
                {
                    cali_time_sec -= 1;
                    ui_draw_recalibrating_countdown(cali_time_sec);
                    proto_send_recali_countdown(cali_time_sec);
                    LCD_REFRESH(&rect_full_screen);
                    LCD_RELEASE();

                    TaskDelay(1000 / TICK_RATE_MS);
                    if (cali_time_sec == 0) break;
                }
                EventGroupClearBits(event_group_system, EVT_CO2_CALIB_TIMING);

                // reset the power mode to what it was before calibration
                // because we changed it to mid if it was on demand
                reset_power_mode();

                EventGroupSetBits(event_group_system, EVT_CO2_CALIB_DONE);

                // wait for the co2 history update to be done
                TaskWait(EventGroupCheckBits(event_group_system, EVT_CO2_UP_HIS) == 0, TICK_MAX);

                correction_value = get_calibration_correction();
                cc_new           = get_cc_value();

                // show the cc for 3 seconds
                ui_draw_bkg();
                ui_draw_text(-1, 15, "CORRECTION", true);
                ui_draw_cc_value(correction_value);

                LCD_REFRESH(&rect_full_screen);
                LCD_RELEASE();

                proto_send_recali_done(correction_value);

                // wait 3 seconds
                while (1)
                {
                    cali_time_sec += 1;
                    if (cali_time_sec == 3) break;
                    TaskDelay(1000 / TICK_RATE_MS);
                }

                TaskWait(EventGroupCheckBits(event_group_system, EVT_CO2_UP_HIS) == 0, TICK_MAX);

                // convert int16_t to uint16_t for history record
                add_history_record(cc_new, RECORD_TYPE_CALIBRATION_CORRECTION);

                // wait for the co2 update to be done if it is not done
                EventGroupWaitBits(event_group_system, EVT_UI_UP_CO2, TICK_MAX);
                EventGroupClearBits(event_group_system, EVT_CO2_CALIB_DONE);

                EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);

                EventGroupClearBits(event_group_system, EVT_UI_OFF_SCREEN);
                print("task recali end\n");
            }

            if (EventGroupCheckBits(event_group_system, EVT_SCREEN_ON_ONETIME | EVT_BTN_PRESS | EVT_CHARGING)) // 是上电或按键亮的屏幕
            {
                EventGroupClearBits(event_group_system, EVT_SCREEN_ON_ONETIME | EVT_CHARGING);

                if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS))
                {

                    EventGroupSetBits(event_group_system, EVT_CO2_UPDATE_ONCE); // 亮屏采样一次
                    EventGroupClearBits(event_group_system, EVT_UI_UP_CO2);     // 清除可能的标志,保证数值闪烁

                    // This gets called when the button is pressed when the screen is off
                    print("wake up from key press\n");
                    app_timer_stop(ui_blink_timer);
                    app_timer_start(ui_blink_timer, APP_TIMER_TICKS(300), NULL);
                    ui_blink_slc   = UI_BLINK_CO2_VALUE;
                    ui_blink_state = UI_BOOL_TRUE;
                }
            }

            ui_draw_main_page();
            LCD_REFRESH(&rect_full_screen);
            LCD_RELEASE();
            LCD_BL_ON();
            reset_off_screen_timer();

            while (1)
            {
                TaskWait(EventGroupHasEvent(event_group_system), TICK_MAX);

                if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
                {
                    print("BATTERY LOW, show low bat screen\n");
                    change_timer_power_mode(false);
                    goto show_low_bat;
                }

                if (EventGroupCheckBits(event_group_system, EVT_UI_OFF_SCREEN))
                {
                    EventGroupClearBits(event_group_system, EVT_UI_OFF_SCREEN);
                    print("goto off screen\n");
                    goto show_off_screen; // 回去息屏状态
                }

                if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS | EVT_BTN_PRESS_2T | EVT_BTN_PRESS_2S)) // 除电池更新外, 重置息屏计时
                {
                    print("reset off screen timer with key press\n");
                    reset_off_screen_timer();
                }

                if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS_2S))
                {
                    EventGroupClearBits(event_group_system, EVT_BTN_PRESS_2S);

                    if (ui_blink_slc == UI_BLINK_CO2_PPM_LABEL)
                    {
                        print("manual force calibration start\n");
                        co2_start_calibration();
                    }
                    else if (ui_blink_slc == UI_BLINK_BLE)
                    {
                        ble_delete_bonds();
                    }
                    else
                    {
                        continue; // 长按只有CO2 PPM 标签闪烁时有效
                    }
                }

                if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS_2T)) // 双击
                {
                    EventGroupClearBits(event_group_system, EVT_BTN_PRESS_2T);

                    app_timer_stop(ui_blink_timer);
                    app_timer_start(ui_blink_timer, APP_TIMER_TICKS(300), NULL);
                    ui_blink_timeout_ms = 0;
                    ui_blink_state      = UI_BOOL_FALSE;

                    ui_blink_slc += 1;
                    if (ui_blink_slc > UI_BLINK_LAST) // 循环闪烁
                    {
                        print("reset ui_blink_slc\n");
                        ui_blink_slc = UI_BLINK_FIRST;
                    }

                    if (ui_blink_slc == UI_BLINK_HOUR)
                    {
                        print("edit hour\n");
                        prepare_edit_hour();
                    }
                    else if (ui_blink_slc == UI_BLINK_MINUTE)
                    {
                        print("edit minute\n");
                        prepare_edit_minute();
                    }
                }

                if (EventGroupCheckBits(event_group_system, EVT_BTN_PRESS)) // 单击
                {
                    EventGroupClearBits(event_group_system, EVT_BTN_PRESS);

                    ui_blink_timeout_ms = 0;
                    print("ui_blink_slc=%d on press\n", ui_blink_slc);
                    // This gets called when the button is pressed when the screen is on
                    switch (ui_blink_slc)
                    {
                    default:
                    case UI_BLINK_NONE:
                        if (EventGroupCheckBits(event_group_system, EVT_CO2_SENSOR_ERROR))
                        {
                            EventGroupClearBits(event_group_system, EVT_CO2_SENSOR_ERROR);
                        }
                        else
                        {
                            app_timer_stop(ui_blink_timer);
                            app_timer_start(ui_blink_timer, APP_TIMER_TICKS(300), NULL);
                            ui_blink_slc   = UI_BLINK_CO2_VALUE;
                            ui_blink_state = UI_BOOL_TRUE;
                            EventGroupSetBits(event_group_system, EVT_CO2_UPDATE_ONCE);
                        }
                        break;
                    case UI_BLINK_ALARM:
                        set_buzzer_state(!get_buzzer_state());
                        proto_send_device_state();
                        break;
                    case UI_BLINK_VIBRATOR:
                        set_vibrator_state(!get_vibrator_state());
                        proto_send_device_state();
                        break;
                    case UI_BLINK_HOUR:
                        inc_hour();
                        proto_send_device_state();
                        EventGroupSetBits(event_group_system, EVT_TIME_UPDATE);
                        break;
                    case UI_BLINK_MINUTE:
                        inc_minute();
                        proto_send_device_state();
                        EventGroupSetBits(event_group_system, EVT_TIME_UPDATE);
                        break;
                    case UI_BLINK_CO2_FREQUENCY:
                        inc_power_mode();
                        proto_send_device_state();
                        EventGroupSetBits(event_group_system, EVT_BAT_UPDATE);
                        break;
                    case UI_BLINK_CO2_PPM_LABEL:
                        break;
                    case UI_BLINK_BLE:
                        print("toggle ble, current state %d\n", get_ble_state());
                        if (0 == get_ble_state())
                        {
                            advertising_start();
                            cfg_fstorage_set_bluetooth_enabled(1);
                        }
                        else
                        {
                            advertising_stop();
                            cfg_fstorage_set_bluetooth_enabled(0);
                        }
                        ui_blink_slc = UI_BLINK_NONE;
                        EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
                        break;
                    }
                }

                if (EventGroupCheckBits(event_group_system, EVT_UI_UPDATE))
                {
                    if (EventGroupCheckBits(event_group_system, EVT_UI_UP_CO2))
                    {
                        EventGroupClearBits(event_group_system, EVT_UI_UP_CO2);
                        if (ui_blink_slc == UI_BLINK_CO2_VALUE)
                        {
                            ui_blink_slc = UI_BLINK_NONE;
                            app_timer_stop(ui_blink_timer);
                        }
                    }

                    ui_draw_main_page();
                    LCD_REFRESH(&rect_full_screen);
                    LCD_RELEASE();

                    EventGroupClearBits(event_group_system, EVT_UI_UPDATE & ~(EVT_UI_UP_CO2 | EVT_UI_OFF_SCREEN | EVT_BAT_LOW | EVT_BAT_LOW_WARNING));
                }

                if (EventGroupCheckBits(event_group_system, EVT_SHOW_PASSKEY))
                {
                    goto show_passkey;
                }

                if (EventGroupCheckBits(event_group_system, EVT_CO2_CALIB_TIMING))
                {
                    print("goto show calib\n");
                    goto show_calib;
                }
            }
        }
    }
    TTE
}

void ui_blink_timer_handler(void *pcontext)
{
    ui_blink_timeout_ms += 300;
    // print("ui_blink_timout_ms=%d\n", ui_blink_timeout_ms);
    // 非二氧化碳值的闪烁定时5秒结束
    if (ui_blink_slc != UI_BLINK_CO2_VALUE && ui_blink_timeout_ms >= 5000 && !EventGroupCheckBits(event_group_system, EVT_BTN_DOWN))
    {
        print("ui blink timer reset ui_blink_slc\n");
        ui_blink_slc = UI_BLINK_NONE;
        app_timer_stop(ui_blink_timer);
    }
    if (ui_blink_state == UI_BOOL_TRUE)
    {
        ui_blink_state = UI_BOOL_FALSE;
    }
    else
    {
        ui_blink_state = UI_BOOL_TRUE;
    }
    EventGroupSetBits(event_group_system, EVT_UI_BLINK);
    // print("ui blink timer %d\n", ui_blink_state);
}

void off_screen_timer_handler(void *pcontext)
{

    power_mode_t pwr_mode;

    if (EventGroupCheckBits(event_group_system, EVT_SCREEN_FORCE_ON)) // 充电不息屏,常亮不息屏
    {
        return;
    }

    screen_on_time_sec += 1;
    pwr_mode = get_power_mode();
    print("off screen tick %d sec, Power mode: %d\n", screen_on_time_sec, pwr_mode);

    switch (pwr_mode)
    {
    case PWR_MODE_ON_DEMAND:
    case PWR_MODE_LOW:
        if (screen_on_time_sec >= 10)
        {
            print("low power mode off screen timeout\n");
            EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);
            app_timer_stop(off_screen_timer);
            screen_on_time_sec = 0;
        }
    case PWR_MODE_MID:
        if (screen_on_time_sec >= 10)
        {
            print("low-mid power mode off screen timeout\n");
            EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);
            app_timer_stop(off_screen_timer);
            screen_on_time_sec = 0;
        }
        break;
    case PWR_MODE_HI:
        if (screen_on_time_sec >= 60)
        {
            print("hi power mode off screen timeout\n");
            EventGroupSetBits(event_group_system, EVT_UI_OFF_SCREEN);
            app_timer_stop(off_screen_timer);
            screen_on_time_sec = 0;
        }
        break;
    default:
        app_timer_stop(off_screen_timer);
        break;
    }
}
