#include "button.h"

#define SCAN_INTERVAL_MS 10

// Button press durations
#define DOWN_MS            200
#define SHORTPRESS_MS      40
#define LONGPRESS_MS       2000
#define ULTRA_LONGPRESS_MS 5000

static uint8_t gpiote_init_flag   = 0;
static uint8_t button_scan_enable = 0; // Button scan enable flag

APP_TIMER_DEF(button_timer);           // Button scan timer
APP_TIMER_DEF(key_double_press_timer); // Double press detection timer

uint32_t button_pins[]                  = BUTTONS_LIST; // Button pins
uint32_t button_down_ms[BUTTONS_NUMBER] = {0};          // Button press durations

static uint8_t key_double_press_flag = 0; // Double press detection flag

extern void sleep_mode_enter(void);
extern void sleep_mode_exit(void);

void button_event_handler(void *p_event_data, uint16_t event_size);
void key_double_press_timer_handler(void *pcontext);

static uint8_t is_button_pin(nrfx_gpiote_pin_t pin)
{
    for (uint8_t i = 0; i < BUTTONS_NUMBER; i++)
    {
        if (pin == button_pins[i]) return 1;
    }
    return 0;
}

void button_timer_handler(void *pcontext)
{
#if BUTTONS_NUMBER <= 8
    static uint8_t button_down_bits = 0;
#elif BUTTONS_NUMBER <= 16
    static uint16_t button_down_bits = 0;
#else
    static uint32_t button_down_bits = 0;
#endif

    for (int i = 0; i < BUTTONS_NUMBER; i++)
    {
        if (0 == nrf_gpio_pin_read(button_pins[i])) // Active low
        {
            button_down_ms[i] += SCAN_INTERVAL_MS;
            if (button_down_ms[i] == SHORTPRESS_MS || (button_down_ms[i] != 0 && button_down_ms[i] % DOWN_MS == 0))
            {
                button_event_data_t evt_data = {.pin = button_pins[i], .evt = BTN_EVT_DOWN};
                app_sched_event_put(&evt_data, sizeof(button_event_data_t), button_event_handler);
            }
            if (button_down_ms[i] == LONGPRESS_MS)
            {
                button_event_data_t evt_data = {.pin = button_pins[i], .evt = BTN_EVT_PRESS_2S};
                app_sched_event_put(&evt_data, sizeof(button_event_data_t), button_event_handler);
            }
            if (button_down_ms[i] == ULTRA_LONGPRESS_MS)
            {
                button_event_data_t evt_data = {.pin = button_pins[i], .evt = BTN_EVT_PRESS_5S};
                app_sched_event_put(&evt_data, sizeof(button_event_data_t), button_event_handler);
            }
            button_down_bits |= (1 << i);
        }
        else
        {
            if (button_down_ms[i] >= SHORTPRESS_MS)
            {
                button_event_data_t evt_data = {.pin = button_pins[i], .evt = BTN_EVT_UP};
                app_sched_event_put(&evt_data, sizeof(button_event_data_t), button_event_handler);
            }
            if (button_down_ms[i] >= SHORTPRESS_MS && button_down_ms[i] < LONGPRESS_MS)
            {
                button_event_data_t evt_data = {.pin = button_pins[i], .evt = BTN_EVT_PRESS};
                app_sched_event_put(&evt_data, sizeof(button_event_data_t), button_event_handler);
            }
            button_down_ms[i] = 0;
            button_down_bits &= ~(1 << i);
        }
    }

    if (0 == button_down_bits)
    {
        app_timer_stop(button_timer);
        button_scan_enable = 0;
    }
}

void button_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (!is_button_pin(pin)) return;

    if (button_scan_enable == 0)
    {
        button_scan_enable = 1;
        app_timer_start(button_timer, APP_TIMER_TICKS(SCAN_INTERVAL_MS), NULL);
    }
}

void gpiote_init(void)
{
    if (!gpiote_init_flag)
    {
        gpiote_init_flag = 1;
        nrfx_gpiote_init();
    }
}

void button_init(void)
{
    ret_code_t                 errCode;
    nrf_drv_gpiote_in_config_t inConfig = {
        .sense           = NRF_GPIOTE_POLARITY_TOGGLE,
        .pull            = NRF_GPIO_PIN_PULLUP,
        .is_watcher      = false,
        .hi_accuracy     = false,
        .skip_gpio_setup = false,
    };

    gpiote_init();

    for (int i = 0; i < BUTTONS_NUMBER; i++)
    {
        nrf_gpio_cfg_input(button_pins[i], NRF_GPIO_PIN_PULLUP);

        errCode = nrf_drv_gpiote_in_init(button_pins[i], &inConfig, button_interrupt_handler);
        APP_ERROR_CHECK(errCode);
        nrf_drv_gpiote_in_event_enable(button_pins[i], true);
    }

    app_timer_create(&button_timer, APP_TIMER_MODE_REPEATED, button_timer_handler);
    app_timer_create(&key_double_press_timer, APP_TIMER_MODE_SINGLE_SHOT, key_double_press_timer_handler);
}

void button_stop(void)
{
    app_timer_stop(button_timer);
}

void button_sleep_mode_enter(void)
{
    nrfx_gpiote_uninit();
    nrf_gpio_cfg_sense_input(PIN_KEY, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    nrf_gpio_cfg_sense_set(PIN_KEY, NRF_GPIO_PIN_SENSE_LOW);
}

void button_down_handler(uint32_t pin);
void button_up_handler(uint32_t pin);
void button_press_handler(uint32_t pin);
void button_press_2s_handler(uint32_t pin);
void button_press_5s_handler(uint32_t pin);

void button_event_handler(void *p_event_data, uint16_t event_size)
{
    button_event_data_t *evt_data = (button_event_data_t *)p_event_data;

    switch (evt_data->evt)
    {
    case BTN_EVT_DOWN:
        button_down_handler(evt_data->pin);
        break;
    case BTN_EVT_UP:
        button_up_handler(evt_data->pin);
        break;
    case BTN_EVT_PRESS:
        button_press_handler(evt_data->pin);
        break;
    case BTN_EVT_PRESS_2S:
        button_press_2s_handler(evt_data->pin);
        break;
    case BTN_EVT_PRESS_5S:
        button_press_5s_handler(evt_data->pin);
        break;
    default:
        break;
    }
}

void button_down_handler(uint32_t pin)
{
    switch (pin)
    {
    case PIN_KEY:
        EventGroupSetBits(event_group_system, EVT_BTN_DOWN);
        break;
    default:
        break;
    }
}

void button_up_handler(uint32_t pin)
{
    switch (pin)
    {
    case PIN_KEY:
        EventGroupClearBits(event_group_system, EVT_BTN_DOWN);
        break;
    default:
        break;
    }
}

void button_press_handler(uint32_t pin)
{
    switch (pin)
    {
    case PIN_KEY:

        if (key_double_press_flag == 1)
        {
            key_double_press_flag = 0;
            app_timer_stop(key_double_press_timer);
            EventGroupSetBits(event_group_system, EVT_BTN_PRESS_2T);
        }
        else
        {
            key_double_press_flag = 1;
            app_timer_start(key_double_press_timer, APP_TIMER_TICKS(400), NULL);
        }
        break;
    default:
        break;
    }
}

void button_press_2s_handler(uint32_t pin)
{
    print("KEY 0 PRESS 2S\n");
    switch (pin)
    {
    case PIN_KEY:
        print("KEY 0 PRESS 2S\n");
        EventGroupSetBits(event_group_system, EVT_BTN_PRESS_2S);
        break;
    default:
        break;
    }
}

void button_press_5s_handler(uint32_t pin)
{
    switch (pin)
    {
    case PIN_KEY:
        // print("KEY 0 PRESS 5S\n");
        // NVIC_SystemReset();
        break;
    default:
        break;
    }
}

void key_double_press_timer_handler(void *pcontext)
{
    key_double_press_flag = 0;
    EventGroupSetBits(event_group_system, EVT_BTN_PRESS);
}
