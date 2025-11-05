#ifndef __SPI_H__
#define __SPI_H__

#include "app_timer.h"
#include "custom_board.h"
#include "log.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"

typedef enum
{
    SPI_NOT_USE = 0,
    SPI_LCD,
    SPI_FLASH,
} spi_usage_t;

typedef void (*xfer_done_cb)(void);

/**
 * @brief Configure SPI interface
 *
 * @param usage SPI_NOT_USE to uninitialize, SPI_LCD to configure for LCD, SPI_FLASH to configure for flash
 */
void spi_config(spi_usage_t usage);

/**
 * @brief SPI read
 *
 * @param data Buffer to store read data
 * @param len Length of data to read
 * @param rx_cb Callback for read completion, NULL for blocking read
 * @param timeout_ms Timeout in milliseconds for blocking mode
 * @return uint8_t 1 if SPI is busy, 2 if SPI error, 3 if timeout, 0 if success
 */
uint8_t spi_read(uint8_t *data, uint8_t len, xfer_done_cb rx_cb, uint32_t timeout_ms);

/**
 * @brief SPI write
 *
 * @param data Buffer with data to send
 * @param len Length of data to send
 * @param tx_cb Callback for write completion, NULL for blocking write
 * @param timeout_ms Timeout in milliseconds for blocking mode
 * @return uint8_t 1 if SPI is busy, 2 if SPI error, 3 if timeout, 0 if success
 */
uint8_t spi_write(uint8_t *data, uint8_t len, xfer_done_cb tx_cb, uint32_t timeout_ms);

/**
 * @brief Get SPI usage status
 *
 * @return spi_usage_t Current SPI usage status
 */
spi_usage_t spi_get_usage(void);

/**
 * @brief Get SPI busy status
 *
 * @return bool True if SPI is busy, false otherwise
 */
bool spi_get_busy_state(void);

#endif // __SPI_H__
