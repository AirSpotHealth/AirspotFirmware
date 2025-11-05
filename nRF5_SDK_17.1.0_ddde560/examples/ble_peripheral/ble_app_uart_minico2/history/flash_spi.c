#include "flash_spi.h"

static zb25d16_handle_t zb_handle;

static uint8_t flash_spi_init(void)
{
    // Configure SPI for flash
    return 0;
}

static uint8_t flash_spi_deinit(void)
{
    // Deinitialize SPI configuration
    return 0;
}

static uint8_t flash_cs_gpio_init(void)
{
    print("init flash cs gpio\n");
    nrf_gpio_cfg_output(PIN_FLASH_CS);
    nrf_gpio_pin_set(PIN_FLASH_CS);
    return 0;
}

static uint8_t flash_cs_gpio_deinit(void)
{
    nrf_gpio_cfg_default(PIN_FLASH_CS);
    return 0;
}

static uint8_t flash_cs_gpio_write(uint8_t value)
{
    // Write to flash chip select GPIO pin
    if (value)
    {
        nrf_gpio_pin_set(PIN_FLASH_CS);
    }
    else
    {
        nrf_gpio_pin_clear(PIN_FLASH_CS);
    }
    // print("flash cs write %s\n", value ? "high" : "low");
    return 0;
}

void delay_ms(uint32_t ms)
{
    nrf_delay_ms(ms);
}

void zb25d16_debug_print(const char *const fmt, ...)
{
    // Define variable argument list
    va_list args;

    // Initialize variable argument list
    va_start(args, fmt);

    // Print log to SEGGER_RTT
    SEGGER_RTT_vprintf(0, fmt, &args);

    // End usage of variable arguments
    va_end(args);
}

/**
 * @brief SPI reads data into a buffer, default 10ms timeout
 *
 * @param buf Data buffer
 * @param len Length of data to read
 * @return uint8_t Returns 0 on success, 1 if SPI is busy, 2 if read fails, 3 if read times out
 */
uint8_t flash_spi_read(uint8_t *buf, uint8_t len)
{
    return spi_read(buf, len, NULL, 10);
}

/**
 * @brief SPI writes data from a buffer, default 10ms timeout
 *
 * @param buf Data buffer
 * @param len Length of data to write
 * @return uint8_t Returns 0 on success, 1 if SPI is busy, 2 if write fails, 3 if write times out
 */
uint8_t flash_spi_write(uint8_t *buf, uint8_t len)
{
    return spi_write(buf, len, NULL, 10);
}

/**
 * @brief Flash reads data from a specific address, byte-by-byte, default 10ms timeout
 *
 * @param addr Flash address to read from
 * @param buf Data buffer
 * @param len Length of data to read
 * @return uint8_t Returns 0 on success, 1 if SPI is busy, 2 if read fails, 3 if read times out
 */
uint8_t flash_read_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
    return zb25d16_read_data_fast(&zb_handle, addr, buf, len);
}

/**
 * @brief Flash writes data from a buffer to a specific address, default 10ms timeout
 *
 * @param addr Flash address to write to
 * @param buf Data buffer
 * @param len Length of data to write
 * @return uint8_t Returns 0 on success, 1 if SPI is busy, 2 if write fails, 3 if write times out
 */
uint8_t flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
    return zb25d16_single_page_write(&zb_handle, addr, buf, len);
}

/** Erase a data sector on the flash */
uint8_t flash_erase_data_sector(uint16_t sector)
{
    uint8_t ret;
    ret = zb25d16_erase_sector(&zb_handle, sector);
    return ret;
}

/** Erase the entire flash chip */
uint8_t flash_erase_chip(void)
{
    uint8_t ret;
    ret = zb25d16_erase_chip(&zb_handle);
    return ret;
}

/**
 * @brief Check flash busy status
 *
 * @return uint8_t Returns 0 if idle, 1 if busy, 2 if unable to get status
 */
uint8_t flash_get_busy_state(void)
{
    zb25d16_bool_t zb_state;
    if (0 == zb25d16_get_busy_state(&zb_handle, &zb_state))
    {
        return (uint8_t)zb_state;
    }
    return 2;
}

/**
 * @brief Put flash into low power mode
 *
 * @return uint8_t Returns 0 on success, 1 on failure
 */
uint8_t flash_enter_sleep(void)
{
    uint8_t ret;
    ret = zb25d16_power_down(&zb_handle);
    print("enter sleep %d\n", ret);
    return ret;
}

/**
 * @brief Wake flash up from low power mode
 *
 * @return uint8_t Returns 0 on success, 1 on failure
 */
uint8_t flash_exit_sleep(void)
{
    uint8_t ret;
    ret = zb25d16_power_up(&zb_handle);
    return ret;
}

/** Initialize flash driver */
uint8_t flash_driver_init(void)
{
    DRIVER_ZB25D16_LINK_INIT(&zb_handle, zb25d16_handle_t);
    DRIVER_ZB25D16_LINK_SPI_INIT(&zb_handle, flash_spi_init);
    DRIVER_ZB25D16_LINK_SPI_DEINIT(&zb_handle, flash_spi_deinit);
    DRIVER_ZB25D16_LINK_SPI_READ(&zb_handle, flash_spi_read);
    DRIVER_ZB25D16_LINK_SPI_WRITE(&zb_handle, flash_spi_write);
    DRIVER_ZB25D16_LINK_CS_GPIO_INIT(&zb_handle, flash_cs_gpio_init);
    DRIVER_ZB25D16_LINK_CS_GPIO_DEINIT(&zb_handle, flash_cs_gpio_deinit);
    DRIVER_ZB25D16_LINK_CS_GPIO_WRITE(&zb_handle, flash_cs_gpio_write);
    DRIVER_ZB25D16_LINK_DELAY_MS(&zb_handle, delay_ms);
    DRIVER_ZB25D16_LINK_DEBUG_PRINT(&zb_handle, zb25d16_debug_print);

    return zb25d16_init(&zb_handle);
}

/** Uninitialize flash driver */
uint8_t flash_driver_uninit(void)
{
    return zb25d16_deinit(&zb_handle);
}
