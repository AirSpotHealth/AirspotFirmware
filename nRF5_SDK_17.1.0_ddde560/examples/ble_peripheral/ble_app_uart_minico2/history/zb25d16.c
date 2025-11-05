#include "zb25d16.h"

#define PAGE_SIZE       (256)
#define SECTOR_SIZE     (4 * 1024)
#define FLASH_SIZE      (2 * 1024 * 1024)
#define BLOCK_SIZE      (64 * 1024)
#define HALF_BLOCK_SIZE (BLOCK_SIZE / 2)

/**
 * @brief instruction definition
 *
 */
#define CMD_WRITE_ENABLE          0x06
#define CMD_WRITE_DISABLE         0x04
#define CMD_READ_STATUS_REGISTER  0x05
#define CMD_WRITE_STATUS_REGISTER 0x01
#define CMD_READ_DATA             0x03
#define CMD_FAST_READ             0x0b
#define CMD_FAST_READ_DUAL_OUTPUT 0x3b
#define CMD_PAGE_PROGRAM          0x02
#define CMD_BLOCK_ERASE_64K       0xd8
#define CMD_HALF_BLOCK_ERASE_32K  0x52
#define CMD_SECTOR_ERASE          0x20
#define CMD_CHIP_ERASE            0xc7
#define CMD_POWER_DOWN            0xb9
#define CMD_RELEASE_POWER_DOWN    0xab
#define CMD_MANUFACTURER          0x90
#define CMD_JEDEC_ID              0x9f

#define CHECK_FUNC(func_call)         \
    do                                \
    {                                 \
        if (0 != func_call) return 1; \
    } while (0);

#define CHIP_SEL()   handle->cs_write(0)
#define CHIP_UNSEL() handle->cs_write(1)

/**
 * @brief status register bits
 */
#define STA_BIT_BUSY (1 << 0)
#define STA_BIT_WEL  (1 << 1)

/**
 * @brief 初始化
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_init(zb25d16_handle_t *handle)
{
    if (handle == NULL) /* check handle */
    {
        return 2; /* return error */
    }
    if (handle->debug_print == NULL) /* check debug_print */
    {
        return 3; /* return error */
    }
    if (handle->spi_init == NULL) /* check spi_init */
    {
        handle->debug_print(
            "nrf24l01: spi_init is null.\n"); /* spi_init is null */

        return 3; /* return error */
    }
    if (handle->spi_deinit == NULL) /* check spi_deinit */
    {
        handle->debug_print(
            "nrf24l01: spi_deinit is null.\n"); /* spi_deinit is null */

        return 3; /* return error */
    }
    if (handle->spi_read == NULL) /* check spi_read */
    {
        handle->debug_print(
            "nrf24l01: spi_read is null.\n"); /* spi_read is null */

        return 3; /* return error */
    }
    if (handle->spi_write == NULL) /* check spi_write */
    {
        handle->debug_print(
            "nrf24l01: spi_write is null.\n"); /* spi_write is null */

        return 3; /* return error */
    }
    if (handle->cs_gpio_init == NULL) /* check cs_gpio_init */
    {
        handle->debug_print(
            "nrf24l01: cs_gpio_init is null.\n"); /* cs_gpio_init is null */

        return 3; /* return error */
    }
    if (handle->cs_gpio_deinit == NULL) /* check cs_gpio_deinit */
    {
        handle->debug_print(
            "nrf24l01: cs_gpio_deinit is null.\n"); /* cs_gpio_deinit is null */

        return 3; /* return error */
    }
    if (handle->cs_write == NULL) /* check cs_write */
    {
        handle->debug_print(
            "nrf24l01: cs_write is null.\n"); /* cs_write is null */

        return 3; /* return error */
    }
    if (handle->cs_gpio_init() != 0) /* cs gpio init */
    {
        handle->debug_print(
            "nrf24l01: gpio init failed.\n"); /* cs gpio init failed */

        return 4; /* return error */
    }
    if (handle->spi_init() != 0) /* spi init */
    {
        handle->debug_print(
            "nrf24l01: spi init failed.\n"); /* spi init failed */
        (void)handle->cs_gpio_deinit();      /* cs gpio deinit */

        return 1; /* return error */
    }

    handle->inited = 1; /* flag finish initialization */

    return 0; /* success return 0 */
}

/**
 * @brief 反初始化
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_deinit(zb25d16_handle_t *handle)
{
    uint8_t res;

    if (handle == NULL) /* check handle */
    {
        return 2; /* return error */
    }
    if (handle->inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = handle->cs_gpio_deinit(); /* cs gpio deinit */
    if (res != 0)                   /* check result */
    {
        handle->debug_print(
            "nrf24l01: gpio deinit failed.\n"); /* gpio deinit failed */

        return 4; /* return error */
    }
    res = handle->spi_deinit(); /* spi deinit */
    if (res != 0)               /* check result */
    {
        handle->debug_print(
            "nrf24l01: spi deinit failed.\n"); /* spi deinit failed */

        return 5; /* return error */
    }
    handle->inited = 0; /* flag close */

    return 0; /* success return 0 */
}

static uint8_t write_enable(zb25d16_handle_t *handle)
{
    uint8_t buf[1] = {0};
    buf[0]         = CMD_WRITE_ENABLE;
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(buf, 1));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

static uint8_t write_disable(zb25d16_handle_t *handle)
{
    uint8_t buf[1] = {0};
    buf[0]         = CMD_WRITE_DISABLE;
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(buf, 1));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 获取繁忙状态
 *
 * @param handle zb25d16句柄
 * @param state 繁忙状态
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_get_busy_state(zb25d16_handle_t *handle, zb25d16_bool_t *state)
{
    uint8_t buf[2] = {0};
    buf[0]         = CMD_READ_STATUS_REGISTER;
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(buf, 1));
    CHECK_FUNC(handle->spi_read(buf + 1, 1));
    CHECK_FUNC(handle->cs_write(1));
    *state = buf[1] & STA_BIT_BUSY ? ZB25D16_BOOL_TRUE : ZB25D16_BOOL_FALSE;
    return 0;
}

/**
 * @brief 获取写使能状态
 *
 * @param handle zb25d16句柄
 * @param state 写使能状态
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_get_write_enable_state(zb25d16_handle_t *handle, zb25d16_bool_t *state)
{
    uint8_t buf[2] = {0};
    buf[0]         = CMD_READ_STATUS_REGISTER;
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(buf, 1));
    CHECK_FUNC(handle->spi_read(buf + 1, 1));
    CHECK_FUNC(handle->cs_write(1));
    *state = buf[1] & STA_BIT_WEL ? ZB25D16_BOOL_TRUE : ZB25D16_BOOL_FALSE;
    return 0;
}

/**
 * @brief 读出数据到缓存
 *
 * @param handle zb25d16句柄
 * @param address flash地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_read_data(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len)
{
    uint8_t _buf[4];
    _buf[0] = CMD_READ_DATA;
    _buf[1] = (uint8_t)(address >> 16);
    _buf[2] = (uint8_t)(address >> 8);
    _buf[3] = (uint8_t)(address >> 0);
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 4));
    CHECK_FUNC(handle->spi_read(buf, len));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 快速读出数据到缓存
 *
 * @param handle zb25d16句柄
 * @param address flash地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_read_data_fast(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len)
{
    uint8_t _buf[5];
    _buf[0] = CMD_FAST_READ;
    _buf[1] = (uint8_t)(address >> 16);
    _buf[2] = (uint8_t)(address >> 8);
    _buf[3] = (uint8_t)(address >> 0);
    _buf[4] = 0xFF;
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 5));
    CHECK_FUNC(handle->spi_read(buf, len));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 单页写，超出页尾的数据将被抛弃
 *
 * @param handle zb25d16句柄
 * @param address 地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_single_page_write(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len)
{
    uint32_t next_page_addr;
    uint8_t  write_len;
    uint8_t  _buf[4];
    _buf[0]        = CMD_PAGE_PROGRAM;
    _buf[1]        = (uint8_t)(address >> 16);
    _buf[2]        = (uint8_t)(address >> 8);
    _buf[3]        = (uint8_t)(address >> 0);
    next_page_addr = (address / PAGE_SIZE + 1) * PAGE_SIZE;
    if (address + len <= next_page_addr)
    {
        write_len = len;
    }
    else
    {
        write_len = next_page_addr - address;
    }
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 4));
    CHECK_FUNC(handle->spi_write(buf, write_len));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 多页写，超出flash寻址范围的数据将被抛弃
 *
 * @param handle zb25d16句柄
 * @param address 地址
 * @param buf 数据缓存
 * @param len 数据长度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_multi_page_write(zb25d16_handle_t *handle, uint32_t address, uint8_t *buf, uint16_t len)
{
    uint32_t write_addr;
    uint32_t write_len;
    uint32_t next_page_addr;
    write_addr = address;
    do
    {
        next_page_addr = (write_addr / PAGE_SIZE + 1) * PAGE_SIZE;
        if (address + len <= next_page_addr)
        {
            write_len = len;
        }
        else
        {
            write_len = next_page_addr - address;
        }
        CHECK_FUNC(zb25d16_single_page_write(handle, write_addr, buf, write_len));
        if (next_page_addr == FLASH_SIZE)
        {
            return 0;
        }
        write_addr = next_page_addr;
        *buf += write_len;
        len -= write_len;
    } while (len > 0);
    return 0;
}

/**
 * @brief 格式化扇区
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 8191
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_sector(zb25d16_handle_t *handle, uint16_t sector_number)
{
    uint32_t address;
    uint8_t  _buf[4];
    address = SECTOR_SIZE * sector_number;
    _buf[0] = CMD_SECTOR_ERASE;
    _buf[1] = (uint8_t)(address >> 16);
    _buf[2] = (uint8_t)(address >> 8);
    _buf[3] = (uint8_t)(address >> 0);
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 4));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 格式化块
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 31
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_block(zb25d16_handle_t *handle, uint16_t block_number)
{
    uint32_t address;
    uint8_t  _buf[4];
    address = BLOCK_SIZE * block_number;
    _buf[0] = CMD_BLOCK_ERASE_64K;
    _buf[1] = (uint8_t)(address >> 16);
    _buf[2] = (uint8_t)(address >> 8);
    _buf[3] = (uint8_t)(address >> 0);
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 4));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 格式化半块
 *
 * @param handle zb25d16句柄
 * @param sector_number 扇区编号 0 ~ 63
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_half_block(zb25d16_handle_t *handle, uint16_t half_block_number)
{
    uint32_t address;
    uint8_t  _buf[4];
    address = HALF_BLOCK_SIZE * half_block_number;
    _buf[0] = CMD_HALF_BLOCK_ERASE_32K;
    _buf[1] = (uint8_t)(address >> 16);
    _buf[2] = (uint8_t)(address >> 8);
    _buf[3] = (uint8_t)(address >> 0);
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 4));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 格式化芯片
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_erase_chip(zb25d16_handle_t *handle)
{
    uint8_t  _buf[1];
    _buf[0] = CMD_CHIP_ERASE;
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 1));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 进入低功耗
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_power_down(zb25d16_handle_t *handle)
{
    uint8_t  _buf[1];
    _buf[0] = CMD_POWER_DOWN;
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 1));
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}

/**
 * @brief 取消低功耗
 *
 * @param handle zb25d16句柄
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t zb25d16_power_up(zb25d16_handle_t *handle)
{
    uint8_t  _buf[1];
    _buf[0] = CMD_RELEASE_POWER_DOWN;
    CHECK_FUNC(write_enable(handle));
    CHECK_FUNC(handle->cs_write(0));
    CHECK_FUNC(handle->spi_write(_buf, 1));
    handle->delay_ms(1);
    CHECK_FUNC(handle->cs_write(1));
    return 0;
}
