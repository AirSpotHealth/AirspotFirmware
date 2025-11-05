#ifndef __CO2_H__
#define __CO2_H__

#include "battery.h"
#include "cfg_fstorage.h" // For advanced_alarm_setting_t and its accessors
#include "custom_board.h"
#include "driver_scd4x_basic.h"
#include "log.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "nrf_strerror.h"
#include "protocol.h"
#include "ttask.h"
#include "user.h"
#include "cfg_fstorage.h" // For advanced_alarm_setting_t and its accessors
#include <stdint.h>

typedef enum
{
    PWR_MODE_ON_DEMAND = 0,
    PWR_MODE_LOW,
    PWR_MODE_MID,
    PWR_MODE_HI,
} power_mode_t;

typedef enum
{
    CO2_WARN_G = 0,
    CO2_WARN_Y,
    CO2_WARN_R,
} co2_warn_t;

typedef enum
{
    CO2_ALARM_0_800 = 0,
    CO2_ALARM_800_1000,
    CO2_ALARM_1000_1200,
    CO2_ALARM_1200_1500,
    CO2_ALARM_1500_MAX,
} co2_alarm_t;

// ASC state
// Add these declarations
typedef struct
{
    uint32_t lowest3;          // Lowest sum of 3 consecutive readings
    uint16_t reading0;         // Last reading
    uint16_t reading1;         // Second last reading
    uint16_t reading2;         // Third last reading
    uint8_t  day_count;        // Days elapsed in current ASC period
    uint16_t target;           // Target CO2 value (default 426)
    uint16_t day_record_count; // Number of records in current day
} asc_state_t;

// Calibration context structure
typedef struct
{
    power_mode_t power_mode_before_calib;
    uint8_t      calib_duration_record_count;
    uint32_t     calib_duration_sum;
    uint16_t     co2_calib_target;
} calib_ctx_t;

// CO2 context structure
typedef struct
{
    float        tmp;
    float        humi;
    scd4x_bool_t drdy;
    uint32_t     start_tick;
    uint16_t     sensor_status;
    uint16_t     co2_ppm;
    uint16_t     old_co2_ppm;
    uint16_t     co2_value;
    bool         is_stable;
    uint8_t      is_twi_inited;
    power_mode_t power_mode;
    int16_t      cc_value;
    uint16_t     asc_count;
    int16_t      asc_correction;
    uint8_t      error_code;
    uint8_t      sensor_variant;
    uint8_t      sensor_error_count;
} co2_ctx_t;

// Function declarations
void asc_process_reading(uint16_t new_reading);
void asc_check_daily_update(bool check_record_count);
void asc_init(void);

#define CO2_PPM_WARN_GY_DEF        800
#define CO2_PPM_WARN_YR_DEF        1000
#define CO2_ILLUMINATION_THRESHOLD 800
#define CO2_CALIB_TARGET           426

extern void advertising_start(void);
extern void advertising_stop(void);

/**
 * @brief Initialize TWI (I2C) interface.
 *
 * @return uint8_t 0 on success, otherwise 1.
 */
uint8_t co2_twi_init(void);

/**
 * @brief Uninitialize TWI (I2C) interface.
 *
 * @return uint8_t 0 on success, otherwise 1.
 */
uint8_t co2_twi_uninit(void);

/**
 * @brief Write data to the CO2 sensor.
 *
 * @param addr Sensor address.
 * @param buf Data buffer.
 * @param len Data length.
 * @return uint8_t 0 on success, otherwise 1.
 */
uint8_t co2_write(uint8_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief Read data from the CO2 sensor.
 *
 * @param addr Sensor address.
 * @param buf Data buffer.
 * @param len Data length.
 * @return uint8_t 0 on success, otherwise 1.
 */
uint8_t co2_read(uint8_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief Get the CO2 concentration value.
 *
 * @return uint16_t CO2 concentration value.
 */
uint16_t get_co2_value(void);

/**
 * @brief Set the power mode (sampling mode).
 *
 * @param mode Power mode.
 */
void set_power_mode(power_mode_t mode);

/**
 * @brief Get the power mode (sampling mode).
 *
 * @return power_mode_t Power mode.
 */
power_mode_t get_power_mode(void);

/**
 * @brief Increment the power mode to the next mode.
 */
void inc_power_mode(void);

/**
 * @brief Set the CO2 warning thresholds.
 *
 * @param g_y Green to Yellow threshold.
 * @param y_r Yellow to Red threshold.
 */
void set_co2_warn_threshold(uint16_t g_y, uint16_t y_r);

/**
 * @brief Get the CO2 warning thresholds.
 *
 * @param g_y Pointer to store Green to Yellow threshold.
 * @param y_r Pointer to store Yellow to Red threshold.
 */
void get_co2_warn_threshold(uint16_t *g_y, uint16_t *y_r);

/**
 * @brief Start CO2 calibration.
 *
 */
void co2_start_calibration(void);

/**
 * @brief Update calibration target.
 *
 * @param target
 */
void update_calibration_target(uint16_t target);

/**
 * @brief Enable or disable automatic self-calibration of the sensor.
 *
 * @param enable 1 to enable, 0 to disable.
 * @return uint8_t 0 on success, otherwise 1.
 */
uint8_t co2_set_self_calibration(uint8_t enable);

/**
 * Get Error Code
 *
 * @return uint8_t Error code.
 */
uint8_t get_error_code(void);

#endif // __CO2_H__
