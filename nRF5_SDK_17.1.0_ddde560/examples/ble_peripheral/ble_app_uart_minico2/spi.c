#include "spi.h"

#define SPI_INSTANCE 0
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);

static spi_usage_t           spi_usage  = SPI_NOT_USE;
static volatile bool         busy       = false;
static volatile xfer_done_cb tx_done_cb = NULL;
static volatile xfer_done_cb rx_done_cb = NULL;

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context);

/**
 * @brief Configure SPI interface
 *
 * @param usage SPI_NOT_USE to uninitialize, SPI_LCD to configure for LCD, SPI_FLASH to configure for flash
 */
void spi_config(spi_usage_t usage)
{
    if (spi_usage == usage) return;

    nrf_drv_spi_uninit(&spi);
    spi_usage                       = usage;
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

    if (usage == SPI_LCD)
    {
        spi_config.ss_pin       = PIN_LCD_CS;
        spi_config.miso_pin     = NRF_DRV_SPI_PIN_NOT_USED;
        spi_config.mosi_pin     = PIN_LCD_SDA;
        spi_config.sck_pin      = PIN_LCD_CLK;
        spi_config.frequency    = NRF_DRV_SPI_FREQ_8M;
        spi_config.mode         = NRF_DRV_SPI_MODE_2;
        spi_config.irq_priority = APP_IRQ_PRIORITY_HIGH;
    }
    else if (usage == SPI_FLASH)
    {
        spi_config.ss_pin       = NRF_DRV_SPI_PIN_NOT_USED;
        spi_config.miso_pin     = PIN_FLASH_MISO;
        spi_config.mosi_pin     = PIN_FLASH_MOSI;
        spi_config.sck_pin      = PIN_FLASH_CLK;
        spi_config.frequency    = NRF_DRV_SPI_FREQ_8M;
        spi_config.mode         = NRF_DRV_SPI_MODE_0;
        spi_config.irq_priority = APP_IRQ_PRIORITY_HIGH;
    }
    else
    {
        return;
    }

    busy = false;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
}

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context)
{
    if (p_event->type == NRF_DRV_SPI_EVENT_DONE)
    {
        busy = false;

        if (tx_done_cb != NULL)
        {
            tx_done_cb();
        }
        if (rx_done_cb != NULL)
        {
            rx_done_cb();
        }
    }
}

/**
 * @brief SPI read
 *
 * @param data Buffer to store read data
 * @param len Length of data to read
 * @param rx_cb Callback for read completion, NULL for blocking read
 * @param timeout_ms Timeout in milliseconds for blocking mode
 * @return uint8_t 1 if SPI is busy, 2 if SPI error, 3 if timeout, 0 if success
 */
uint8_t spi_read(uint8_t *data, uint8_t len, xfer_done_cb rx_cb, uint32_t timeout_ms)
{
    uint32_t tick_from;
    if (busy) return 1;

    tx_done_cb = NULL;
    rx_done_cb = rx_cb;
    busy       = true;
    tick_from  = app_timer_cnt_get();

    if (NRF_SUCCESS != nrf_drv_spi_transfer(&spi, NULL, 0, data, len))
    {
        busy       = false;
        rx_done_cb = NULL;
        return 2;
    }

    if (rx_cb == NULL) // Blocking read
    {
        while (busy && app_timer_cnt_diff_compute(app_timer_cnt_get(), tick_from) < APP_TIMER_TICKS(timeout_ms))
        {
            __WFE();
        }
        if (busy)
        {
            busy       = false;
            rx_done_cb = NULL;
            return 3;
        }
    }
    return 0;
}

/**
 * @brief SPI write
 *
 * @param data Buffer with data to send
 * @param len Length of data to send
 * @param tx_cb Callback for write completion, NULL for blocking write
 * @param timeout_ms Timeout in milliseconds for blocking mode
 * @return uint8_t 1 if SPI is busy, 2 if SPI error, 3 if timeout, 0 if success
 */
uint8_t spi_write(uint8_t *data, uint8_t len, xfer_done_cb tx_cb, uint32_t timeout_ms)
{
    uint32_t tick_from;
    if (busy) return 1;

    tx_done_cb = tx_cb;
    rx_done_cb = NULL;
    busy       = true;
    tick_from  = app_timer_cnt_get();

    if (NRF_SUCCESS != nrf_drv_spi_transfer(&spi, data, len, NULL, 0))
    {
        busy       = false;
        tx_done_cb = NULL;
        return 2;
    }

    if (tx_cb == NULL) // Blocking write
    {
        while (busy && app_timer_cnt_diff_compute(app_timer_cnt_get(), tick_from) < APP_TIMER_TICKS(timeout_ms))
        {
            __WFE();
        }
        if (busy)
        {
            busy       = false;
            tx_done_cb = NULL;
            return 3;
        }
    }
    return 0;
}

/** @brief Get SPI usage status */
spi_usage_t spi_get_usage(void)
{
    return spi_usage;
}

/** @brief Get SPI busy status */
bool spi_get_busy_state(void)
{
    return busy;
}
