#ifndef __ZB25D16_H__
#define __ZB25D16_H__

#include "stdint.h"
#include "string.h"



typedef struct zb25d16_s
{
    uint8_t (*cs_gpio_init)(void);                    /**< point to a gpio_init function address */
    uint8_t (*cs_gpio_deinit)(void);                  /**< point to a gpio_deinit function address */
    uint8_t (*cs_write)(uint8_t value);               /**< point to a gpio_write function address */
    uint8_t (*spi_init)(void);                        /**< point to a spi_init function address */
    uint8_t (*spi_deinit)(void);                      /**< point to a spi_deinit function address */
    uint8_t (*spi_read)(uint8_t *buf, uint8_t len);   /**< point to a spi_read function address */
    uint8_t (*spi_write)(uint8_t *buf, uint8_t len);  /**< point to a spi_write function address */
    void (*delay_ms)(uint32_t ms);                    /**< point to a delay function address */
    void (*debug_print)(const char *const fmt, ...);  /**< point to a debug_print function address */
    uint8_t inited;                                   /**< inited flag */
} zb25d16_handle_t;

typedef enum
{
    ZB25D16_BOOL_FALSE=0,
    ZB25D16_BOOL_TRUE,
}zb25d16_bool_t;

/**
 * @brief     initialize zb25d16_handle_t structure
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] STRUCTURE is zb25d16_handle_t
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_INIT(HANDLE, STRUCTURE) memset(HANDLE, 0, sizeof(STRUCTURE))

/**
 * @brief     link spi_init function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a spi_init function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_SPI_INIT(HANDLE, FUC) (HANDLE)->spi_init = FUC

/**
 * @brief     link spi_deinit function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a spi_deinit function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_SPI_DEINIT(HANDLE, FUC) (HANDLE)->spi_deinit = FUC

/**
 * @brief     link spi_read function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a spi_read function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_SPI_READ(HANDLE, FUC) (HANDLE)->spi_read = FUC

/**
 * @brief     link gpio_write function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_write function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_SPI_WRITE(HANDLE, FUC) (HANDLE)->spi_write = FUC

/**
 * @brief     link gpio_init function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_init function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_CS_GPIO_INIT(HANDLE, FUC) (HANDLE)->cs_gpio_init = FUC

/**
 * @brief     link gpio_deinit function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_deinit function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_CS_GPIO_DEINIT(HANDLE, FUC) (HANDLE)->cs_gpio_deinit = FUC

/**
 * @brief     link gpio_write function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_write function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_CS_GPIO_WRITE(HANDLE, FUC) (HANDLE)->cs_write = FUC

/**
 * @brief     link delay function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_write function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_DELAY_MS(HANDLE, FUC) (HANDLE)->delay_ms = FUC

/**
 * @brief     link debug print function
 * @param[in] HANDLE points to an zb25d16 handle structure
 * @param[in] FUC points to a gpio_write function address
 * @note      none
 */
#define DRIVER_ZB25D16_LINK_DEBUG_PRINT(HANDLE, FUC) (HANDLE)->debug_print = FUC

/**
 * @brief 初始化
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_init(zb25d16_handle_t *handle);

/**
 * @brief 反初始化
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_deinit(zb25d16_handle_t *handle);

/**
 * @brief 获取繁忙状态
 *
 * @param handle zb25d16句柄
 * @param state 繁忙状态
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_get_busy_state(zb25d16_handle_t *handle, zb25d16_bool_t *state);

/**
 * @brief 获取写使能状态
 *
 * @param handle zb25d16句柄
 * @param state 写使能状态
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_get_write_enable_state(zb25d16_handle_t *handle, zb25d16_bool_t *state);

/**
 * @brief 读出数据到缓存
 *
 * @param handle zb25d16句柄
 * @param address flash地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_read_data(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len);

/**
 * @brief 快速读出数据到缓存
 *
 * @param handle zb25d16句柄
 * @param address flash地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_read_data_fast(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len);

/**
 * @brief 单页写，超出页尾的数据将被抛弃
 *
 * @param handle zb25d16句柄
 * @param address 地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_single_page_write(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len);

/**
 * @brief 多页写，超出flash寻址范围的数据将被抛弃
 *
 * @param handle zb25d16句柄
 * @param address 地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_multi_page_write(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len);

/**
 * @brief 格式化扇区
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 8191
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_sector(zb25d16_handle_t *handle, uint16_t sector_number);

/**
 * @brief 格式化块
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 31
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_block(zb25d16_handle_t *handle, uint16_t block_number);

/**
 * @brief 格式化半块
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 63
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_half_block(zb25d16_handle_t *handle, uint16_t half_block_number);

/**
 * @brief 格式化芯片
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_chip(zb25d16_handle_t *handle);

/**
 * @brief 进入低功耗
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_power_down(zb25d16_handle_t *handle);

/**
 * @brief 取消低功耗
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_power_up(zb25d16_handle_t *handle);

#endif
