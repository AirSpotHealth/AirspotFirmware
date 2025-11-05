#ifndef __MBATTERY_H__
#define __MBATTERY_H__

#include "stdint.h"

#include "nrf_gpio.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_gpiote.h"
#include "ttask.h"
#include "log.h"
#include "custom_board.h"
#include "button.h"

#include "user.h"

#define USB_IN()      nrf_gpio_pin_read(PIN_USB_IN)
#define IS_CHARGING() !nrf_gpio_pin_read(PIN_CHRG_STA)

extern void battery_level_update(uint8_t  battery_level);

/**
 * @brief 需要在转换后才能获得不为0的电量
 *
 * @return uint8_t 电量百分比 0~100
 */
uint8_t get_battery_level(void);

/**返回电池电压 */
uint16_t get_battery_voltage_mv(void);

/**
 * @brief 更新电池电量
 *
 */
void battery_state_update(void);

/**
 * @brief 初始化ADC
 *
 */
void battery_init(void);


#endif
