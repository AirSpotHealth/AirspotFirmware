#ifndef __ST7301_H__
#define __ST7301_H__

#include "spi.h"
#include "stdint.h"
#include "string.h"

#include "app_timer.h"
#include "log.h"
#include "nrf_delay.h"

#define ST7301_XS 0
#define ST7301_XE 0x3B
#define ST7301_YS 0
#define ST7301_YE 0xEF

#define COMMAND   0
#define PARAMETER 1

#define LCD_WIDTH 176
#define LCD_HIGHT 176

#define ST7301_COLOR(color) ((color) << 2)

#define LCD_CHIP_SEL()   nrf_gpio_pin_clear(PIN_LCD_CS);
#define LCD_CHIP_UNSEL() nrf_gpio_pin_set(PIN_LCD_CS);
#define LCD_RESET_LOW()  nrf_gpio_pin_clear(PIN_LCD_RST);
#define LCD_RESET_HIGH() nrf_gpio_pin_set(PIN_LCD_RST);
#define LCD_BL_ON()      nrf_gpio_pin_clear(PIN_LCD_BL);
#define LCD_BL_OFF()     nrf_gpio_pin_set(PIN_LCD_BL);
#define LCD_PWR_ON()     nrf_gpio_pin_clear(PIN_LCD_PWR);
#define LCD_PWR_OFF()    nrf_gpio_pin_set(PIN_LCD_PWR);
#define LCD_SEL_CMD()    nrf_gpio_pin_clear(PIN_LCD_CD);
#define LCD_SEL_PARM()   nrf_gpio_pin_set(PIN_LCD_CD);

#define COL_OFFSET 8
// #define ROW_OFFSET 0
#define ROW_OFFSET 64

#define ST7301_RESET_LOW()  LCD_RESET_LOW()
#define ST7301_RESET_HIGH() LCD_RESET_HIGH()
#define ST7301_BL_ON()      LCD_BL_ON()
#define ST7301_BL_OFF()     LCD_BL_OFF()
#define ST7301_PWR_ON()     LCD_PWR_ON()
#define ST7301_PWR_OFF()    LCD_PWR_OFF()
#define ST7301_SEL_CMD()    LCD_SEL_CMD()
#define ST7301_SEL_PARM()   LCD_SEL_PARM()

void lcd_gpio_init(void);
void lcd_gpio_uninit(void);

void st7301_config(void);

void st7301_init(void);

/**
 * @brief Set the window addr
 *
 * @param x 起始x
 * @param y 起始y
 * @param w 宽度
 * @param h 高度
 * @return uint8_t
 */
uint8_t set_window_addr(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

void lcd_low_power(void);

#endif
