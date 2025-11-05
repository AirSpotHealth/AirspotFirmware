#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "app_scheduler.h"
#include "app_timer.h"
#include "custom_board.h"
#include "log.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "ttask.h"
#include "user.h"

typedef enum
{
    BTN_EVT_NONE = 0,
    BTN_EVT_DOWN,
    BTN_EVT_UP,
    BTN_EVT_PRESS,
    BTN_EVT_PRESS_2S,
    BTN_EVT_PRESS_5S,
} button_event_t;

typedef struct
{
    uint32_t       pin; // Pin number
    button_event_t evt; // Event type
} button_event_data_t;

/**
 * @brief Initialize GPIOTE module
 */
void gpiote_init(void);

/**
 * @brief Initialize buttons
 */
void button_init(void);

/**
 * @brief GPIOTE event callback for button interrupts
 *
 * @param pin Pin number
 * @param action GPIOTE polarity action
 */
void button_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

/**
 * @brief Prepare button module for sleep mode
 */
void button_sleep_mode_enter(void);

#endif // __BUTTON_H__