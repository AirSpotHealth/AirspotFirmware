#ifndef __FLASH_SPI_H__
#define __FLASH_SPI_H__

#include "app_timer.h"
#include "custom_board.h"
#include "log.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "spi.h"
#include "stdint.h"
#include "zb25d16.h"

/** Flash reads sampling data from a specified address, byte-by-byte */
uint8_t flash_read_data(uint32_t addr, uint8_t *buf, uint16_t len);

/** Flash writes sampling data to a specified address, byte-by-byte */
uint8_t flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len);

/** Flash erases a single data sector */
uint8_t flash_erase_data_sector(uint16_t sector);

/** Flash erases the entire chip */
uint8_t flash_erase_chip(void);

/**
 * @brief Checks the busy status of the flash
 *
 * @return uint8_t Returns 0 if idle, 1 if busy, 2 if status retrieval fails
 */
uint8_t flash_get_busy_state(void);

/**
 * @brief Puts flash into low power mode
 *
 * @return uint8_t Returns 0 on success, 1 on failure
 */
uint8_t flash_enter_sleep(void);

/**
 * @brief Wakes flash up from low power mode
 *
 * @return uint8_t Returns 0 on success, 1 on failure
 */
uint8_t flash_exit_sleep(void);

/** Initializes the flash interface */
uint8_t flash_driver_init(void);

/** Uninitializes the flash interface */
uint8_t flash_driver_uninit(void);

#endif
