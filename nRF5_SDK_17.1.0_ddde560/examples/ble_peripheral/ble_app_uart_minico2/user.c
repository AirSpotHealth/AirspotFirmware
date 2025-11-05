#include "user.h"
#include "history.h"

DefSystemTickCount();

EventGroup_t event_group_system = 0;

APP_PWM_INSTANCE(PWM1, 1); // Create the instance "PWM1" using TIMER1.

static __attribute__((section(".bss.noinit"), zero_init)) volatile uint32_t time_base;                            // Seconds since 2000-01-01 00:00:00
static uint8_t                                                              inc_hour_cnt = 0, inc_minute_cnt = 0; // Increment counters for hours and minutes
static bool                                                                 time_set                    = false;  // Indicates if the time is set by the user
static uint16_t                                                             asc_check_duration          = 0;      // 2 hours in seconds
static uint32_t                                                             last_asc_check_time         = 0;      // Stores the last time ASC check was triggered
static uint32_t                                                             flight_mode_activation_time = 0;      // Stores the time when flight mode was activated

extern void proto_send_device_state(void);

/** Set buzzer state */
void set_buzzer_state(uint8_t on_off)
{
    cfg_fstorage_set_buzzer_mode(on_off);
}

/** Get buzzer state */
uint8_t get_buzzer_state(void)
{
    return cfg_fstorage_get_buzzer_mode();
}

/** Set vibrator state */
void set_vibrator_state(uint8_t on_off)
{
    cfg_fstorage_set_vibrator_mode(on_off);
}

/** Get vibrator state */
uint8_t get_vibrator_state(void)
{
    return cfg_fstorage_get_vibrator_mode();
}

/** Set local time (seconds since 2000-01-01 00:00:00) */
void set_timebase(uint32_t time)
{
    time_base = time;
    EventGroupSetBits(event_group_system, EVT_TIME_UPDATE | EVT_TIMEBASE_UP);
}

/** Prepare to edit hour */
void prepare_edit_hour(void)
{
    inc_hour_cnt = 0;
}

/** Increment hour by 1 */
void inc_hour(void)
{
    time_base += 3600; // Increment by 1 hour
    if (++inc_hour_cnt == 24)
    {
        inc_hour_cnt = 0;
        time_base -= 24 * 3600;
    }
    time_set = true;
}

/** Prepare to edit minute */
void prepare_edit_minute(void)
{
    inc_minute_cnt = 0;
}

/** Increment minute by 1 */
void inc_minute(void)
{
    time_base += 60; // Increment by 1 minute
    if (++inc_minute_cnt == 60)
    {
        inc_minute_cnt = 0;
        time_base -= 60 * 60;
    }
    time_set = true;
}

/** Increment second by 1 */
void inc_second(void)
{
    time_base += 1;
    if (time_base % 60 == 0)
    {
        // if not in low battery mode, set the time update and battery adc enable
        if (!EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
        {
            EventGroupSetBits(event_group_system, EVT_TIME_UPDATE | EVT_BATTER_ADC_EN);
        }
    }

    if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING)) return;

    // Check for flight mode auto-disable
    if (cfg_fstorage_get_flight_mode() && flight_mode_activation_time != 0)
    {
        // 12 hours = 12 * 60 * 60 seconds = 43200 seconds
        if ((get_time_now() - flight_mode_activation_time) >= 43200)
        {
            print("Flight mode auto-disabled after 12 hours.\n");
            cfg_fstorage_set_flight_mode(0);                                                // Disable flight mode
            update_flight_mode_activation_time();                                           // Reset the activation time
            add_history_record(0, RECORD_TYPE_FLIGHT_MODE);                                 // Log flight mode deactivation
            EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS | EVT_FLIGHT_MODE_UPDATE); // Notify system about history update

            // send the device state
            proto_send_device_state();
        }
    }

    if (asc_check_duration == 0)
    {

        // check if it's 23:59 and time_set is true
        if (get_time_now_hour24() == 23 && get_time_now_minute() == 59 && time_set)
        {
            if (time_base - last_asc_check_time >= 60)
            {
                asc_check_daily_update(true);
                last_asc_check_time = time_base;
            }
        }
    }
    else
    {
        // check if it's the time to check the daily update
        if (asc_check_duration > 0 && time_base % asc_check_duration == 0 && cfg_fstorage_get_power_mode() != 0)
        {
            print("asc check duration: %d, last_asc_check_time: %d, time_base: %d\n", asc_check_duration, last_asc_check_time, time_base);
            if (time_base - last_asc_check_time >= asc_check_duration)
            {
                asc_check_daily_update(false);
                last_asc_check_time = time_base;
            }
        }
    }
}

/** Get current time in seconds since 2000-01-01 00:00:00 */
uint32_t get_time_now(void)
{
    return time_base;
}

/** Get current hour (24-hour format) */
uint8_t get_time_now_hour24(void)
{
    uint32_t current_seconds   = time_base % 60;
    uint32_t remaining_minutes = (time_base - current_seconds) / 60;
    uint32_t current_minutes   = remaining_minutes % 60;
    uint32_t remaining_hours   = (remaining_minutes - current_minutes) / 60;
    uint32_t current_hours     = remaining_hours % 24;

    return (uint8_t)current_hours;
}

/** Get current minute */
uint8_t get_time_now_minute(void)
{
    uint32_t current_seconds   = time_base % 60;
    uint32_t remaining_minutes = (time_base - current_seconds) / 60;
    uint32_t current_minutes   = remaining_minutes % 60;

    return (uint8_t)current_minutes;
}

/** Get device name */
char *get_device_name(void)
{
    return cfg_fstorage_get_device_name();
}

/** Set device name */
void set_device_name(char *name)
{
    cfg_fstorage_set_device_name(name);
}

/** Initialize user module */
void user_init(void)
{
    nrf_gpio_cfg_output(PIN_BUZZER);
    nrf_gpio_pin_clear(PIN_BUZZER);

    nrf_gpio_cfg_output(PIN_VIBRATOR);
    nrf_gpio_pin_clear(PIN_VIBRATOR);
}

#define PWM_FREQ 4000

/** Initialize PWM */
void pwm_init(void)
{

    ret_code_t err_code;

    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH((1000000UL / PWM_FREQ), PIN_BUZZER);
    pwm1_cfg.pin_polarity[1]  = APP_PWM_POLARITY_ACTIVE_HIGH;

    err_code = app_pwm_init(&PWM1, &pwm1_cfg, NULL);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM1);

    nrf_gpio_cfg_output(PIN_VIBRATOR);
    nrf_gpio_pin_clear(PIN_VIBRATOR);
}

/** Uninitialize PWM and turn off outputs */
void pwm_uninit(void)
{
    app_pwm_uninit(&PWM1);

    nrf_gpio_cfg_output(PIN_BUZZER);
    nrf_gpio_pin_clear(PIN_BUZZER);

    nrf_gpio_cfg_output(PIN_VIBRATOR);
    nrf_gpio_pin_clear(PIN_VIBRATOR);
}

/** Turn on PWM channel 0 */
void pwm_ch0_on(void)
{
    app_pwm_channel_duty_set(&PWM1, 0, 50);
}

/** Turn off PWM channel 0 */
void pwm_ch0_off(void)
{
    app_pwm_channel_duty_set(&PWM1, 0, 0);
}

/** Uninitialize SPI */
void uninit_spi(void)
{
    nrfx_gpiote_uninit();
}

/** Power off the system */
void power_off(void)
{
    spi_config(SPI_FLASH);
    flash_enter_sleep();
    spi_config(SPI_NOT_USE);

    spi_config(SPI_LCD);
    lcd_low_power();
    spi_config(SPI_NOT_USE);

    app_timer_stop_all();
    nrfx_gpiote_uninit();

    nrf_gpio_cfg_default(PIN_VIBRATOR);
    nrf_gpio_cfg_default(PIN_CO2_SDA);
    nrf_gpio_cfg_default(PIN_CO2_SCL);
    nrf_gpio_cfg_default(PIN_BUZZER);
    nrf_gpio_cfg_default(PIN_CHRG_STA);
    nrf_gpio_cfg_default(PIN_CO2_PWR);
    nrf_gpio_cfg_default(PIN_BAT_ADC);
    nrf_gpio_cfg_default(PIN_KEY);
    nrf_gpio_cfg_default(PIN_FLASH_CS);
    nrf_gpio_cfg_default(PIN_FLASH_MISO);
    nrf_gpio_cfg_default(PIN_FLASH_MOSI);
    nrf_gpio_cfg_default(PIN_FLASH_CLK);
    nrf_gpio_cfg_default(PIN_USB_IN);
    nrf_gpio_cfg_default(PIN_LCD_PWR);
    nrf_gpio_cfg_default(PIN_LCD_CS);
    nrf_gpio_cfg_default(PIN_LCD_CD);
    nrf_gpio_cfg_default(PIN_LCD_CLK);
    nrf_gpio_cfg_default(PIN_LCD_SDA);
    nrf_gpio_cfg_default(PIN_LCD_TE);
    nrf_gpio_cfg_default(PIN_LCD_BL);

    nrf_gpio_cfg_sense_input(PIN_USB_IN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_set(PIN_USB_IN, NRF_GPIO_PIN_SENSE_HIGH);

    nrf_gpio_cfg_sense_input(BUTTON_0, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

    sd_power_system_off();
}

/** Check if it is do not disturb time */
bool is_dnd_time(void)
{
    uint8_t dnd_mode = cfg_fstorage_get_dnd_mode();

    if (dnd_mode == 0 || !time_set)
    {
        return false;
    }

    uint8_t dnd_start_hour   = cfg_fstorage_get_dnd_start_hour();
    uint8_t dnd_start_minute = cfg_fstorage_get_dnd_start_minute();
    uint8_t dnd_end_hour     = cfg_fstorage_get_dnd_end_hour();
    uint8_t dnd_end_minute   = cfg_fstorage_get_dnd_end_minute();
    uint8_t current_hour     = get_time_now_hour24();
    uint8_t current_minute   = get_time_now_minute();

    if (dnd_start_hour < dnd_end_hour)
    {
        if (current_hour >= dnd_start_hour && current_hour < dnd_end_hour)
        {
            return true;
        }
    }
    else if (dnd_start_hour > dnd_end_hour)
    {
        if (current_hour >= dnd_start_hour || current_hour < dnd_end_hour)
        {
            return true;
        }
    }
    else
    {
        if (current_minute >= dnd_start_minute && current_minute < dnd_end_minute)
        {
            return true;
        }
    }

    return false;
}

void set_time_set(bool value)
{
    time_set = value;
}

bool get_time_set(void)
{
    return time_set;
}

void set_asc_check_duration(uint16_t duration)
{
    asc_check_duration = duration;
}

/** Update flight mode activation time */
void update_flight_mode_activation_time(void)
{
    if (cfg_fstorage_get_flight_mode())
    {
        flight_mode_activation_time = get_time_now();
        print("Flight mode activated at: %u\n", flight_mode_activation_time);
    }
    else
    {
        flight_mode_activation_time = 0; // Reset when flight mode is deactivated
        print("Flight mode deactivated.\n");
    }
}
