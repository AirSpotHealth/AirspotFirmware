#ifndef __HISTORY_H__
#define __HISTORY_H__

#include "co2.h"
#include "flash_spi.h"
#include "list.h"
#include "log.h"
#include "protocol.h"
#include "spi.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "ttask.h"
#include "user.h"

#define FLASH_SIZE (2 * 1024 * 1024) // 2 MB

/** Sector size */
#define FLASH_SECTOR_SIZE (4 * 1024) // 4 KB

/** Number of storage sectors */
#define FLASH_SECTOR_COUNT (FLASH_SIZE / FLASH_SECTOR_SIZE)

/** Page size */
#define FLASH_PAGE_SIZE (256) // 256 bytes

/** Total number of pages */
#define FLASH_PAGE_COUNT (FLASH_SIZE / FLASH_PAGE_SIZE)

/** Total number of half pages */
#define FLASH_HALF_PAGE_COUNT (FLASH_PAGE_COUNT * 2)

/** Number of pages per sector */
#define FLASH_PAGE_OF_SECTOR (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)

/** Maximum number of records per page */
#define RECORDS_PER_PAGE (32)

/** Next sector */
#define FLASH_SECTOR_NEXT(sector) ((sector) + 1 == FLASH_SECTOR_COUNT ? 0 : ((sector) + 1))

/** Index record storage sector, only one sector is used */
#define FLASH_INDEX_SECTOR (0)

/** Data storage start sector */
#define FLASH_DATA_SECTOR_START (0)

/** Number of data storage sectors (16 Mbits - 1 sector) */
#define FLASH_DATA_SECTOR_NUM (16 * 1024 * 1024 / 8 / FLASH_SECTOR_SIZE - 1)

/** Starting address of a sector */
#define FLASH_ADDR_OF_SECTOR(sector) ((sector) * FLASH_SECTOR_SIZE)

/** Sector of a given address */
#define FLASH_SECTOR_OF_ADDR(addr) ((addr) / FLASH_SECTOR_SIZE)

/** Check if an address is the start of a sector */
#define FLASH_ADDR_IS_SECTOR_START(addr) ((addr) % FLASH_SECTOR_SIZE == 0)

#if defined(__CC_ARM)
#pragma anon_unions
#elif defined(__ICCARM__)
#endif

#define HHEAD        0x12345678
#define HISTORY_SIZE 256 // Page size for history records
#define HALF_PAGE    (HISTORY_SIZE / 2)

static const uint32_t SYNCED_TIME_THRESHOLD = 0x10000000;
static const uint32_t INVALID_TIMESTAMP_F   = 0xFFFFFFFF;
static const uint32_t INVALID_TIMESTAMP_0   = 0x00000000;

/// Record type enum for history
typedef enum
{
    RECORD_TYPE_CO2 = 0,
    RECORD_TYPE_BATTERY_LOW,
    RECORD_TYPE_CALIBRATION,
    RECORD_TYPE_SENSOR_ERROR,
    RECORD_TYPE_RESET_REASON,
    RECORD_TYPE_SENSOR_USER_FACTORY_RESET,
    RECORD_TYPE_CALIBRATION_CORRECTION,
    RECORD_TYPE_SENSOR_AUTO_CALIBRATION,
    RECORD_TYPE_INTEGRITY_ERROR,
    RECORD_TYPE_CALIB_TARGET,
    RECORD_TYPE_TIME_SET,
    RECORD_TYPE_ASC_ADJUSTMENT,
    RECORD_TYPE_ASC_LOWEST,
    RECORD_TYPE_CALIBRATION_CORRECTION_OLD,
    RECORD_TYPE_MANUAL_CALIB_START,
    RECORD_TYPE_CALIBRATION_TARGET_UPDATE,
    RECORD_TYPE_DFU,
    RECORD_TYPE_DFU_FAIL,
    RECORD_TYPE_CO2_RETRY,
    RECORD_TYPE_FLIGHT_MODE,
    RECORD_TYPE_CO2_SCALE_FACTOR,
} record_type_t;

/**
 * Fixed Length Record Structure
 */
typedef struct
{
    uint32_t timestamp;
    uint16_t value;
    uint8_t  type;
    uint8_t  reserved;
} record_t;

/* Size of the record*/
#define RECORD_SIZE   (sizeof(record_t))
#define ERASED_RECORD ((record_t){0xFFFFFFFF, 0, 0, 0})

/**
 * @brief
 * History Record Page
 * It is just a buffer to store the history records in a page
 * It is a wrapper for the raw buffer so that we can access the records as an array of records
 * or as a raw buffer for easy read/write to flash
 * The page size is 256 bytes
 * The record size is 8 bytes
 */
typedef union
{
    struct
    {
        record_t records[RECORDS_PER_PAGE];
    };
    uint8_t buf[HISTORY_SIZE];
} record_page_t;

typedef struct
{
    record_t records[RECORDS_PER_PAGE];
    uint8_t  count;
} record_buffer_page;

/**
 * Circular storage area, when the count is 0, the first area is equal to the last area,
 * the last area is always empty and ready to be written
 */
typedef struct
{
    uint16_t sector; // Sector
    uint8_t  page;   // Page
    uint8_t  count;  // Record count
} store_area_t;

/** Check if two areas are equal */
#define STORE_AREA_EQUALS(ptr_a, ptr_b) ((ptr_a)->sector == (ptr_b)->sector && (ptr_a)->page == (ptr_b)->page)

/** Get the address of a history area */
#define GET_HIS_ADDR(p_history_area) ((p_history_area)->sector * FLASH_SECTOR_SIZE + (p_history_area)->page * FLASH_PAGE_SIZE)

/** Macro to swap endianness of a 16-bit value */
#define SWAP_ENDIAN16(x) ((uint16_t)(((x) >> 8) | ((x) << 8)))

/** Macro to swap endianness of a 32-bit value */
#define SWAP_ENDIAN32(x) ((uint32_t)((((x) & 0x000000FF) << 24) | \
                                     (((x) & 0x0000FF00) << 8) |  \
                                     (((x) & 0x00FF0000) >> 8) |  \
                                     (((x) & 0xFF000000) >> 24)))

/** Structure for history data request */
// half_page_number: page number of the flash to read from
typedef struct
{
    uint16_t half_page_number;
} record_request_t;

/**
 * @brief Request history data
 *
 * @param half_page_number Page number of the flash to read from (half page) 0 - 16383
 */
void history_request(uint16_t half_page_number);

/**
 * @brief Sends a page of historical data over Bluetooth.
 *
 * @param records Pointer to the raw buffer containing the page data (8 bytes per record).
 * @param record_count Number of valid records in the page.
 * @param half_page_number Page number of the flash the data is from
 */
void send_history_data(const record_page_t *rec_page, uint8_t record_count, uint16_t half_page_number);

/*
 * Utility Functions
 */

/**
 * @brief Get the last valid record from the history page
 *
 * @param page The history page to get the last record from
 * @param check_unsynced Flag to check for unsynced timestamps (default: true)
 * @return record_t The last valid record; if no valid record is found, returns a default zeroed record
 */
record_t get_last_record(const record_page_t *page, bool check_unsynced);

/**
 * @brief Check if a page is valid
 * A page is valid if the first record is not 0xFFFFFFFF or 0
 * @param page The page to check
 * @return true if the page is valid, false otherwise
 */

bool is_page_valid(const record_page_t *page);

/**
 * @brief Check if a record is valid
 *
 * @param record The record to check
 * @return true if the record is valid, false otherwise
 */
bool is_valid_record(const record_t *record);

/**
 * @brief Get the record count from the history
 *
 * @param history The history to get the record count from
 * @return uint8_t The record count
 */
uint8_t get_record_count(const record_page_t *history);

/**
 * @brief Get the current half page number
 */
uint16_t get_current_half_page(void);

/**
 * @brief stores the record value and record type in the record buffer
 *
 * @param value The record value
 * @param type The record type
 */
void add_record(uint16_t value, record_type_t type);

/** Read Flash Data */
uint8_t flash_read_data_(uint32_t addr, uint8_t *buf, uint16_t len);

/** Write Flash Data With Verification */
uint8_t flash_write_data_(uint32_t addr, uint8_t *buf, uint16_t len);

#define BAR_COUNT 32 // Number of bars in the graph

#endif // __HISTORY_H__
