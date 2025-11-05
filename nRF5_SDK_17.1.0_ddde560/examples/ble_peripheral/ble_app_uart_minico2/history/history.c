#include "history.h"

// Add the declaration at the top of the file after includes
extern void update_co2_history(uint16_t new_value);

typedef uint8_t (*flash_op_func)(uint32_t addr, uint8_t *buf, uint16_t len);

uint8_t sector_data_buffer[FLASH_SECTOR_SIZE] = {0};

// Define circular buffer structure
#define BUFFER_SIZE RECORDS_PER_PAGE
typedef struct
{
    record_t records[BUFFER_SIZE];
    uint8_t  write_index;
    uint8_t  read_index;
    uint8_t  count;
} record_circular_buffer_t;

static record_circular_buffer_t record_buffer = {0};

// Current record , page and buffer
record_t           cur_record      = {0};
record_page_t      rec_page        = {0};
record_buffer_page rec_buffer_page = {0};

/** Most recent flash record position */
static store_area_t cur_store_area = {0};

/** Flag to indicate if all records should be erased */
uint8_t history_erase_all = 0;

/** Integrity check timestamp */
static uint32_t integrity_check_timestamp = 0;

/**
 * @brief Get a storage area with an offset
 *
 * @param cur Pointer to the current storage area
 * @param offset Offset value (positive for next, negative for previous)
 * @return store_area_t Storage area after applying the offset
 */
/** Get a storage area with an offset */
store_area_t get_store_area_with_offset(store_area_t *cur, int16_t offset)
{
    store_area_t result = *cur;

    // Calculate the absolute position in the linear address space
    int32_t linear_position = (result.sector * FLASH_PAGE_OF_SECTOR) + result.page + offset;

    // Handle wrap-around using modulo operation
    linear_position = (linear_position % FLASH_PAGE_COUNT + FLASH_PAGE_COUNT) % FLASH_PAGE_COUNT;

    // Calculate the sector and page from the linear position
    result.sector = linear_position / FLASH_PAGE_OF_SECTOR;
    result.page   = linear_position % FLASH_PAGE_OF_SECTOR;
    result.count  = 0; // Reset the record count

    return result;
}

/**
 * @brief Get the next storage area
 *
 * @param cur Pointer to the current storage area
 * @return store_area_t Storage area after applying the offset
 */
store_area_t get_next_store_area(store_area_t *cur) { return get_store_area_with_offset(cur, 1); }

/**
 * @brief Get the previous storage area
 *
 * @param cur Pointer to the current storage area
 * @return store_area_t Storage area after applying the offset
 */
store_area_t get_prev_store_area(store_area_t *cur) { return get_store_area_with_offset(cur, -1); }

/** Read/Write Flash Data in Chunks */
uint8_t flash_access_data(uint32_t addr, uint8_t *buf, uint16_t len, flash_op_func operation)
{
    uint8_t  ret;
    uint16_t chunk_size = 128; // Ensure max SPI size
    uint16_t offset     = 0;

    while (len > 0)
    {
        uint16_t bytes_to_process = (len > chunk_size) ? chunk_size : len;

        // Perform read/write
        ret = operation(addr + offset, buf + offset, bytes_to_process);
        if (ret != 0)
        {
            printf("Flash operation failed at address: 0x%08X, ret=%d\n", addr + offset, ret);
            return ret; // Exit on failure
        }

        offset += bytes_to_process;
        len -= bytes_to_process;
    }

    return 0; // Success
}

/** Read Flash Data */
uint8_t flash_read_data_(uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t ret = flash_access_data(addr, buf, len, &flash_read_data);
    if (ret != 0)
    {
        printf("ERROR: Failed to read flash at addr 0x%08X (ret=%d)\n", addr, ret);
        return ret;
    }

    return 0; // Success
}

/** Write Flash Data With Verification */
uint8_t flash_write_data_(uint32_t addr, uint8_t *buf, uint16_t len)
{

    uint8_t ret = flash_access_data(addr, buf, len, &flash_write_data);
    if (ret != 0)
    {
        print("ERROR: Failed to write flash at addr 0x%08X (ret=%d)\n", addr, ret);
        return ret;
    }

    return 0; // Success
}

/** Acquire SPI */
#define ACQUIRE_SPI()                                       \
    do                                                      \
    {                                                       \
        TaskWait(spi_get_usage() == SPI_NOT_USE, TICK_MAX); \
        spi_config(SPI_FLASH);                              \
    } while (0)

/** Release SPI */
#define RELEASE_SPI()                                \
    do                                               \
    {                                                \
        TaskWait(!flash_get_busy_state(), TICK_MAX); \
        spi_config(SPI_NOT_USE);                     \
    } while (0)

TaskDefine(task_history_recover)
{
    static record_page_t rec_page;
    static uint32_t      latest_ts    = 0;
    static uint16_t      found_synced = 0;
    static uint32_t      timestamp    = 0;
    static store_area_t  area         = {0};
    static uint16_t      sector, page;
    static uint8_t       found_readings = 0;
    static store_area_t  read_area;
    extern uint16_t      co2_history[BAR_COUNT];

    TTS
    {
        print("Recover history\n");

        for (sector = 0; sector < FLASH_SECTOR_COUNT; sector++)
        {
            for (page = 0; page < FLASH_PAGE_OF_SECTOR; page++)
            {
                area.sector = sector;
                area.page   = page;

                ACQUIRE_SPI();
                flash_read_data_(GET_HIS_ADDR(&area), rec_page.buf, HISTORY_SIZE);
                RELEASE_SPI();

                if (!is_page_valid(&rec_page))
                {
                    continue;
                }

                timestamp = get_last_record(&rec_page, true).timestamp;

                // print("sector %d page %d, end_timestamp: %8X\n", area.sector, area.page, timestamp);

                if (timestamp < SYNCED_TIME_THRESHOLD || timestamp == INVALID_TIMESTAMP_F)
                {
                    continue;
                }

                found_synced++;

                if (timestamp > latest_ts)
                {
                    print("Updating latest_ts from %8X to %8X at sector %d, page %d\n",
                          latest_ts, timestamp, area.sector, area.page);

                    latest_ts            = timestamp;
                    cur_store_area       = area;
                    cur_store_area.count = get_record_count(&rec_page);
                }
            }
        }

        // Determine where to write next
        if (found_synced == 0)
        {
            cur_store_area.sector = 0;
            cur_store_area.page   = 0;
            cur_store_area.count  = 0;
        }
        else if (cur_store_area.count == RECORDS_PER_PAGE)
        {
            cur_store_area = get_next_store_area(&cur_store_area);
        }

        integrity_check_timestamp = rec_page.records[cur_store_area.count - 1].timestamp;

        print("Integrity check timestamp: %u\n", integrity_check_timestamp);

        print("Current Valid Record::: sector: %d, page: %d, record_count: %d\n", cur_store_area.sector, cur_store_area.page, cur_store_area.count);

        // Initialize CO2 history after finding current valid record
        found_readings = 0;
        read_area      = cur_store_area;

        // we can have one value already in the co2_history
        // if so assign it to old_value and now we will only need BAR_COUNT - 1 readings
        if (co2_history[0] != 0)
        {
            found_readings = 1;
        }

        // Read pages in reverse order until we have BAR_COUNT readings or hit a blank page

        while (found_readings < BAR_COUNT)
        {
            ACQUIRE_SPI();
            flash_read_data_(GET_HIS_ADDR(&read_area), rec_page.buf, HISTORY_SIZE);
            RELEASE_SPI();

            log_hex_dump("rec_page", rec_page.buf, HISTORY_SIZE);

            if (!is_page_valid(&rec_page))
            {
                break;
            }

            // Read records in reverse to preserve time order
            for (int i = RECORDS_PER_PAGE - 1 - found_readings; i >= 0; i--)
            {
                if (rec_page.records[i].type == RECORD_TYPE_CO2)
                {
                    print("CO2 value: %d\n", SWAP_ENDIAN16(rec_page.records[i].value));
                    co2_history[found_readings] = SWAP_ENDIAN16(rec_page.records[i].value);
                    found_readings++;

                    if (found_readings == BAR_COUNT)
                    {
                        break;
                    }
                }
            }

            read_area = get_prev_store_area(&read_area);

            if (STORE_AREA_EQUALS(&read_area, &cur_store_area))
            {
                break;
            }
        }

        // debug print the co2_history
        print("CO2 history: \n");
        for (int i = 0; i < BAR_COUNT; i++)
        {
            print("%d, ", co2_history[i]);
        }
        print("\n");

        print("Recovered %d CO2 readings from history\n", found_readings);
    }
    TTE
}

TaskDefine(task_history_storage)
{
    static uint32_t history_address   = 0;
    static uint8_t  flash_init_result = 0;
    static bool     erase_sector      = false;
    static record_t current_record    = {0};
    static uint32_t write_addr        = 0;

    TTS
    {
        flash_init_result = flash_driver_init();

        print("Task: History Storage started. Flash init result: %d\n", flash_init_result);

        // Recover history from flash
        print("recover history\n");
        TaskWaitSync(task_history_recover, TICK_MAX);
        print("recover history done\n");

        history_erase_all = cfg_fstorage_get_erase_required();

        // now clear the erase_required flag if it is set to 1
        // because this was one time operation when the firmware is updated.
        if (history_erase_all)
        {
            cfg_fstorage_set_erase_required(0);
            cfg_fstorage_save();
        }

        while (1)
        {
            TaskWait((EventGroupCheckBits(event_group_system, EVT_CO2_UP_HIS | EVT_BAT_LOW_WARNING) && !EventGroupCheckBits(event_group_system, EVT_REQUEST_HISTORY)) || history_erase_all, TICK_MAX);

            // if it is a battery low event, skip the history update
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW))
            {
                break; // Exit the loop and the task, it will be restarted when the battery is charged
            }

            if (history_erase_all)
            {
                ACQUIRE_SPI();
                flash_erase_chip();
                RELEASE_SPI();

                // clear the co2_history
                memset(co2_history, 0, sizeof(co2_history));
                history_erase_all     = 0;
                cur_store_area.sector = 0;
                cur_store_area.page   = 0;
                cur_store_area.count  = 0;
                NUS_TAKE();
                send_erase_done();
                NUS_GIVE();
                print("Erase all history done\n");
                continue;
            }

            // Process records from circular buffer
            while (record_buffer.count > 0)
            {
                // Get record from buffer
                current_record = record_buffer.records[record_buffer.read_index];

                // Calculate write address
                history_address = GET_HIS_ADDR(&cur_store_area);
                write_addr      = history_address + cur_store_area.count * RECORD_SIZE;

                print("Writing record at addr=%08X: timestamp=%u, type=%d, value=%d, buffer_count=%d\n",
                      write_addr,
                      current_record.timestamp,
                      current_record.type,
                      SWAP_ENDIAN16(current_record.value),
                      record_buffer.count);

                // Add the record to the page buffer
                rec_page.records[cur_store_area.count] = current_record;

                // Write record to flash
                ACQUIRE_SPI();
                flash_write_data_(write_addr, (uint8_t *)&current_record, RECORD_SIZE);
                RELEASE_SPI();

                integrity_check_timestamp = current_record.timestamp;

                // Update buffer state
                record_buffer.read_index = (record_buffer.read_index + 1) % BUFFER_SIZE;
                record_buffer.count--;

                print("Written record at sector %d, page %d, index %d, address %08X\n",
                      cur_store_area.sector, cur_store_area.page, cur_store_area.count, write_addr);

                // Increment the record count
                cur_store_area.count++;

                if (cur_store_area.count == RECORDS_PER_PAGE)
                {
                    if (cur_store_area.page == FLASH_PAGE_OF_SECTOR - 1)
                    {
                        print("Page full, moving to the next sector: sector %d, page %d\n",
                              cur_store_area.sector, cur_store_area.page);
                        erase_sector = true;
                    }
                    else
                    {
                        print("Page full, moving to the next page: sector %d, page %d\n",
                              cur_store_area.sector, cur_store_area.page);
                    }

                    // Page is full, move to the next page
                    cur_store_area = get_next_store_area(&cur_store_area);
                    rec_page       = (record_page_t){0}; // Clear the page buffer

                    print("New location: sector=%d, page=%d\n",
                          cur_store_area.sector, cur_store_area.page);

                    if (erase_sector)
                    {
                        // Erase the sector
                        ACQUIRE_SPI();
                        flash_erase_data_sector(cur_store_area.sector);
                        RELEASE_SPI();

                        erase_sector = false;
                    }

                    rec_page = (record_page_t){0}; // Clear the page buffer
                }
            }

            EventGroupClearBits(event_group_system, EVT_CO2_UP_HIS);
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW_WARNING))
            {
                EventGroupClearBits(event_group_system, EVT_BAT_LOW_WARNING);
                EventGroupSetBits(event_group_system, EVT_BAT_LOW);
            }
        }
    }

    print("END HISTORY STORAGE\n");
    flash_driver_uninit();
    TTE
}

TaskDefine(task_populate_fake_records)
{
    static record_t      fake_record      = {0};
    static record_page_t fake_record_page = {0};
    static uint16_t      sector;
    static uint32_t      write_address = 0;
    static uint32_t      timestamp     = 0;
    static uint16_t      min_value     = 400;
    static uint16_t      max_value     = 4000;
    static uint16_t      value         = 0;
    static uint16_t      step          = 100;
    static uint16_t      record_num    = 0;

    static uint16_t page_idx = 0;
    static uint8_t  rec_idx  = 0;

    TTS
    {
        TaskWait(EventGroupCheckBits(event_group_system, EVT_POPULATE_FAKE_DATA), TICK_MAX);
        print("Task: Populate Fake Records started.\n");

        // Step 1: Erase the flash
        print("Step 1: Erasing Flash sectors.\n");

        ACQUIRE_SPI();
        flash_erase_chip();
        RELEASE_SPI();

        // Step 2: Initialize variables for circular buffer testing
        print("Step 2: Initializing variables for circular buffer testing.\n");
        timestamp = get_time_now();

        // set the timestamp to current timestamp - 7 days (seconds) * 10,000 records
        timestamp -= 7 * 24 * 60 * 60;

        cur_store_area.sector = 8000 / FLASH_PAGE_OF_SECTOR;
        cur_store_area.page   = 8000 % FLASH_PAGE_OF_SECTOR;
        cur_store_area.count  = 0;

        print("Initial store area: sector=%d, page=%d\n",
              cur_store_area.sector, cur_store_area.page);

        print("Initial timestamp: %u\n", timestamp);
        print("Sawtooth pattern range: %u to %u, step: %u\n", min_value, max_value, step);

        // Step 3: Populate flash with fake records
        print("Step 3: Populating flash with fake records.\n");

        for (page_idx = 0; page_idx < 312; page_idx++) // 10,000/32 = 312 pages
        {
            print("Processing page %d\n", page_idx);

            // Fill the page with records
            for (rec_idx = 0; rec_idx < RECORDS_PER_PAGE; rec_idx++)
            {
                record_num                        = page_idx * RECORDS_PER_PAGE + rec_idx;
                value                             = min_value + ((record_num * step) % (max_value - min_value + step));
                fake_record.timestamp             = timestamp;
                fake_record.type                  = RECORD_TYPE_CO2;
                fake_record.value                 = SWAP_ENDIAN16(value);
                fake_record.reserved              = 0;
                fake_record_page.records[rec_idx] = fake_record;

                // increment the timestamp by 60 seconds for each record so that we have total of (7 days * 24 hours * 60 minutes * 60 seconds) / 60 = 10080 records
                timestamp += 60;
            }

            // Write the full page to flash
            write_address = GET_HIS_ADDR(&cur_store_area);

            ACQUIRE_SPI();
            flash_write_data_(write_address, (uint8_t *)&fake_record_page, HISTORY_SIZE);
            RELEASE_SPI();

            // verify the page is written correctly
            ACQUIRE_SPI();
            flash_read_data_(write_address, fake_record_page.buf, HISTORY_SIZE);
            TaskWait(!flash_get_busy_state(), TICK_MAX);
            RELEASE_SPI();

            print("\nPage %d written successfully.\n", page_idx);

            if (page_idx == 311)
            {
                print("Last page written. Exiting loop.\n");
            }
            else
            {
                // Move to the next storage area (circular logic handled by get_next_store_area)
                cur_store_area = get_next_store_area(&cur_store_area);
                print("Next storage area: sector=%d, page=%d\n", cur_store_area.sector, cur_store_area.page);
            }

            memset(&fake_record_page, 0, sizeof(record_page_t));
        }

        print("Step 3: Flash population complete. Total pages written: %d\n", 312);
        print("Task: Populate Fake Records completed.\n");
        send_populate_done();

        EventGroupClearBits(event_group_system, EVT_POPULATE_FAKE_DATA);
    }
    TTE
}

void set_store_area_from_page(store_area_t *store_area, uint16_t half_page_number)
{
    // Calculate the full page number by dividing the half-page number by 2
    uint16_t full_page_number = half_page_number / 2;

    // Determine the sector from the full page number
    store_area->sector = full_page_number / FLASH_PAGE_OF_SECTOR;

    // Determine the page within the sector
    store_area->page = full_page_number % FLASH_PAGE_OF_SECTOR;

    // Debug print for verification
    print("Converted half-page %d -> sector %d, page %d\n",
          half_page_number, store_area->sector, store_area->page);
}

record_request_t record_request = {0};

/**
 * @brief Request history data
 *
 * @param page_number Page number of the flash to read from
 */
void history_request(uint16_t half_page_number)
{
    record_request.half_page_number = half_page_number;

    print("History request: Page=%6X\n", half_page_number);
}

/**
 * @brief stores the record value and record type in the record buffer
 *
 * @param value The record value
 * @param type The record type
 */
void add_record(uint16_t value, record_type_t type)
{
    // Check if buffer is full
    if (record_buffer.count >= BUFFER_SIZE)
    {
        print("ERROR: Buffer full, cannot add record\n");
        return;
    }

    // Create new record
    record_t new_record  = {0};
    new_record.timestamp = get_time_now();
    new_record.type      = type;
    new_record.value     = SWAP_ENDIAN16(value);
    new_record.reserved  = 0;

    // Add to buffer at write index
    record_buffer.records[record_buffer.write_index] = new_record;

    // Update write index and count
    record_buffer.write_index = (record_buffer.write_index + 1) % BUFFER_SIZE;
    record_buffer.count++;

    print("Added record to buffer: index=%d, count=%d, value=%d\n",
          record_buffer.write_index > 0 ? record_buffer.write_index - 1 : BUFFER_SIZE - 1,
          record_buffer.count, value);

    // Update CO2 history if this is a CO2 record
    if (type == RECORD_TYPE_CO2)
    {
        update_co2_history(value);
    }
}

TaskDefine(task_history_upload)
{
    static record_page_t record_page = {0xFF};
    static store_area_t  tx_store_area;
    static uint32_t      read_address = 0;

    TTS
    {
        while (1)
        {
            TaskWait(EventGroupCheckBits(event_group_system, EVT_REQUEST_HISTORY), TICK_MAX);

            // Exit if battery is low
            if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING))
            {
                EventGroupClearBits(event_group_system, EVT_REQUEST_HISTORY);
                TaskDelay(1000 / TICK_RATE_MS);
                continue;
            }

            // if the page number is invalid, exit
            // the page number should be between 0 and FLASH_HALF_PAGE_COUNT i.e 0 to 16383
            if (record_request.half_page_number >= FLASH_HALF_PAGE_COUNT)
            {
                EventGroupClearBits(event_group_system, EVT_REQUEST_HISTORY);
                continue;
            }

            // Set the store area from the requested page number
            set_store_area_from_page(&tx_store_area, record_request.half_page_number);

            read_address = GET_HIS_ADDR(&tx_store_area);

            print("\nBASE ADDR: %08X\n", read_address);

            // if the requested page number is odd, read the second half of the page (second 128 bytes)
            // otherwise read the first half of the page (first 128 bytes)
            if (record_request.half_page_number % 2 != 0)
            {
                read_address += HISTORY_SIZE / 2;
            }

            print("Request history: sector %d, page %d, address %08X\n", tx_store_area.sector, tx_store_area.page, read_address);

            ACQUIRE_SPI();
            flash_read_data_(read_address, record_page.buf, HALF_PAGE);
            print("Read history at sector %d, page %d\n", tx_store_area.sector, tx_store_area.page);
            RELEASE_SPI();

            log_hex_dump("Read history: ", record_page.buf, HALF_PAGE);

            tx_store_area.count = get_record_count(&record_page);

            NUS_TAKE();
            send_history_data(&record_page, tx_store_area.count, record_request.half_page_number);
            EventGroupWaitBits(event_group_system, EVT_NUS_TX_RDY, TICK_MAX);
            EventGroupClearBits(event_group_system, EVT_NUS_TX_RDY);
            NUS_GIVE();

            // reset the record_page buffer
            record_page = (record_page_t){0xFF};

            EventGroupClearBits(event_group_system, EVT_REQUEST_HISTORY);
        }
        TTE
    }
}

/**
 * @brief Check if a record is valid
 *
 * @param record The record to check
 * @return true if the record is valid, false otherwise
 */
bool is_valid_record(const record_t *record)
{
    if (record == NULL)
    {
        return false;
    }

    // Only treat 0xFFFFFFFF as invalid; 0x00000000 is still a valid record
    return record->timestamp != INVALID_TIMESTAMP_F;
}

/**
 * @brief Get the last valid record from the history page
 *
 * @param page The history page to get the last record from
 * @param check_unsynced Flag to check for unsynced timestamps (default: true)
 * @return record_t The last valid record; if no valid record is found, returns a default zeroed record
 */
record_t get_last_record(const record_page_t *page, bool check_unsynced)
{
    record_t last_valid_record = {0}; // Default zeroed record (00)

    for (int i = RECORDS_PER_PAGE - 1; i >= 0; i--) // Traverse from last record
    {
        const record_t *record = &page->records[i];

        // Ignore completely erased records
        if (record->timestamp == INVALID_TIMESTAMP_F)
        {
            continue;
        }

        // If we don't want unsynced timestamps, return the first valid record
        if (!check_unsynced)
        {
            return *record;
        }

        // If timestamp is valid but unsynced, store it as the last unsynced timestamp
        if (record->timestamp < SYNCED_TIME_THRESHOLD && record->timestamp > 0)
        {
            if (last_valid_record.timestamp == 0)
            {
                last_valid_record = *record;
            }
            continue; // Keep searching for a synced timestamp
        }

        // If timestamp is synced, return it immediately
        if (record->timestamp > SYNCED_TIME_THRESHOLD)
        {
            return *record;
        }
    }

    // If no synced timestamp was found, return the last unsynced record
    return last_valid_record;
}

/**
 * @brief Get the record count from the history
 *
 * @param record_page The record page to get the record count from
 * @return uint8_t The record count
 */
uint8_t get_record_count(const record_page_t *record_page)
{
    uint8_t         count  = 0;
    const record_t *record = NULL;

    for (int i = 0; i < RECORDS_PER_PAGE; i++)
    {
        record = &record_page->records[i];

        // Only stop at `0xFFFFFFFF`, but not at `0x00000000`
        if (record->timestamp == INVALID_TIMESTAMP_F)
        {
            break; // Stop counting at the first fully erased record
        }

        count++; // Count all valid timestamps, including `0x00000000`
    }
    return count;
}

/**
 * @brief Check if a page is valid
 * A page is valid if the first record is not 0xFFFFFFFF or 0
 * @param page The page to check
 * @return true if the page is valid, false otherwise
 */
bool is_page_valid(const record_page_t *page) { return is_valid_record(&page->records[0]); }

/**
 * Get the cuurent half page number
 */
uint16_t get_current_half_page(void)
{
    return (cur_store_area.sector * FLASH_PAGE_OF_SECTOR + cur_store_area.page) * 2 + (cur_store_area.count >= RECORDS_PER_PAGE / 2 ? 1 : 0);
}
