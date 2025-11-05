#include "cfg_fstorage.h"

cfg_fstorage_t cfg_fstorage = CFG_DEFAULT;

static const uint32_t structSize = sizeof(cfg_fstorage_t);

// Globals for wear-leveling
static int32_t current_slot = -1; // -1 means no config saved yet

static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = BOOTLOADER_ADDRESS;
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ? bootloader_addr : (code_sz * page_sz));
}

static void fstorage_evt_handler(nrf_fstorage_evt_t *p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        print("--> Event received: ERROR while executing an fstorage operation.\n");
        return;
    }

    switch (p_evt->id)
    {
    case NRF_FSTORAGE_EVT_WRITE_RESULT:
        print("--> Event received: wrote %d bytes at address 0x%x.\n",
              p_evt->len, p_evt->addr);
        break;

    case NRF_FSTORAGE_EVT_ERASE_RESULT:
        print("--> Event received: erased %d page from address 0x%x.\n",
              p_evt->len, p_evt->addr);
        break;

    default:
        break;
    }
}

void wait_for_flash_ready(nrf_fstorage_t const *p_fstorage)
{
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        extern nrf_drv_wdt_channel_id m_channel_id;
        nrf_drv_wdt_channel_feed(m_channel_id);
    }
}

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
    {
        .evt_handler = fstorage_evt_handler,
        .start_addr  = 0x3e000, // 4 KB
        .end_addr    = 0x3ffff,
};

void cfg_fstorage_init(void)
{
    ret_code_t          rc;
    nrf_fstorage_api_t *p_fs_api = &nrf_fstorage_sd;

    fstorage.end_addr = nrf5_flash_end_addr_get();
    // Use two pages (8 KB) near the end of flash for config
    fstorage.start_addr = fstorage.end_addr - 4 * 1024;

    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);
}

#define PAGE_SIZE 4096 // Increased from 4096 to 8192 (8KB)
#define NUM_SLOTS (PAGE_SIZE / (structSize))

static bool is_slot_empty(uint32_t addr)
{
    uint8_t buf[54];
    for (uint32_t offset = 0; offset < structSize; offset += sizeof(buf))
    {
        uint32_t to_read = (structSize - offset) < sizeof(buf) ? (structSize - offset) : sizeof(buf);
        nrf_fstorage_read(&fstorage, addr + offset, buf, to_read);
        for (uint32_t i = 0; i < to_read; i++)
        {
            if (buf[i] != 0xFF)
            {
                return false;
            }
        }
    }
    return true;
}

static void find_last_used_slot(int32_t *last_slot_out, cfg_fstorage_t *out_cfg)
{
    uint32_t       current_addr = fstorage.start_addr;
    cfg_fstorage_t temp;
    int32_t        last_used   = -1;
    bool           needs_erase = false;

    for (int slot = 0; slot < NUM_SLOTS; slot++)
    {
        // Read the full structSize
        ret_code_t rc = nrf_fstorage_read(&fstorage, current_addr, &temp, structSize);
        if (rc != NRF_SUCCESS)
        {
            print("Error: Failed to read slot %d at address 0x%x. Skipping...\n", slot, current_addr);
            current_addr += structSize;
            continue; // Skip this slot
        }

        // Check for empty slot
        if (is_slot_empty(current_addr))
        {
            break; // Stop searching; no further slots can be valid
        }

        // ** Check if this is an outdated version**
        if (temp.cfg_ver < CFG_VER)
        {
            print("Older config version detected! Erasing all slots...\n");
            needs_erase = true;
            break; // Stop checking further, we will erase everything
        }

        // Configuration is up-to-date
        memcpy(out_cfg, &temp, structSize);
        last_used = slot;
        current_addr += structSize;
    }

    if (needs_erase)
    {
        // ** ERASE THE WHOLE PAGE**
        cfg_fstorage_erase_all(0); // Pass `1` to indicate it was erased due to migration
        *last_slot_out = -1;       // No valid slot remains
    }
    else
    {
        *last_slot_out = last_used;
    }
}

void cfg_fstorage_load(void)
{
    cfg_fstorage_t temp;
    find_last_used_slot(&current_slot, &temp);

    if (current_slot == -1)
    {
        // No valid configuration found, use default
        cfg_fstorage = (cfg_fstorage_t)CFG_DEFAULT;
        cfg_fstorage_save();
        print("No valid configuration found. Using default.\n");
    }
    else
    {
        // Load the latest valid configuration
        memcpy(&cfg_fstorage, &temp, structSize);
        print("Loaded configuration from slot %d.\n", current_slot);
    }
}

void erase_page(void)
{
    // Erase two pages (8KB) instead of one page (4KB)
    ret_code_t rc = nrf_fstorage_erase(&fstorage, fstorage.start_addr, 1, NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
}

void save_config(void)
{
    ret_code_t rc;
    uint32_t   write_addr = fstorage.start_addr + (current_slot * structSize);

    print("Writing config to flash at slot %d (addr=0x%x)\n", current_slot, write_addr);

    rc = nrf_fstorage_write(&fstorage, write_addr, &cfg_fstorage, structSize, NULL);
    APP_ERROR_CHECK(rc);

    // Wait until the flash operation completes
    wait_for_flash_ready(&fstorage);
}

void cfg_fstorage_save(void)
{
    ret_code_t rc;

    // Update the version number to the current CFG_VER
    cfg_fstorage.cfg_ver = CFG_VER;

    // Move to the next slot in the circular buffer
    current_slot += 1;

    // Check if we need to erase the page BEFORE we try to write
    if (current_slot >= NUM_SLOTS)
    {
        print("Flash page full, erasing and starting over...\n");
        // Erase the page
        erase_page();
        // Reset to first slot
        current_slot = 0;
    }

    save_config();
}

void cfg_fstorage_erase_all(uint8_t erase_required)
{
    print("Erasing entire flash storage page...\n");

    erase_page();

    current_slot = 0;                           // After erase, no configuration exists
    cfg_fstorage = (cfg_fstorage_t)CFG_DEFAULT; // Reset to default
    cfg_fstorage_save();                        // Save the default configuration as the first entry

    cfg_fstorage.erase_required = erase_required;

    print("Erased all configurations from flash.\n");
}

void cfg_fstorage_set_device_name(char *name)
{
    strncpy(cfg_fstorage.device_name, name, sizeof(cfg_fstorage.device_name));
    cfg_fstorage_save();
}

char *cfg_fstorage_get_device_name(void)
{
    print("load device name: %s\n", cfg_fstorage.device_name);
    return cfg_fstorage.device_name;
}

void cfg_fstorage_set_vibrator_mode(uint8_t mode)
{
    cfg_fstorage.vibrator_mode = mode;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_vibrator_mode(void)
{
    return cfg_fstorage.vibrator_mode;
}

void cfg_fstorage_set_buzzer_mode(uint8_t mode)
{
    cfg_fstorage.buzzer_mode = mode;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_buzzer_mode(void)
{
    return cfg_fstorage.buzzer_mode;
}

void cfg_fstorage_set_bluetooth_enabled(uint8_t enabled)
{
    cfg_fstorage.bluetooth_enabled = enabled;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_bluetooth_enabled(void)
{
    print("cfg get bluetooth_enabled %d\n", cfg_fstorage.bluetooth_enabled);
    return cfg_fstorage.bluetooth_enabled;
}

void cfg_fstorage_set_auto_calibrate(uint8_t autoC)
{
    cfg_fstorage.auto_calibrate = autoC;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_auto_calibrate(void)
{
    return cfg_fstorage.auto_calibrate;
}

void cfg_fstorage_set_erase_required(uint8_t erase)
{
    cfg_fstorage.erase_required = erase;
}

uint8_t cfg_fstorage_get_erase_required(void)
{
    return cfg_fstorage.erase_required;
}

void cfg_fstorage_set_power_mode(uint8_t mode)
{
    cfg_fstorage.power_mode = mode;
    cfg_fstorage_save();
    print("cfg set power mode %d\n", cfg_fstorage.power_mode);
}

uint8_t cfg_fstorage_get_power_mode(void)
{
    return cfg_fstorage.power_mode;
}

void cfg_fstorage_set_dnd_mode(uint8_t mode)
{
    cfg_fstorage.dnd_mode = mode;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_dnd_mode(void)
{
    return cfg_fstorage.dnd_mode;
}

void cfg_fstorage_set_dnd_start_end_time(uint8_t start_hour, uint8_t start_minute, uint8_t end_hour, uint8_t end_minute)
{
    cfg_fstorage.dnd_start_hour   = start_hour;
    cfg_fstorage.dnd_start_minute = start_minute;
    cfg_fstorage.dnd_end_hour     = end_hour;
    cfg_fstorage.dnd_end_minute   = end_minute;
    cfg_fstorage.dnd_mode         = 1;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_dnd_start_hour(void)
{
    return cfg_fstorage.dnd_start_hour;
}

uint8_t cfg_fstorage_get_dnd_start_minute(void)
{
    return cfg_fstorage.dnd_start_minute;
}

uint8_t cfg_fstorage_get_dnd_end_hour(void)
{
    return cfg_fstorage.dnd_end_hour;
}

uint8_t cfg_fstorage_get_dnd_end_minute(void)
{
    return cfg_fstorage.dnd_end_minute;
}

int16_t cfg_fstorage_get_cc(void)
{
    return cfg_fstorage.cc;
}

void cfg_fstorage_set_cc(int16_t cc)
{
    cfg_fstorage.cc = cc;
    cfg_fstorage_save();
}

uint8_t *cfg_fstorage_dump(void)
{
    return (uint8_t *)&cfg_fstorage;
}

void cfg_fstorage_set_calib_target(uint16_t target)
{
    cfg_fstorage.calib_target = target;
    cfg_fstorage_save();
}

uint16_t cfg_fstorage_get_calib_target(void)
{
    return cfg_fstorage.calib_target;
}

void cfg_fstorage_set_ui_mode(uint8_t mode, uint16_t max_value, uint16_t min_value)
{
    cfg_fstorage.ui_mode = mode;

    if (mode == 0)
    {
        cfg_fstorage.graph_max_value = max_value;
        cfg_fstorage.graph_min_value = min_value;
    }
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_ui_mode(void)
{
    return cfg_fstorage.ui_mode;
}

uint16_t cfg_fstorage_get_graph_max_value(void)
{
    return cfg_fstorage.graph_max_value;
}

uint16_t cfg_fstorage_get_graph_min_value(void)
{
    return cfg_fstorage.graph_min_value;
}

uint16_t cfg_fstorage_get_graph_yellow_start(void)
{
    return cfg_fstorage.graph_yellow_start;
}

uint16_t cfg_fstorage_get_graph_red_start(void)
{
    return cfg_fstorage.graph_red_start;
}

void cfg_fstorage_set_graph_yellow_and_red_start(uint16_t yellow_start, uint16_t red_start)
{
    print("Updating the yellow and red start to: %d, %d\n", yellow_start, red_start);
    cfg_fstorage.graph_yellow_start = yellow_start;
    cfg_fstorage.graph_red_start    = red_start;
    cfg_fstorage_save();
}

// Implementation for advanced alarm settings

void cfg_fstorage_set_advanced_alarm(uint8_t index, const advanced_alarm_setting_t *alarm_setting)
{
    if (index < 10 && alarm_setting != NULL)
    {
        print("Setting advanced alarm index %d: co2 %d, repeats %d, enabled %d\n",
              index, alarm_setting->co2_level, alarm_setting->alarm_repeats, alarm_setting->notification_enabled);
        cfg_fstorage.advanced_alarms[index] = *alarm_setting;
        cfg_fstorage_save();
    }
}

advanced_alarm_setting_t cfg_fstorage_get_advanced_alarm(uint8_t index)
{
    if (index < 10)
    {
        return cfg_fstorage.advanced_alarms[index];
    }
    // Return a default/disabled alarm setting if index is out of bounds
    advanced_alarm_setting_t default_alarm = {0, 0, 0};
    return default_alarm;
}

void cfg_fstorage_get_all_advanced_alarms(advanced_alarm_setting_t *alarms_array_ptr)
{
    if (alarms_array_ptr != NULL)
    {
        memcpy(alarms_array_ptr, cfg_fstorage.advanced_alarms, sizeof(cfg_fstorage.advanced_alarms));
    }
}

void cfg_fstorage_set_illuminate_screen_on_alarm(uint8_t enabled)
{
    cfg_fstorage.illuminate_screen_on_alarm = enabled;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_illuminate_screen_on_alarm(void)
{
    return cfg_fstorage.illuminate_screen_on_alarm;
}

void cfg_fstorage_reset_advanced_alarms_to_default(void)
{
    const cfg_fstorage_t defaults = CFG_DEFAULT; // This uses the macro from the .h file
    memcpy(cfg_fstorage.advanced_alarms, defaults.advanced_alarms, sizeof(defaults.advanced_alarms));
    cfg_fstorage.illuminate_screen_on_alarm = defaults.illuminate_screen_on_alarm;
    cfg_fstorage.alarm_co2_falling          = defaults.alarm_co2_falling;
    cfg_fstorage_save();
    print("Advanced alarm settings have been reset to defaults.\n");
}

void cfg_fstorage_set_alarm_co2_falling(uint8_t enabled)
{
    cfg_fstorage.alarm_co2_falling = enabled;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_alarm_co2_falling(void)
{
    return cfg_fstorage.alarm_co2_falling;
}

void cfg_fstorage_set_co2_scale_factor(float co2_scale_factor)
{
    cfg_fstorage.co2_scale_factor = co2_scale_factor;
    cfg_fstorage_save();
}

float cfg_fstorage_get_co2_scale_factor(void)
{
    return cfg_fstorage.co2_scale_factor;
}

void cfg_fstorage_set_flight_mode(uint8_t enabled)
{
    cfg_fstorage.flight_mode = enabled;
    cfg_fstorage_save();
}

uint8_t cfg_fstorage_get_flight_mode(void)
{
    return cfg_fstorage.flight_mode;
}
