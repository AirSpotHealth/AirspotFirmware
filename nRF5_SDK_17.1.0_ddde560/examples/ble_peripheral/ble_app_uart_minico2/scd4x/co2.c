#include "co2.h"

#define ASC_PER_DAY_RECORD_COUNT 400

// CO2 power control macros (moved here to avoid "use before definition" errors)
#define CO2_PWR_ON()  nrf_gpio_pin_set(PIN_CO2_PWR)
#define CO2_PWR_OFF() nrf_gpio_pin_clear(PIN_CO2_PWR)

/* TWI instance ID. */
#define TWI_INSTANCE_ID 1

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

// Add these static variables
static asc_state_t asc_state = {
    .lowest3          = 10000, // Initialize to a high value
    .reading1         = 0,
    .reading2         = 0,
    .day_count        = 0,
    .reading0         = 0,
    .target           = CO2_CALIB_TARGET,
    .day_record_count = 0,
};

void asc_init(void)
{
    // Load CC from flash
    asc_state.lowest3          = 10000; // This will now store the lowest average, not the sum
    asc_state.day_count        = 0;
    asc_state.reading1         = 0;
    asc_state.reading2         = 0;
    asc_state.reading0         = 0;
    asc_state.day_record_count = 0;
}

static calib_ctx_t calib_ctx = {
    .power_mode_before_calib     = PWR_MODE_LOW,
    .calib_duration_record_count = 0,
    .calib_duration_sum          = 0,
    .co2_calib_target            = CO2_CALIB_TARGET,
};

static co2_ctx_t co2_ctx = {
    .tmp                = 0,
    .humi               = 0,
    .drdy               = SCD4X_BOOL_FALSE,
    .start_tick         = 0,
    .sensor_status      = 0,
    .co2_ppm            = 0,
    .old_co2_ppm        = 0,
    .co2_value          = 0,
    .is_stable          = true,
    .is_twi_inited      = 0,
    .power_mode         = PWR_MODE_LOW,
    .cc_value           = 0,
    .asc_count          = 0,
    .asc_correction     = 0,
    .error_code         = 0,
    .sensor_variant     = 2, // 2 means unknown and not measured
    .sensor_error_count = 0,
};

// Recovery mechanism state
static uint8_t consecutive_sensor_errors = 0;

/** Get CO2 concentration value */
uint16_t get_co2_value(void)
{
    return co2_ctx.co2_value;
}

/** Get Sensor Variant */
uint8_t get_sensor_variant(void)
{
    return co2_ctx.sensor_variant;
}

/** Get Error Code */
uint8_t get_error_code(void)
{
    return co2_ctx.error_code;
}

/** Get Consecutive Sensor Error Count */
uint8_t get_sensor_recovery_attempt(void)
{
    return consecutive_sensor_errors;
}

/** Get CC value */
int16_t get_cc_value(void)
{
    return co2_ctx.cc_value;
}

/** Get ASC day count */
uint8_t get_asc_day_count(void)
{
    return asc_state.day_count;
}

/** Get CC value */
int16_t get_calibration_correction(void)
{
    if (calib_ctx.calib_duration_record_count == 0)
    {
        return co2_ctx.cc_value;
    }

    // calculate average value of calibration readings
    int16_t average_value = (int16_t)(calib_ctx.calib_duration_sum / calib_ctx.calib_duration_record_count);
    print("average_value=%d, calib_duration_record_count=%d, calib_duration_sum=%d\n", average_value, calib_ctx.calib_duration_record_count, calib_ctx.calib_duration_sum);

    // calculate new CC value
    int16_t old_cc = cfg_fstorage_get_cc();

    // update CC value as new CC value - old CC value
    co2_ctx.cc_value = calib_ctx.co2_calib_target - average_value;

    cfg_fstorage_set_cc(co2_ctx.cc_value);

    print("old_cc=%d, cc_value=%d\n", old_cc, co2_ctx.cc_value);

    asc_state.reading1 = 0; // instead of asc_state.skip_3r we reset the reading1, reading2, reading0 to 0 which is the same effect
    asc_state.reading2 = 0;
    asc_state.reading0 = 0;

    return co2_ctx.cc_value - old_cc;
}

/** Set power mode, i.e., sampling mode */
void set_power_mode(power_mode_t mode)
{
    co2_ctx.power_mode = mode;
    cfg_fstorage_set_power_mode((uint8_t)mode);
}

/** Reset Power mode to what it was before calibration */
void reset_power_mode(void)
{
    set_power_mode(calib_ctx.power_mode_before_calib);
}

/** Get power mode, i.e., sampling mode */
power_mode_t get_power_mode(void)
{
    return co2_ctx.power_mode;
}

/** Increment power mode */
void inc_power_mode(void)
{
    co2_ctx.power_mode += 1;
    if (co2_ctx.power_mode > PWR_MODE_HI)
    {
        co2_ctx.power_mode = PWR_MODE_ON_DEMAND;
    }
    cfg_fstorage_set_power_mode((uint8_t)co2_ctx.power_mode);
}

/** Update calibration target
 *
 * @param target
 */
void update_calibration_target(uint16_t target)
{
    calib_ctx.co2_calib_target = target;
    asc_state.target           = target;
    cfg_fstorage_set_calib_target(target);
    add_history_record(target, RECORD_TYPE_CALIBRATION_TARGET_UPDATE);
}

/** Start CO2 calibration, battery level must be above 5 */
void co2_start_calibration()
{
    print("CALIBRATION TARGET: %d\n", calib_ctx.co2_calib_target);
    EventGroupSetBits(event_group_system, EVT_CO2_CALIB_START);
}

/**
 * @brief Initialize TWI
 *
 * @return uint8_t 0 on success, otherwise 1
 */
uint8_t co2_twi_init(void)
{
    if (co2_ctx.is_twi_inited) return 0;
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
        .scl                = PIN_CO2_SCL,
        .sda                = PIN_CO2_SDA,
        .frequency          = NRF_DRV_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW,
        .clear_bus_init     = false};

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    print("twi init with [%d]\n", err_code);
    if (err_code != NRF_SUCCESS) return 1;

    nrf_drv_twi_enable(&m_twi);
    co2_ctx.is_twi_inited = 1;
    return 0;
}

/**
 * @brief Uninitialize TWI
 *
 * @return uint8_t 0 on success
 */
uint8_t co2_twi_uninit(void)
{
    if (!co2_ctx.is_twi_inited) return 0;
    print("twi uninit\n");
    nrf_drv_twi_uninit(&m_twi);
    co2_ctx.is_twi_inited = 0;
    return 0;
}

/**
 * @brief Write data to the sensor
 *
 * @param addr Sensor address
 * @param buf Buffer
 * @param len Data length
 * @return uint8_t 0 on success, otherwise 1
 */
uint8_t co2_write(uint8_t addr, uint8_t *buf, uint16_t len)
{
    ret_code_t err_code;

    err_code = nrf_drv_twi_tx(&m_twi, addr, buf, len, false);
    return err_code != NRF_SUCCESS;
}

/**
 * @brief Write data to sensor without response
 *
 * @param addr address of sensor
 * @param buf buffer
 * @param len length of data
 * @return void
 */
void co2_write_void(uint8_t addr, uint8_t *buf, uint16_t len)
{
    nrf_drv_twi_tx(&m_twi, addr, buf, len, false);
}

/**
 * @brief 从传感器读取数据
 *
 * @param addr Sensor address
 * @param buf Buffer
 * @param len Data length
 * @return uint8_t 0 on success, otherwise 1
 */
uint8_t co2_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    ret_code_t err_code;

    err_code = nrf_drv_twi_rx(&m_twi, addr, buf, len);
    return err_code != NRF_SUCCESS;
}

// Adapt sensor driver, delay over 50ms returns immediately, non-blocking delay must be explicitly handled by the scheduler
void co2_delay_ms(uint32_t ms)
{
    if (ms > 50) return;
    nrf_delay_ms(ms);
}

void co2_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SEGGER_RTT_vprintf(0, fmt, &args);
    va_end(args);
}

// Generic CO2 sensor initialization functions (used by both factory test and normal operation)
int co2_sensor_setup(void)
{
    print("CO2 Sensor Setup\n");

    // GPIO setup
    nrf_gpio_cfg_output(PIN_CO2_PWR);
    CO2_PWR_OFF();

    // Handle setup
    DRIVER_SCD4X_LINK_INIT(&gs_handle, scd4x_handle_t);
    DRIVER_SCD4X_LINK_IIC_INIT(&gs_handle, co2_twi_init);
    DRIVER_SCD4X_LINK_IIC_DEINIT(&gs_handle, co2_twi_uninit);
    DRIVER_SCD4X_LINK_IIC_WRITE_COMMAND(&gs_handle, co2_write);
    DRIVER_SCD4X_LINK_IIC_WRITE_COMMAND_VOID(&gs_handle, co2_write_void);
    DRIVER_SCD4X_LINK_IIC_READ_COMMAND(&gs_handle, co2_read);
    DRIVER_SCD4X_LINK_DELAY_MS(&gs_handle, co2_delay_ms);
    DRIVER_SCD4X_LINK_DEBUG_PRINT(&gs_handle, co2_print);

    // Initialize driver with default type (will be updated after variant detection in configure)
    // Setting type here is required for driver initialization, but actual variant is read later
    scd4x_set_type(&gs_handle, SCD41); // Default type for initialization
    scd4x_init(&gs_handle);

    return 0;
}

int co2_sensor_power_on(void)
{
    print("CO2 Sensor Power On\n");
    CO2_PWR_ON();
    return 0; // Caller should delay 1500ms
}

int co2_sensor_twi_init(void)
{
    print("CO2 Sensor TWI Init\n");
    return co2_twi_init(); // This doesn't delay
}

int co2_sensor_configure(void)
{
    print("CO2 Sensor Configure\n");
    co2_ctx.error_code = 0;

    // NOTE: Caller MUST call scd4x_stop_periodic_measurement() and wait 500ms BEFORE calling this function
    // Per SCD4x datasheet Section 3.8: Configuration commands require sensor in IDLE state

    // Get sensor variant FIRST and set correct type
    // CRITICAL: SCD40 and SCD41 have different ranges, accuracy specs, and internal algorithms
    // SCD40: 400-2000 ppm range, ±(50ppm + 5%)
    // SCD41: 400-5000 ppm range, ±(40ppm + 5%)
    // Setting wrong type causes systematic reading errors (often triggering ERROR_CODE_CO2_READING_OUT_OF_RANGE_LOW)
    if (scd4x_get_sensor_variant(&gs_handle, &co2_ctx.sensor_variant) != 0)
    {
        print("Failed to get sensor variant, assuming SCD41\n");
        co2_ctx.sensor_variant = 1; // Assume SCD41 as fallback
    }
    print("Sensor variant: %d (0=SCD40, 1=SCD41, 2=Unknown)\n", co2_ctx.sensor_variant);

    // Set the correct sensor type based on detected variant
    // Variant 0 = SCD40 (raw data 0x04), Variant 1 = SCD41 (raw data 0x14)
    if (co2_ctx.sensor_variant == 0)
    {
        print("Configuring for SCD40\n");
        scd4x_set_type(&gs_handle, SCD40);
    }
    else
    {
        print("Configuring for SCD41\n");
        scd4x_set_type(&gs_handle, SCD41);
    }

    // Disable automatic self calibration (we use our own ASC)
    if (scd4x_set_automatic_self_calibration(&gs_handle, false) != 0)
    {
        print("Failed to disable auto calibration\n");
        co2_ctx.error_code = ERROR_CODE_SENSOR_AUTO_CALIB;
        return -1;
    }

    // Load CC value from storage
    co2_ctx.cc_value = cfg_fstorage_get_cc();
    print("CC value loaded: %d\n", co2_ctx.cc_value);

    return 0;
}

void co2_sensor_cleanup(void)
{
    print("CO2 Sensor Cleanup\n");
    if (co2_ctx.is_twi_inited)
    {
        co2_twi_uninit();
    }
    CO2_PWR_OFF();
}

scd4x_handle_t      gs_handle; /**< scd4x handle - exposed for factory test */
static scd4x_bool_t asc_enabled            = SCD4X_BOOL_FALSE;
static uint8_t      data_ready_check_count = 0; /**< Loop counter for data ready checks */

/**
 * @brief Enable/disable sensor automatic calibration
 *
 * @param enable 1 to enable, 0 to disable
 * @return uint8_t 0 on success, otherwise 1
 */
uint8_t co2_set_self_calibration(uint8_t enable)
{
    EventGroupSetBits(event_group_system, EVT_CO2_CALIB_MODE_CHANGE);
    asc_enabled = (scd4x_bool_t)enable;
    cfg_fstorage_set_auto_calibrate(enable);
    add_history_record(asc_enabled, RECORD_TYPE_SENSOR_AUTO_CALIBRATION); // add auto calibration record
    EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);

    if (asc_enabled)
    {
        asc_init();
    }

    return 0;
}

void asc_process_reading(uint16_t new_reading)
{
    if (!asc_enabled) return;

    asc_state.day_record_count++;

    asc_state.reading2 = asc_state.reading1;
    asc_state.reading1 = asc_state.reading0;
    asc_state.reading0 = new_reading;

    // Only process if we have 3 readings
    if (asc_state.reading2 == 0) return;

    // Calculate the average of 3 readings
    uint16_t avg_3r = (asc_state.reading0 + asc_state.reading1 + asc_state.reading2) / 3;

    print("asc_state.reading0=%d, asc_state.reading1=%d, asc_state.reading2=%d, avg3R=%d, asc_state.lowest3=%d\n",
          asc_state.reading0, asc_state.reading1, asc_state.reading2, avg_3r, asc_state.lowest3);

    // Track lowest 3-reading average (even if below 420, we'll enforce minimum when using it)
    if (avg_3r < asc_state.lowest3)
    {
        asc_state.lowest3 = avg_3r;
        print("asc_state.lowest3=%d\n", asc_state.lowest3);
    }

    // Special case: reading average < 420 (below acceptable minimum)
    // SKIP THIS FOR FLIGHT MODE
    if (asc_state.lowest3 < 420 && cfg_fstorage_get_flight_mode() == 0)
    {
        int16_t adjustment = 420 - asc_state.lowest3;

        if (adjustment == 0)
        {
            return;
        }

        co2_ctx.cc_value += adjustment;
        print("new cc value: %d\n", co2_ctx.cc_value);
        // Save CC to flash
        cfg_fstorage_set_cc(co2_ctx.cc_value);

        add_history_record(adjustment, RECORD_TYPE_ASC_ADJUSTMENT);

        print("Low CO2 adjust: %d\n", adjustment);
        asc_state.lowest3 = 420;
    }
}

void asc_check_daily_update(bool check_record_count)
{
    if (!asc_enabled) return;

    if (check_record_count && asc_state.day_record_count < ASC_PER_DAY_RECORD_COUNT) return;

    // This should be called at 00:01 each day
    asc_state.day_count++;
    asc_state.day_record_count = 0;

    print("Lowest CO2 this ASC period: %d\n", asc_state.lowest3);
    print("ASC day: %d\n", asc_state.day_count);

    if (asc_state.day_count >= 7)
    {
        int16_t old_cc = co2_ctx.cc_value;

        // Calculate new CC
        int16_t adjustment = asc_state.target - asc_state.lowest3;

        co2_ctx.cc_value += adjustment;

        print("Weekly ASC adjustment: %d, New CC: %d\n", adjustment, co2_ctx.cc_value);

        add_history_record(asc_state.target, RECORD_TYPE_CALIB_TARGET);
        add_history_record(asc_state.lowest3, RECORD_TYPE_ASC_LOWEST);
        add_history_record(old_cc, RECORD_TYPE_CALIBRATION_CORRECTION_OLD);
        add_history_record(co2_ctx.cc_value, RECORD_TYPE_CALIBRATION_CORRECTION);

        // Save CC to flash
        cfg_fstorage_set_cc(co2_ctx.cc_value);

        // Reset for next period
        asc_state.day_count        = 0;
        asc_state.lowest3          = 10000;
        asc_state.reading1         = 0;
        asc_state.reading2         = 0;
        asc_state.reading0         = 0;
        asc_state.target           = calib_ctx.co2_calib_target;
        asc_state.day_record_count = 0;

        EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);

        print("Weekly ASC adjustment: %d, New CC: %d\n", adjustment, co2_ctx.cc_value);
    }
}

float get_co2_scale_factor(void)
{
    float scale_factor = cfg_fstorage_get_co2_scale_factor();
    if (cfg_fstorage_get_flight_mode() == 1)
    {
        scale_factor = 1.6;
    }
    else
    {
        scale_factor = cfg_fstorage_get_co2_scale_factor();
    }

    print("CO2 Scaling: Factor=%d.%03d\n",
          (int)scale_factor,
          (int)((scale_factor - (int)scale_factor) * 1000));
    return scale_factor;
}

#define APP_CHECK(label, func_call, default_app_err_code)                                                      \
    do                                                                                                         \
    {                                                                                                          \
        uint8_t driver_ret = func_call;                                                                        \
        if (driver_ret != 0)                                                                                   \
        {                                                                                                      \
            print("call func %s failed at line %d, driver_ret code %d\n", #func_call, __LINE__, driver_ret);   \
            if (driver_ret == 1) /* SCD4X I2C/Comm error */                                                    \
            {                                                                                                  \
                co2_ctx.error_code = ERROR_CODE_SENSOR_I2C_COMM_FAILURE;                                       \
            }                                                                                                  \
            else if (driver_ret == 4) /* SCD4X CRC error */                                                    \
            {                                                                                                  \
                co2_ctx.error_code = ERROR_CODE_SENSOR_CRC_FAILURE;                                            \
            }                                                                                                  \
            /* Errors 2 (null handle) and 3 (not inited) from driver, or any other unexpected driver error, */ \
            /* will use the default_app_err_code for the specific operation. */                                \
            else                                                                                               \
            {                                                                                                  \
                co2_ctx.error_code = default_app_err_code;                                                     \
            }                                                                                                  \
            goto label;                                                                                        \
        }                                                                                                      \
    } while (0)

// macro for waiting the co2 up his timer to be cleared
#define WAIT_CO2_UP_HIS() TaskWait(EventGroupCheckBits(event_group_system, EVT_CO2_UP_HIS) == 0, TICK_MAX);

// Reusable function to get sensor details (extracted from handle_get_sensor_details)
sensor_details_t co2_get_sensor_details(scd4x_handle_t *handle)
{
    sensor_details_t details         = {0};
    uint16_t         serial_words[3] = {0, 0, 0};

    details.success = 1; // Assume success initially

    if (scd4x_get_temperature_offset(handle, &details.temp_offset_ticks) != 0)
    {
        print("Failed to get temperature offset\n");
        details.success = 0;
    }

    if (scd4x_get_sensor_altitude(handle, &details.altitude_m) != 0)
    {
        print("Failed to get sensor altitude\n");
        details.success = 0;
    }

    if (scd4x_get_ambient_pressure(handle, &details.ambient_pressure_mbar) != 0)
    {
        print("Failed to get ambient pressure\n");
        details.success = 0;
    }

    if (scd4x_get_serial_number(handle, serial_words) != 0)
    {
        print("Failed to get serial number\n");
        details.success = 0;
    }
    else
    {
        // Convert uint16_t words to uint8_t bytes for display
        details.serial_bytes[0] = (uint8_t)(serial_words[0] >> 8);
        details.serial_bytes[1] = (uint8_t)(serial_words[0] & 0xFF);
        details.serial_bytes[2] = (uint8_t)(serial_words[1] >> 8);
        details.serial_bytes[3] = (uint8_t)(serial_words[1] & 0xFF);
        details.serial_bytes[4] = (uint8_t)(serial_words[2] >> 8);
        details.serial_bytes[5] = (uint8_t)(serial_words[2] & 0xFF);
    }

    return details;
}

static void handle_get_sensor_details(scd4x_handle_t *p_gs_handle)
{
    print("Handling EVT_GET_SENSOR_DETAILS\n");

    EventGroupClearBits(event_group_system, EVT_GET_SENSOR_DETAILS);

    // Use the reusable function to get sensor details
    sensor_details_t details = co2_get_sensor_details(p_gs_handle);

    print("temp_offset_ticks: %d, altitude_m: %d, ambient_pressure_mbar: %d\n",
          details.temp_offset_ticks, details.altitude_m, details.ambient_pressure_mbar);

    if (details.success)
    {
        proto_send_sensor_details_response(details.temp_offset_ticks, details.altitude_m, details.ambient_pressure_mbar,
                                           cfg_fstorage_get_auto_calibrate(), cfg_fstorage_get_calib_target(), details.serial_bytes, get_sensor_variant());
    }
    else
    {
        print("Could not retrieve all sensor details. Response not sent.\n");
        // Optionally, send a response with error flags or default values.
    }
}

TaskDefine(task_co2_read)
{
    TTS
    {
        asc_enabled        = cfg_fstorage_get_auto_calibrate();
        co2_ctx.power_mode = cfg_fstorage_get_power_mode();
        print("task_co2_init run\n");

        print("task_co2_read run\n");

        // Use generic CO2 sensor initialization functions
        co2_sensor_setup();

        co2_sensor_power_on();
        TaskDelay(1500 / TICK_RATE_MS); // wait for sensor power on complete

        if (co2_sensor_twi_init() != 0)
        {
            print("CO2 TWI initialization failed during startup\n");
            co2_ctx.error_code = ERROR_CODE_SENSOR_I2C_COMM_FAILURE;
            goto sensor_error;
        }
        TaskDelay(500 / TICK_RATE_MS);

        if (cfg_fstorage_get_erase_required() == 1)
        {
            print("CO2 factory reset required\n");
            scd4x_perform_factory_reset(&gs_handle);
            cfg_fstorage_set_cc(0);

            TaskDelay(1200 / TICK_RATE_MS); // wait for sensor to reset
        }

        // Stop any ongoing measurement before configuration (per SCD4x datasheet Section 3.8)
        print("Stopping periodic measurement before configuration\n");
        scd4x_stop_periodic_measurement(&gs_handle);
        TaskDelay(500 / TICK_RATE_MS); // Wait for sensor to enter IDLE state

        // Configure sensor (ASC disable, CC load, sensor variant)
        if (co2_sensor_configure() != 0)
        {
            print("CO2 sensor configuration failed during startup\n");
            // Error code already set by co2_sensor_configure()
            goto sensor_error;
        }

        // Initialize ASC if enabled
        calib_ctx.co2_calib_target = cfg_fstorage_get_calib_target();
        if (asc_enabled)
        {
            asc_init();
        }

        while (1)
        {
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
            {
                print("BATTERY LOW, exit\n");
                break; // Exit the loop and the task, it will be restarted when the battery is charged
            }

            if (EventGroupCheckBits(event_group_system, EVT_GET_SENSOR_DETAILS))
            {
                EventGroupClearBits(event_group_system, EVT_GET_SENSOR_DETAILS);
                handle_get_sensor_details(&gs_handle);
                goto sensor_wait;
            }

            // check if the sensor is powered on
            if (!co2_ctx.is_twi_inited)
            {
                print("DEVICE REBOOTED - Re-initializing sensor\n");

                // Re-initialize sensor using generic functions
                co2_sensor_power_on();
                TaskDelay(1500 / TICK_RATE_MS); // wait for sensor power on complete

                if (co2_sensor_twi_init() != 0)
                {
                    print("CO2 TWI initialization failed after device reboot\n");
                    co2_ctx.error_code = ERROR_CODE_SENSOR_I2C_COMM_FAILURE;
                    goto sensor_error;
                }
                TaskDelay(500 / TICK_RATE_MS);

                co2_ctx.error_code = 0;

                print("DEVICE REBOOTED SETTING ASC TO FALSE\n");
                APP_CHECK(sensor_error, scd4x_set_automatic_self_calibration(&gs_handle, false), ERROR_CODE_SENSOR_AUTO_CALIB); // set automatic self calibration
            }

            if (EventGroupCheckBits(event_group_system, EVT_CO2_CALIB_START))
            {
                EventGroupClearBits(event_group_system, EVT_CO2_CALIB_START);

                calib_ctx.calib_duration_record_count = 0;
                calib_ctx.calib_duration_sum          = 0;
                EventGroupSetBits(event_group_system, EVT_CO2_CALIB_TIMING);
                calib_ctx.power_mode_before_calib = co2_ctx.power_mode;

                // wait for the co2 up his timer to be cleared
                WAIT_CO2_UP_HIS();

                add_history_record(1, RECORD_TYPE_MANUAL_CALIB_START);
                add_history_record(co2_ctx.cc_value, RECORD_TYPE_CALIBRATION_CORRECTION_OLD);
                add_history_record(calib_ctx.co2_calib_target, RECORD_TYPE_CALIB_TARGET);

                EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);

                if (co2_ctx.power_mode == PWR_MODE_ON_DEMAND)
                {
                    print("Calibration in on demand mode, switch to mid power mode\n");
                    set_power_mode(PWR_MODE_MID);
                }
                print("start calibrate: %d\n", co2_ctx.power_mode);
            }

            if (EventGroupCheckBits(event_group_system, EVT_CO2_FACTORY_RESET | EVT_CO2_CALIB_MODE_CHANGE))
            {
                APP_CHECK(sensor_error, scd4x_perform_factory_reset(&gs_handle), ERROR_CODE_SENSOR_RESET); // reset sensor
                TaskDelay(1200 / TICK_RATE_MS);

                APP_CHECK(sensor_error, scd4x_set_automatic_self_calibration(&gs_handle, false), ERROR_CODE_SENSOR_AUTO_CALIB); // set automatic self calibration
                                                                                                                                // wait for sensor to reset

                if (EventGroupCheckBits(event_group_system, EVT_CO2_CALIB_MODE_CHANGE))
                {
                    EventGroupClearBits(event_group_system, EVT_CO2_CALIB_MODE_CHANGE); // clear self calibration flag
                }
                else
                {
                    co2_ctx.cc_value           = 0;                // reset cc value
                    calib_ctx.co2_calib_target = CO2_CALIB_TARGET; // reset co2 calib target
                    cfg_fstorage_set_cc(co2_ctx.cc_value);         // save cc value to flash

                    // wait for the co2 up his timer to be cleared
                    WAIT_CO2_UP_HIS();

                    add_history_record(1, RECORD_TYPE_SENSOR_USER_FACTORY_RESET); // add factory reset record

                    EventGroupClearBits(event_group_system, EVT_CO2_FACTORY_RESET); // clear factory reset flag
                    // clear self calibration flag

                    NUS_TAKE();
                    send_factory_reset_done(asc_enabled); // send factory reset done to NUS
                    NUS_GIVE();
                }
            }

            co2_ctx.start_tick = GetSystemTickCount();

            APP_CHECK(sensor_error, scd4x_start_periodic_measurement(&gs_handle), ERROR_CODE_SENSOR_MEASURMENT_START);

            do
            {
                print("scd4x_start_single_shot\n");

                EventGroupSetBits(event_group_system, EVT_BATTER_ADC_EN);     // Enable battery sampling
                EventGroupClearBits(event_group_system, EVT_CO2_UPDATE_ONCE); // clear update once flag

                TaskWait(EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING), 5000 / TICK_RATE_MS); // wait for 5 seconds or battery low

                if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
                {
                    print("battery low, break\n");
                    break;
                }

                // check if data is ready
                APP_CHECK(sensor_error, scd4x_get_data_ready_status(&gs_handle, &co2_ctx.drdy), ERROR_CODE_SENSOR_DATA_STATUS);

                if (!co2_ctx.drdy)
                {
                    print("CO2 data not ready: Sensor responsive but DRDY flag timeout after 10s.\n");
                    co2_ctx.error_code = ERROR_CODE_SENSOR_DATA_STATUS;
                    goto sensor_error;
                }

                APP_CHECK(sensor_error, scd4x_read(&gs_handle, &co2_ctx.co2_ppm, &co2_ctx.sensor_status, &co2_ctx.asc_count, &co2_ctx.asc_correction), ERROR_CODE_SENSOR_READ);
                print("read co2 %d ppm, sensor status %d\n", co2_ctx.co2_ppm, co2_ctx.sensor_status);

                // if the sensor status is not zero, it means the sensor is not ready
                if (co2_ctx.sensor_status != 0)
                {
                    print("co2 sensor status not ready: %d\n", co2_ctx.sensor_status);
                    co2_ctx.error_code = ERROR_CODE_SENSOR_NOT_READY;
                    goto sensor_error;
                }

                // use scaling to get the real co2 ppm
                co2_ctx.co2_ppm = (uint16_t)(co2_ctx.co2_ppm * get_co2_scale_factor());
                print("co2_ppm after scaling: %d\n", co2_ctx.co2_ppm);

                // Check for invalid CO2 readings (only extremely high values)
                // Note: Low values (< 400 ppm) are normal for clean air and are clamped to 400 in display
                // SCD40/SCD41 range: 400-5000 ppm specified, but readings down to ~300 ppm are valid for outdoor air
                if (co2_ctx.co2_ppm > 50000)
                {
                    print("Invalid CO2 reading: %d ppm (too high)\n", co2_ctx.co2_ppm);
                    co2_ctx.error_code = ERROR_CODE_CO2_READING_OUT_OF_RANGE_HIGH;
                    goto sensor_error;
                }

                if (EventGroupCheckBits(event_group_system, EVT_CO2_CALIB_TIMING))
                {
                    calib_ctx.calib_duration_record_count += 1;
                    calib_ctx.calib_duration_sum += co2_ctx.co2_ppm;

                    // wait for the co2 up his timer to be cleared
                    WAIT_CO2_UP_HIS();

                    add_history_record(co2_ctx.co2_ppm, RECORD_TYPE_CO2);
                    EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
                }
                else
                {

                    if (co2_ctx.old_co2_ppm != 0)
                    {
                        // if the co2 ppm is changed > 2x or < 0.5x, it means the sensor is not stable
                        // skip this reading and continue
                        if (co2_ctx.co2_ppm > co2_ctx.old_co2_ppm * 1.5 || co2_ctx.co2_ppm < co2_ctx.old_co2_ppm * 0.75)
                        {
                            print("co2 ppm is not stable: %d\n", co2_ctx.co2_ppm);

                            // But if it was already unstable, that means it is a real change
                            if (co2_ctx.is_stable)
                            {
                                co2_ctx.is_stable = false;

                                // wait for the co2 up his timer to be cleared
                                WAIT_CO2_UP_HIS();

                                // Add the adjusted value to the history record for reporting
                                add_history_record(co2_ctx.co2_ppm + co2_ctx.cc_value, RECORD_TYPE_CO2_RETRY);
                                continue;
                            }
                            else
                            {
                                co2_ctx.is_stable   = true;
                                co2_ctx.old_co2_ppm = co2_ctx.co2_ppm;
                            }
                        }
                        else
                        {
                            co2_ctx.is_stable   = true;
                            co2_ctx.old_co2_ppm = co2_ctx.co2_ppm;
                        }
                    }

                    print("CO2 PPM: %d, CC: %d, is_stable: %d\n", co2_ctx.co2_ppm, co2_ctx.cc_value, co2_ctx.is_stable);

                    co2_ctx.old_co2_ppm = co2_ctx.co2_ppm;
                    co2_ctx.is_stable   = true;

                    // Now that we have the stable reading, add cc value to co2 ppm
                    co2_ctx.co2_ppm = (uint16_t)(co2_ctx.co2_ppm + co2_ctx.cc_value);

                    // wait for the co2 up his timer to be cleared
                    WAIT_CO2_UP_HIS();

                    add_history_record(co2_ctx.co2_ppm, RECORD_TYPE_CO2);
                    if (asc_enabled)
                    {
                        asc_process_reading(co2_ctx.co2_ppm);
                    }

                    co2_ctx.co2_value = co2_ctx.co2_ppm;

                    // if the co2 up history flag is set, wait for co2 up history flag to be cleared
                    // before sending the co2 value to the NUS

                    EventGroupSetBits(event_group_system, EVT_UI_UP_CO2 | EVT_CO2_UP_HIS | EVT_CO2_UP_ALARM); // Enable battery sampling first, CO2 alarm is enabled after battery sampling is completed
                    NUS_TAKE();
                    send_realtime_co2();
                    NUS_GIVE();
                }

                co2_ctx.sensor_error_count = 0; // reset sensor error count after a successful reading
                consecutive_sensor_errors  = 0; // reset consecutive error count after successful reading

                if (EventGroupCheckBits(event_group_system, EVT_CO2_MEASUREMENT_BREAK)) break; // Exit on break condition

            } while (co2_ctx.power_mode == PWR_MODE_HI || !co2_ctx.is_stable);

            goto sensor_wait;

        sensor_wait:
            APP_CHECK(sensor_error, scd4x_stop_periodic_measurement(&gs_handle), ERROR_CODE_SENSOR_MEASURMENT_STOP);
            TaskDelay(500 / TICK_RATE_MS); // Wait 500ms to make sure the sensor stops the measurement

            if (co2_ctx.power_mode == PWR_MODE_MID)
            {
                print("mid power wait\n");
                TaskWait(
                    co2_ctx.power_mode != PWR_MODE_MID ||
                        EventGroupCheckBits(event_group_system, EVT_CO2_UPDATE) ||
                        GetSystemTickSpan(co2_ctx.start_tick, GetSystemTickCount()) >= (1 * 60 * 1000) / TICK_RATE_MS,
                    TICK_MAX);
            }
            //
            //
            else if (co2_ctx.power_mode == PWR_MODE_LOW)
            {
                print("low power wait\n");
                TaskWait(
                    co2_ctx.power_mode != PWR_MODE_LOW ||
                        EventGroupCheckBits(event_group_system, EVT_CO2_UPDATE) ||
                        GetSystemTickSpan(co2_ctx.start_tick, GetSystemTickCount()) >= (3 * 60 * 1000) / TICK_RATE_MS,
                    TICK_MAX);
            }
            else // PWR_MODE_ON_DEMAND
            {
                print("on demand power wait\n");
                TaskWait(
                    co2_ctx.power_mode != PWR_MODE_ON_DEMAND ||
                        EventGroupCheckBits(event_group_system, EVT_CO2_UPDATE),
                    TICK_MAX);
            }
            EventGroupClearBits(event_group_system, EVT_CO2_UPDATE_ONCE);
            print("co2 wait end\n");
            if (!EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
            {
                NUS_TAKE();
                proto_send_battery_level();
                NUS_GIVE();
            }

            continue;

        sensor_error:
            print("read scd4x sensor error - error_code=%d, consecutive_errors=%d\n", co2_ctx.error_code, consecutive_sensor_errors + 1);

            // if the error code is not start measurement, stop the measurement first
            if (co2_ctx.error_code != ERROR_CODE_SENSOR_MEASURMENT_START)
            {
                scd4x_stop_periodic_measurement(&gs_handle);
                TaskDelay(500 / TICK_RATE_MS); // Wait 500ms to make sure the sensor stops the measurement
            }

            // wait for the co2 up his timer to be cleared
            WAIT_CO2_UP_HIS();

            add_history_record(co2_ctx.error_code, RECORD_TYPE_SENSOR_ERROR);

            // First error: just log and retry immediately
            if (co2_ctx.sensor_error_count == 0)
            {
                print("First error - logging and retrying immediately\n");
                EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
                co2_ctx.sensor_error_count += 1;
                consecutive_sensor_errors++;
                continue;
            }
            // Second error: power cycle sensor and retry (silent - no display to user)
            else if (consecutive_sensor_errors == 1)
            {
                print("Second error - power cycling sensor (silent recovery)\n");
                consecutive_sensor_errors++;
                EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS); // Log to history only

                // Power cycle sensor
                if (co2_ctx.is_twi_inited)
                {
                    co2_twi_uninit();
                }
                CO2_PWR_OFF();

                TaskDelay(2000 / TICK_RATE_MS); // 2 second power-off

                continue;
            }
            // Third error: restart entire device
            else
            {
                print("Third consecutive error - RESTARTING DEVICE\n");
                EventGroupSetBits(event_group_system, EVT_CO2_SENSOR_ERROR | EVT_CO2_UP_HIS);
                EventGroupSetBits(event_group_system, EVT_SCREEN_ON_ONETIME);

                // Show error briefly before restart
                TaskDelay(2000 / TICK_RATE_MS);

                // Log device restart to history
                
                WAIT_CO2_UP_HIS();
                add_history_record(0xFF, RECORD_TYPE_SENSOR_ERROR); // 0xFF = device restart marker
                EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
                TaskDelay(100 / TICK_RATE_MS); // Brief delay to ensure history write completes

                // System reset
                print("TIMESTAMP BEFORE SENSOR-ERROR RESET: %u\n", get_time_now());
                NVIC_SystemReset();
            }
        }

        // Cleanup sensor using generic function
        co2_sensor_cleanup();
        print("END CO2\n");
        TTE
    }
}

// Helper struct and function for falling alarm logic
typedef struct
{
    uint8_t  repeats_to_use;
    uint16_t derived_from_co2_level; // The co2_level of the next lower threshold, or 0 if default repeats from current alarm were used
} falling_alarm_params_t;

static falling_alarm_params_t get_falling_alarm_details(const advanced_alarm_setting_t *p_alarms, uint8_t current_alarm_idx_fallen_below)
{
    falling_alarm_params_t params;
    uint8_t                k_loop;

    // Default to the repeats of the alarm level we just fell below
    params.repeats_to_use         = p_alarms[current_alarm_idx_fallen_below].alarm_repeats;
    params.derived_from_co2_level = 0; // Indicates default repeats are used so far

    // Find the highest CO2 level of an enabled alarm that is *lower* than the one we just fell below.
    for (k_loop = 0; k_loop < 10; ++k_loop)
    {
        if (p_alarms[k_loop].notification_enabled && p_alarms[k_loop].alarm_repeats > 0 && p_alarms[k_loop].co2_level > 0 &&
            p_alarms[k_loop].co2_level < p_alarms[current_alarm_idx_fallen_below].co2_level)
        { // Must be strictly lower

            // If this lower alarm's level is higher than any other *lower* alarm level found so far,
            // it becomes our candidate for deriving the repeat count.
            if (p_alarms[k_loop].co2_level > params.derived_from_co2_level)
            { // derived_from_co2_level stores the best candidate's CO2 level so far
                params.derived_from_co2_level = p_alarms[k_loop].co2_level;
                params.repeats_to_use         = p_alarms[k_loop].alarm_repeats;
            }
        }
    }
    return params;
}

static bool advanced_alarms_enabled = false; // Track if any advanced alarms are enabled

// Function to update advanced alarms enabled state
static void update_advanced_alarms_state(void)
{
    advanced_alarm_setting_t alarms[10];
    cfg_fstorage_get_all_advanced_alarms(alarms);

    advanced_alarms_enabled = false;
    for (uint8_t i = 0; i < 10; ++i)
    {
        if (alarms[i].notification_enabled && alarms[i].alarm_repeats > 0 && alarms[i].co2_level > 0)
        {
            advanced_alarms_enabled = true;
            break;
        }
    }
    print("Advanced alarms state updated: %d\n", advanced_alarms_enabled);
}

// Helper function to evaluate basic yellow/red alarms
static uint8_t evaluate_basic_alarms(uint16_t current_co2, uint16_t last_co2, bool co2_is_falling, bool falling_alarms_enabled)
{
    uint8_t  active_alarm_repeats = 0;
    uint16_t yellow_start         = cfg_fstorage_get_graph_yellow_start();
    uint16_t red_start            = cfg_fstorage_get_graph_red_start();

    print("Evaluating basic alarms for yellow_start: %u, red_start: %u\n", yellow_start, red_start);

    // Check yellow transition point (rising)
    if (current_co2 >= yellow_start && last_co2 < yellow_start)
    {
        print("ALARM TYPE: YELLOW TRANSITION - CO2 %u >= Level %u\n", current_co2, yellow_start);
        active_alarm_repeats = 1; // Default to 1 repeat for yellow
    }
    // Check red transition point (rising)
    else if (current_co2 >= red_start && last_co2 < red_start)
    {
        print("ALARM TYPE: RED TRANSITION - CO2 %u >= Level %u\n", current_co2, red_start);
        active_alarm_repeats = 2; // Default to 2 repeats for red
    }
    // Check falling transitions if falling alarms are enabled
    else if (falling_alarms_enabled && co2_is_falling)
    {
        // Check falling from red
        if (last_co2 >= red_start && current_co2 < red_start)
        {
            print("ALARM TYPE: FALLING FROM RED - CO2 %u < Level %u (from %u)\n", current_co2, red_start, last_co2);
            active_alarm_repeats = 2; // Use same number of repeats as rising red
        }
        // Check falling from yellow
        else if (last_co2 >= yellow_start && current_co2 < yellow_start)
        {
            print("ALARM TYPE: FALLING FROM YELLOW - CO2 %u < Level %u (from %u)\n", current_co2, yellow_start, last_co2);
            active_alarm_repeats = 1; // Use same number of repeats as rising yellow
        }
    }
    return active_alarm_repeats;
}

// Helper function to evaluate advanced alarms
static uint8_t evaluate_advanced_alarms(
    const advanced_alarm_setting_t *p_alarms,
    uint16_t                        current_co2,
    uint16_t                        last_co2,
    bool                            co2_is_falling,
    bool                            falling_alarms_enabled,
    uint16_t                       *last_alarm_triggered_level,           // Array of 10
    uint16_t                       *has_falling_alarm_triggered_for_level // Array of 10
)
{
    uint8_t active_alarm_repeats = 0;
    uint8_t alarm_index;

    for (alarm_index = 0; alarm_index < 10; ++alarm_index)
    {
        if (!p_alarms[alarm_index].notification_enabled || p_alarms[alarm_index].alarm_repeats == 0 || p_alarms[alarm_index].co2_level == 0)
        {
            last_alarm_triggered_level[alarm_index]            = 0;
            has_falling_alarm_triggered_for_level[alarm_index] = 0;
            continue;
        }

        // --- RISING ALARM LOGIC ---
        if (current_co2 >= p_alarms[alarm_index].co2_level)
        {
            if (last_alarm_triggered_level[alarm_index] == 0)
            {
                print("ALARM TYPE: RISING - CO2 %u >= Level %u. Repeats: %d\n", current_co2, p_alarms[alarm_index].co2_level, p_alarms[alarm_index].alarm_repeats);
                if (p_alarms[alarm_index].alarm_repeats > active_alarm_repeats)
                {
                    active_alarm_repeats = p_alarms[alarm_index].alarm_repeats;
                }
                last_alarm_triggered_level[alarm_index] = current_co2;
            }
            has_falling_alarm_triggered_for_level[alarm_index] = 0;
        }
        // --- FALLING ALARM LOGIC ---
        else // current_co2 < p_alarms[alarm_index].co2_level
        {
            last_alarm_triggered_level[alarm_index] = 0;

            if (falling_alarms_enabled && co2_is_falling &&
                last_co2 >= p_alarms[alarm_index].co2_level &&
                has_falling_alarm_triggered_for_level[alarm_index] == 0)
            {
                falling_alarm_params_t fall_params            = get_falling_alarm_details(p_alarms, alarm_index);
                uint8_t                repeats_for_this_event = fall_params.repeats_to_use;

                print("ALARM TYPE: FALLING - CO2 %u < Level %u (from %u). Using repeats: %d (derived from next lower active level %u, or default if none lower)\n",
                      current_co2,
                      p_alarms[alarm_index].co2_level, last_co2, repeats_for_this_event, fall_params.derived_from_co2_level);

                if (repeats_for_this_event > active_alarm_repeats)
                {
                    active_alarm_repeats = repeats_for_this_event;
                }
                has_falling_alarm_triggered_for_level[alarm_index] = p_alarms[alarm_index].co2_level;
            }
        }
    }
    return active_alarm_repeats;
}

TaskDefine(task_co2_alarm)
{
    static uint8_t                  vibrator_state, buzzer_enabled_state;
    static uint16_t                 last_co2_value       = 0;
    static uint8_t                  active_alarm_repeats = 0; // Must be static
    static advanced_alarm_setting_t alarms[10];
    static uint16_t                 last_alarm_triggered_level[10]            = {0};
    static uint16_t                 has_falling_alarm_triggered_for_level[10] = {0};
    static bool                     co2_is_falling                            = false; // Moved and made static
    static bool                     falling_alarms_enabled                    = false; // Moved and made static

    TTS
    {

        while (1)
        {
            // Declarations of co2_is_falling and falling_alarms_enabled removed from here

            EventGroupWaitBits(event_group_system, EVT_CO2_UP_ALARM | EVT_FIND_DEVICE | EVT_BAT_LOW | EVT_BAT_LOW_WARNING | EVT_CO2_CALIB_MODE_CHANGE, TICK_MAX);

            update_advanced_alarms_state();

            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
            {
                print("task_co2_alarm: Battery low, exiting task.\n");
                pwm_uninit();
                break;
            }

            if (EventGroupCheckBits(event_group_system, EVT_CO2_UP_ALARM))
            {
                EventGroupClearBits(event_group_system, EVT_CO2_UP_ALARM);

                if (is_dnd_time())
                {
                    print("DND time, skip alarm\n");
                    last_co2_value       = co2_ctx.co2_value;
                    active_alarm_repeats = 0; // Reset if DND, ensures no stale alarm plays
                    continue;
                }

                co2_is_falling         = (last_co2_value != 0 && co2_ctx.co2_value < last_co2_value);
                falling_alarms_enabled = cfg_fstorage_get_alarm_co2_falling();

                print("CO2 Alarm Eval: Current=%u, Last=%u, IsFalling=%d, FallingAlarmsEnabled=%d\n",
                      co2_ctx.co2_value, last_co2_value, co2_is_falling, falling_alarms_enabled);

                if (co2_is_falling && !falling_alarms_enabled)
                {
                    print("CO2 is falling & falling alarms are OFF. Skipping alarm triggering logic.\n");
                    last_co2_value       = co2_ctx.co2_value;
                    active_alarm_repeats = 0; // Reset if skipping
                    continue;
                }

                vibrator_state       = cfg_fstorage_get_vibrator_mode();
                buzzer_enabled_state = cfg_fstorage_get_buzzer_mode();

                // Determine repeats using helper functions
                if (!advanced_alarms_enabled)
                {
                    active_alarm_repeats = evaluate_basic_alarms(co2_ctx.co2_value, last_co2_value, co2_is_falling, falling_alarms_enabled);
                }
                else
                {
                    cfg_fstorage_get_all_advanced_alarms(alarms);
                    active_alarm_repeats = evaluate_advanced_alarms(alarms, co2_ctx.co2_value, last_co2_value, co2_is_falling, falling_alarms_enabled, last_alarm_triggered_level, has_falling_alarm_triggered_for_level);
                }

                last_co2_value = co2_ctx.co2_value;

                print("Final active_alarm_repeats for this cycle: %u\n", active_alarm_repeats);

                if (active_alarm_repeats > 0 && cfg_fstorage_get_illuminate_screen_on_alarm())
                {
                    EventGroupSetBits(event_group_system, EVT_SCREEN_ON_ONETIME);
                }

                if (!vibrator_state && !buzzer_enabled_state)
                {
                    active_alarm_repeats = 0;
                }

                if (active_alarm_repeats == 0)
                {
                    print("No alarm repeats, skipping.\n");
                    continue;
                }

                while (active_alarm_repeats > 0)
                {
                    print("Alarm counting down %u\n", active_alarm_repeats);
                    if (buzzer_enabled_state)
                    {
                        pwm_init();
                        BUZZER_ON();
                    }
                    if (vibrator_state)
                    {
                        VIBRATOR_ON();
                    }
                    TaskDelay(250 / TICK_RATE_MS);

                    if (buzzer_enabled_state)
                    {
                        BUZZER_OFF();
                    }
                    if (vibrator_state)
                    {
                        VIBRATOR_OFF();
                    }
                    pwm_uninit();

                    if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW_WARNING | EVT_BAT_LOW))
                    {
                        print("Battery low during alarm sequence, breaking.\n");
                        active_alarm_repeats = 0; // Stop further repeats
                        break;
                    }
                    active_alarm_repeats--;
                    TaskDelay(250 / TICK_RATE_MS);
                }
            }
            else if (EventGroupCheckBits(event_group_system, EVT_FIND_DEVICE))
            {
                EventGroupClearBits(event_group_system, EVT_FIND_DEVICE);

                active_alarm_repeats = 10;

                while (active_alarm_repeats > 0)
                {
                    pwm_init();
                    BUZZER_ON();
                    VIBRATOR_ON();
                    TaskDelay(250 / TICK_RATE_MS);
                    BUZZER_OFF();
                    VIBRATOR_OFF();
                    pwm_uninit();

                    TaskDelay(250 / TICK_RATE_MS);

                    active_alarm_repeats--;

                    if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW_WARNING | EVT_BAT_LOW))
                    {
                        print("Battery low during find device alarm, stopping alarm.\n");
                        active_alarm_repeats = 0; // Stop further repeats
                        break;
                    }
                }
            }
        }

        print("END CO2 ALARM TASK (BATTERY LOW or other exit)\n");
        pwm_uninit();
    }
    TTE
}
