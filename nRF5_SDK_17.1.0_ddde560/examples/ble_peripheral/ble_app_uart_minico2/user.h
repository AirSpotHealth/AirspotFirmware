#ifndef __USER_H__
#define __USER_H__

#include "app_pwm.h"
#include "cfg_fstorage.h"
#include "custom_board.h"
#include "flash_spi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "spi.h"
#include "st7301.h"
#include "stdint.h"
#include "string.h"
#include "ttask.h"
#include "ui.h"

// Macros for controlling buzzer and vibrator
#define BUZZER_ON()    pwm_ch0_on()
#define BUZZER_OFF()   pwm_ch0_off()
#define VIBRATOR_ON()  nrf_gpio_pin_set(PIN_VIBRATOR)
#define VIBRATOR_OFF() nrf_gpio_pin_clear(PIN_VIBRATOR)

// Error codes
#define ERROR_CODE_SENSOR_READ             0x01
#define ERROR_CODE_SENSOR_AUTO_CALIB       0x02
#define ERROR_CODE_SENSOR_RESET            0x03
#define ERROR_CODE_SENSOR_MEASURMENT_START 0x04
#define ERROR_CODE_SENSOR_DATA_STATUS      0x05
#define ERROR_CODE_SENSOR_VARIANT          0x06
#define ERROR_CODE_SENSOR_MEASURMENT_STOP  0x07
#define ERROR_CODE_INVALID_CO2_READING     0x08
#define ERROR_CODE_SENSOR_I2C_COMM_FAILURE 0x09
#define ERROR_CODE_SENSOR_CRC_FAILURE      0x0A

// Reset reason code
#define RESET_REASON_WAKEUP_FROM_OFF 0xA5

// Function declarations
void     uninit_spi(void);
void     set_buzzer_state(uint8_t on_off);
uint8_t  get_buzzer_state(void);
void     set_vibrator_state(uint8_t on_off);
uint8_t  get_vibrator_state(void);
void     set_timebase(uint32_t time);
void     prepare_edit_hour(void);
void     inc_hour(void);
void     prepare_edit_minute(void);
void     inc_minute(void);
void     inc_second(void);
uint32_t get_time_now(void);
uint8_t  get_time_now_hour24(void);
uint8_t  get_time_now_minute(void);
void     set_ble_disconnect(void);
char    *get_device_name(void);
void     set_device_name(char *name);
void     user_init(void);
void     pwm_init(void);
void     pwm_uninit(void);
void     pwm_ch0_on(void);
void     pwm_ch0_off(void);
void     pwm_ch1_on(void);
void     pwm_ch1_off(void);
void     power_off(void);
bool     is_dnd_time(void);
void     set_time_set(bool value);
bool     get_time_set(void);
void     set_asc_check_duration(uint16_t duration);
void     update_flight_mode_activation_time(void);

#endif // __USER_H__