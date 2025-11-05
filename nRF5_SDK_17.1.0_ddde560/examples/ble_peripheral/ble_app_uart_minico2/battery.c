#include "battery.h"

static uint16_t bat_vol_mv = 0;
static int16_t  bat_lvl    = 0;

#define BAT_PARTIAL_RE   ((2700 + 300) / 300)
#define RAW_TO_MV(value) (600 * (value) / 4096 * BAT_PARTIAL_RE)

#define BAT_VOL_FULL          (4100) // 4.1V
#define BAT_VOL_LOW           (3400) // 3.4V
#define LOW_BAT_VOL_THRESHOLD (3710) // 3.6V

extern void sleep_mode_enter(uint16_t bat_vol_mv);
extern void sleep_mode_exit(void);

extern void proto_send_battery_level(void);

static bool last_state_charging = 0;

// void battery_saadc_event_handler(nrfx_saadc_evt_t const *p_event);

/**
 * @brief 需要在转换后才能获得不为0的电量
 *
 * @return uint8_t 电量百分比 0~100
 */
uint8_t get_battery_level(void)
{
    return bat_lvl;
}

/**返回电池电压 */
uint16_t get_battery_voltage_mv(void)
{
    static nrf_saadc_value_t adc_raw  = 0;
    uint32_t                 err_code = nrfx_saadc_sample_convert(4, &adc_raw);

    // Calculate what it *should* be according to Vbatt/6 x 4096
    float batt_mv_calc = RAW_TO_MV(adc_raw); // your existing conversion

    return batt_mv_calc;
}

void clear_bat_low_flag(void)
{
    sleep_mode_exit();
    proto_send_battery_level();
}

void battery_state_update(void)
{
    bat_vol_mv = get_battery_voltage_mv();
    bat_lvl    = 100 * (bat_vol_mv - BAT_VOL_LOW) / (BAT_VOL_FULL - BAT_VOL_LOW);
    bat_lvl    = bat_lvl < 0 ? 0 : (bat_lvl > 100 ? 100 : bat_lvl);

    print("BATTERY VOLTAGE: %d\n", bat_vol_mv);

    if (!USB_IN())
    {
        if (bat_vol_mv < BAT_VOL_LOW && !EventGroupCheckBits(event_group_system, EVT_BAT_LOW))
        {
            print("BATTERY LOW, enter sleep mode \n");
            proto_send_battery_level();
            sleep_mode_enter(bat_vol_mv);
        }

        else if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW) && bat_vol_mv > LOW_BAT_VOL_THRESHOLD)
        {
            print("BATTERY CHARGED, clear bat low flag\n");
            clear_bat_low_flag();
        }
    }
}

void usb_in_pin_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{

    if (USB_IN())
    {
        print("usb_in_pin_interrupt_handler called, exit sleep mode\n");
        clear_bat_low_flag();

        EventGroupSetBits(event_group_system, EVT_CHARGING);
    }
}

/**
 * @brief 初始化ADC
 *
 */
void battery_init(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_saadc_init(NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t ch_cfg = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN4);
    ch_cfg.reference                  = NRF_SAADC_REFERENCE_INTERNAL;
    ch_cfg.gain                       = NRF_SAADC_GAIN1;
    ch_cfg.acq_time                   = NRF_SAADC_ACQTIME_40US;
    ch_cfg.mode                       = NRF_SAADC_MODE_SINGLE_ENDED;

    err_code = nrfx_saadc_channel_init(4, &ch_cfg);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_config_t inConfig = {
        .sense           = NRF_GPIOTE_POLARITY_LOTOHI,
        .pull            = NRF_GPIO_PIN_NOPULL,
        .is_watcher      = false,
        .hi_accuracy     = false,
        .skip_gpio_setup = false,
    };

    gpiote_init();
    nrf_gpio_cfg_input(PIN_USB_IN, NRF_GPIO_PIN_NOPULL);
    err_code = nrf_drv_gpiote_in_init(PIN_USB_IN, &inConfig, usb_in_pin_interrupt_handler);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(PIN_USB_IN, true);

    nrf_gpio_cfg_input(PIN_CHRG_STA, NRF_GPIO_PIN_PULLUP);
}

TaskDefine(task_batery)
{
    TTS
    {
        print("task_battery run\n");
        battery_init();

        TaskDelay(100 / TICK_RATE_MS); // 等待电压稳定

        while (1)
        {
            if (USB_IN())
            {
                bat_lvl = 0;
                while (USB_IN())
                {
                    last_state_charging = true;
                    if (IS_CHARGING())
                    {
                        bat_lvl += 10;
                        if (bat_lvl > 100)
                        {
                            bat_lvl = 0;
                        }
                        EventGroupSetBits(event_group_system, EVT_BAT_UPDATE);
                        TaskDelay(200 / TICK_RATE_MS);
                        // print("charging\n");
                    }
                    else
                    {
                        battery_state_update();
                        battery_level_update(get_battery_level());
                        proto_send_battery_level();
                        EventGroupSetBits(event_group_system, EVT_BAT_UPDATE);
                        print("charging done\n");
                        TaskWait(!USB_IN(), 1000 / TICK_RATE_MS);
                    }
                }
            }
            else
            {
                print("Not charging\n");

                TaskWait(USB_IN() || last_state_charging || EventGroupCheckBits(event_group_system, EVT_BATTER_ADC_EN), TICK_MAX);

                print("LastStateCharging: %d\n", last_state_charging);

                if (EventGroupCheckBits(event_group_system, EVT_BATTER_ADC_EN) || last_state_charging)
                {
                    if (last_state_charging)
                    {
                        last_state_charging = false;
                    }

                    if (EventGroupCheckBits(event_group_system, EVT_BATTER_ADC_EN))
                    {
                        EventGroupClearBits(event_group_system, EVT_BATTER_ADC_EN);
                    }

                    battery_state_update();

                    if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW_WARNING | EVT_BAT_LOW)) continue;
                    battery_level_update(get_battery_level());
                    proto_send_battery_level();

                    EventGroupSetBits(event_group_system, EVT_BAT_UPDATE); // Update the battery level
                }
            }
        }
    }
    TTE
}
