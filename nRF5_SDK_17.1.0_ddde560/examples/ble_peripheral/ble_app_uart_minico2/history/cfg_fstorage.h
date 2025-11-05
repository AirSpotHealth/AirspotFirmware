#ifndef __CFG_FSTORAGE_H__
#define __CFG_FSTORAGE_H__

#include "app_util.h"
#include "log.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "stdlib.h"
#include "string.h"

#include "nrf_drv_wdt.h"
#include "nrfx_wdt.h"

// Fstorage version number
#define CFG_VER 0x21

// New struct for advanced alarm settings
typedef struct __attribute__((aligned(4)))
{
    uint16_t co2_level;            // 2 bytes - CO2 threshold for this alarm
    uint8_t  alarm_repeats;        // 1 byte  - How many times the alarm should sound/vibrate
    uint8_t  notification_enabled; // 1 byte  - 0 = disabled, 1 = enabled (Now 4-byte aligned)
} advanced_alarm_setting_t;

typedef struct __attribute__((aligned(4)))
{
    uint32_t cfg_ver; // 4 bytes (Aligned)

    int16_t cc;             // 2 bytes
    uint8_t erase_required; // 1 byte
    uint8_t power_mode;     // 1 byte  (Now 4-byte aligned)

    uint8_t vibrator_mode;     // 1 byte
    uint8_t buzzer_mode;       // 1 byte
    uint8_t auto_calibrate;    // 1 byte
    uint8_t bluetooth_enabled; // 1 byte (Now 4-byte aligned)

    uint8_t dnd_mode;         // 1 byte
    uint8_t dnd_start_hour;   // 1 byte
    uint8_t dnd_start_minute; // 1 byte
    uint8_t dnd_end_hour;     // 1 byte (Now 4-byte aligned)

    uint8_t  dnd_end_minute; // 1 byte
    uint16_t calib_target;   // 2 bytes
    uint8_t  ui_mode;        // 1 byte  (Now 4-byte aligned)

    uint16_t graph_max_value; // 2 bytes
    uint16_t graph_min_value; // 2 bytes (Now 4-byte aligned)

    uint16_t graph_yellow_start; // 2 bytes
    uint16_t graph_red_start;    // 2 bytes (Now 4-byte aligned)

    char    device_name[21];            // 21     bytes (Changed from 22 to make total struct size a multiple of 4)
    uint8_t illuminate_screen_on_alarm; // 1 byte
    uint8_t alarm_co2_falling;          // 1 byte
    uint8_t flight_mode;                // 1 byte

    // New fields for advanced alarms
    advanced_alarm_setting_t advanced_alarms[10]; // 10 * 4 = 40 bytes (Now 4-byte aligned)

    // A scaling factor defined by the user to adjust the CO2 reading with range from .2 to 2.0, default is 1.0
    float co2_scale_factor; // 4 bytes
} cfg_fstorage_t;

#define CFG_DEFAULT                              \
    {                                            \
        .cfg_ver                    = CFG_VER,   \
        .device_name                = "Airspot", \
        .vibrator_mode              = 0,         \
        .buzzer_mode                = 0,         \
        .auto_calibrate             = 0,         \
        .bluetooth_enabled          = 1,         \
        .erase_required             = 1,         \
        .power_mode                 = 1,         \
        .dnd_mode                   = 0,         \
        .dnd_start_hour             = 0,         \
        .dnd_start_minute           = 0,         \
        .dnd_end_hour               = 0,         \
        .dnd_end_minute             = 0,         \
        .cc                         = 0,         \
        .calib_target               = 426,       \
        .ui_mode                    = 0,         \
        .graph_max_value            = 1600,      \
        .graph_min_value            = 0,         \
        .graph_yellow_start         = 800,       \
        .graph_red_start            = 1000,      \
        .illuminate_screen_on_alarm = 1,         \
        .alarm_co2_falling          = 0,         \
        .flight_mode                = 0,         \
        .co2_scale_factor           = 1.0,       \
        .advanced_alarms            = {          \
            {800, 1, 1},              \
            {1000, 2, 1},             \
            {1200, 3, 1},             \
            {1500, 5, 1},             \
            {0, 0, 0},                \
            {0, 0, 0},                \
            {0, 0, 0},                \
            {0, 0, 0},                \
            {0, 0, 0},                \
            {0, 0, 0},                \
        },                            \
    }
;

void     cfg_fstorage_init(void);
void     cfg_fstorage_save(void);
void     cfg_fstorage_load(void);
void     cfg_fstorage_set_device_name(char *name);
char    *cfg_fstorage_get_device_name(void);
void     cfg_fstorage_set_vibrator_mode(uint8_t mode);
uint8_t  cfg_fstorage_get_vibrator_mode(void);
void     cfg_fstorage_set_buzzer_mode(uint8_t mode);
uint8_t  cfg_fstorage_get_buzzer_mode(void);
void     cfg_fstorage_set_bluetooth_enabled(uint8_t enabled);
uint8_t  cfg_fstorage_get_bluetooth_enabled(void);
void     cfg_fstorage_set_auto_calibrate(uint8_t autoC);
uint8_t  cfg_fstorage_get_auto_calibrate(void);
void     cfg_fstorage_set_erase_required(uint8_t erase);
uint8_t  cfg_fstorage_get_erase_required(void);
void     cfg_fstorage_erase_all(uint8_t erase_required);
void     cfg_fstorage_set_power_mode(uint8_t mode);
uint8_t  cfg_fstorage_get_power_mode(void);
void     cfg_fstorage_set_dnd_mode(uint8_t mode);
uint8_t  cfg_fstorage_get_dnd_mode(void);
void     cfg_fstorage_set_dnd_start_end_time(uint8_t start_hour, uint8_t start_minute, uint8_t end_hour, uint8_t end_minute);
uint8_t  cfg_fstorage_get_dnd_start_hour(void);
uint8_t  cfg_fstorage_get_dnd_start_minute(void);
uint8_t  cfg_fstorage_get_dnd_end_hour(void);
uint8_t  cfg_fstorage_get_dnd_end_minute(void);
int16_t  cfg_fstorage_get_cc(void);
void     cfg_fstorage_set_cc(int16_t cc);
uint8_t *cfg_fstorage_dump(void);
void     cfg_fstorage_set_calib_target(uint16_t target);
uint16_t cfg_fstorage_get_calib_target(void);
void     cfg_fstorage_set_ui_mode(uint8_t mode, uint16_t max_value, uint16_t min_value);
uint8_t  cfg_fstorage_get_ui_mode(void);
uint16_t cfg_fstorage_get_graph_max_value(void);
uint16_t cfg_fstorage_get_graph_min_value(void);
uint16_t cfg_fstorage_get_graph_yellow_start(void);
uint16_t cfg_fstorage_get_graph_red_start(void);
void     cfg_fstorage_set_graph_yellow_and_red_start(uint16_t yellow_start, uint16_t red_start);

// New function declarations for advanced alarms
void                     cfg_fstorage_set_advanced_alarm(uint8_t index, const advanced_alarm_setting_t *alarm_setting);
advanced_alarm_setting_t cfg_fstorage_get_advanced_alarm(uint8_t index);
void                     cfg_fstorage_get_all_advanced_alarms(advanced_alarm_setting_t *alarms_array_ptr);
void                     cfg_fstorage_set_illuminate_screen_on_alarm(uint8_t enabled);
uint8_t                  cfg_fstorage_get_illuminate_screen_on_alarm(void);
void                     cfg_fstorage_reset_advanced_alarms_to_default(void); // For the reset button
void                     cfg_fstorage_set_alarm_co2_falling(uint8_t enabled);
uint8_t                  cfg_fstorage_get_alarm_co2_falling(void);

void  cfg_fstorage_set_co2_scale_factor(float co2_scale_factor);
float cfg_fstorage_get_co2_scale_factor(void);

void    cfg_fstorage_set_flight_mode(uint8_t enabled);
uint8_t cfg_fstorage_get_flight_mode(void);

#endif // !
