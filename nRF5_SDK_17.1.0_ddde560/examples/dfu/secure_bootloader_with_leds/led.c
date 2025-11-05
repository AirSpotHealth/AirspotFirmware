/*
 * @Author: tai && tai0105@163.com
 * @Date: 2023-03-16 09:37:38
 * @LastEditors: tai && tai0105@163.com
 * @LastEditTime: 2023-03-17 18:00:14
 * @FilePath: \nRF5_SDK_17.1.0_ddde560\examples\dfu\secure_bootloader_with_leds\led.c
 * @Description: 
 */
#include "led.h"

#define LED_CH_ON nrf_gpio_pin_clear
#define LED_CH_OFF nrf_gpio_pin_set
#define LED_CH_TOGGLE nrf_gpio_pin_toggle

#define LED_NUM 4

// 4个led的引脚号
static uint32_t led_pins[LED_NUM][3] = { //按顶部为1，顺时针拍
    {PIN_LED3_R, PIN_LED3_G, PIN_LED3_B},
    {PIN_LED4_R, PIN_LED4_G, PIN_LED4_B}, 
    {PIN_LED2_R, PIN_LED2_G, PIN_LED2_B},
    {PIN_LED1_R, PIN_LED1_G, PIN_LED1_B},   
};

APP_TIMER_DEF(led_app_timer);

static void led_app_timer_elapsed_handler(void* args)
{
    static uint8_t ticks = 0;
    if(++ticks < 10)
    {
        return;
    }
    else
    {
        ticks = 0;
    }

    static uint8_t led_on_index = 0;
    for(uint8_t i = 0; i < LED_NUM; i++)
    {
        LED_CH_OFF(led_pins[i][0]);
        LED_CH_OFF(led_pins[i][1]);
        LED_CH_OFF(led_pins[i][2]);
    }
    //3通道点亮白光
    LED_CH_ON(led_pins[led_on_index][0]); 
    LED_CH_ON(led_pins[led_on_index][1]);
    LED_CH_ON(led_pins[led_on_index][2]);

    if(++led_on_index == LED_NUM)
    {
        led_on_index = 0;
    }
}

void led_init(void)
{
    for(uint8_t led = 0; led < LED_NUM; led++)
    {
        for(uint8_t pin = 0; pin < 3; pin++)
        {
            nrf_gpio_cfg_output(led_pins[led][pin]);
            LED_CH_OFF(led_pins[led][pin]);
        }
    }

    app_timer_init();
    app_timer_create(&led_app_timer, APP_TIMER_MODE_REPEATED, led_app_timer_elapsed_handler);
    app_timer_start(led_app_timer, APP_TIMER_TICKS(25), NULL); // rtc 频率不对？

    // app_simple_timer_init();
    // app_simple_timer_start(APP_SIMPLE_TIMER_MODE_REPEATED, led_app_timer_elapsed_handler, (1000000 / 4 / 10), NULL);
}
