#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "co2.h"
#include "history.h"
#include "log.h"
#include "queue.h"
#include "stdint.h"
#include "string.h"
#include "ttask.h"
#include "ui.h"
#include "user.h"

// NUS (Nordic UART Service) Macros

/** Acquire NUS for sending */
#define NUS_TAKE()                                                                   \
    do                                                                               \
    {                                                                                \
        TaskWait(!EventGroupCheckBits(event_group_system, EVT_NUS_TAKEN), TICK_MAX); \
        EventGroupSetBits(event_group_system, EVT_NUS_TAKEN);                        \
        print("nus taken\n");                                                        \
    } while (0)

/** Release NUS */
#define NUS_GIVE()                                              \
    do                                                          \
    {                                                           \
        EventGroupClearBits(event_group_system, EVT_NUS_TAKEN); \
        print("nus release\n");                                 \
    } while (0)

// PROTOCOL HEADERS
#define CMD_FIRST_BYTE  0xFF
#define CMD_SECOND_BYTE 0xAA

// Protocol Commands
#define CMD_GET_CO2_VALUE                  0x01
#define CMD_SET_BUZZER_STATE               0x02
#define CMD_SET_VIBRATOR_STATE             0x03
#define CMD_SET_TIMEBASE                   0x04
#define CMD_SET_POWER_MODE                 0x05
#define CMD_SET_BLE_DISCONNECT             0x06
#define CMD_SET_GRAPH_YELLOW_AND_RED_START 0x07
#define CMD_GET_DEVICE_STATE               0x08
#define CMD_GET_DEVICE_NAME                0x09
#define CMD_SET_DEVICE_NAME                0x0A
#define CMD_GET_STORAGE_PAGE_NUM           0x0B
#define CMD_GET_HISTORY_PAGE               0x0C
#define CMD_CALIB_START                    0x0D
#define CMD_SET_SCREEN_CONST               0x0E
#define CMD_FIND_MY_AIRSPOT                0x10
#define CMD_SET_SELF_CALIB                 0x11
#define CMD_GET_HARDWARE_VERSION           0x12
#define CMD_GET_SOFTWARE_VERSION           0x13
#define CMD_ERASE_HISTORY                  0xFD
#define CMD_GET_SYSTEM_TIME                0xFE
#define CMD_SET_SENSOR_ERROR_DEBUG         0xFF
#define CMD_GET_BATTERY_LEVEL              0x20
#define CMD_REFRESH_CO2_VALUE              0x21
#define CMD_SET_DND                        0x22
#define CMD_POPULATE_FAKE_RECORDS          0x23
#define CMD_RESET_SENSOR                   0x24
#define CMD_GET_ASC_COUNT                  0x25
#define CMD_GET_DEVICE_STATE_DEBUG         0x26
#define CMD_RESET_DEVICE                   0x27
#define CMD_SET_GRAPH_MODE                 0x28
#define CMD_SET_ASC_CHECK_DURATION         0x29
#define CMD_GET_ASC_DAY_COUNT              0x2A
#define CMD_GET_SENSOR_VARIANT             0x2B
#define CMD_SET_ADVANCED_ALARM             0x2C
#define CMD_SET_ILLUMINATE_SCREEN_ON_ALARM 0x2D
#define CMD_SET_ALARM_CO2_FALLING          0x2E
#define CMD_RESET_ADVANCED_ALARMS          0x2F
#define CMD_GET_SENSOR_DETAILS             0x30
#define CMD_SET_CO2_SCALE_FACTOR           0x31
#define CMD_SET_FLIGHT_MODE                0x32
#define CMD_SET_DEVICE_SLEEP               0xEE
#define CMD_FACTORY_RESET                  0xFA

// Protocol Functions

/**
 * @brief Push received serial data into the buffer
 *
 * @param data Data
 * @param len Data length
 */
void proto_put_data(uint8_t *data, uint16_t len);

/**
 * @brief Send the system time
 *
 */
void proto_send_system_time(void);

/**
 * @brief Respond to APP calibration start
 */
void proto_send_recali_start(void);

/**
 * @brief Respond to APP calibration completion
 */
void proto_send_recali_done(int16_t cc);

/**
 * @brief Report countdown value
 *
 * @param countdown Countdown value
 */
void proto_send_recali_countdown(uint16_t countdown);

/**
 * @brief Report self-calibration result, 2 for success, 3 for failure
 *
 * @param result Result value
 */
void proto_send_set_self_recali_result(uint8_t result);

/**
 * @brief Send hardware version
 */
void proto_send_hardware_version(void);

/**
 * @brief Send software version
 */
void proto_send_software_version(void);

/**
 * @brief Notify that history data transmission is complete
 */
void proto_send_history_cplt(void);

/**
 * @brief Send device state
 */
void proto_send_device_state(void);

/**
 * @brief dump device state
 */
void proto_dump_device_state(void);

/**
 * @brief Send a frame via NUS service
 *
 * @param frame Frame data
 * @param len Frame length
 */
extern void proto_send_frame(uint8_t *frame, uint16_t len);

/**
 * @brief Send real-time CO2 value
 */
void send_realtime_co2(void);

/**
 * @brief Notify that history formatting is complete
 */
void send_erase_done(void);

/**
 * @brief send populating fake records done
 */
void send_populate_done(void);

/**
 * @brief Send the current half page of history data
 */
void proto_send_current_half_page(void);

/**
 * @brief Update the current history record
 *
 * @param value Record value
 * @param type Record type
 */
void add_history_record(uint16_t value, uint8_t type);

/**
 * @brief Send battery level
 *
 */
void proto_send_battery_level(void);

/**
 * Set the co2 scale factor
 */
void proto_set_co2_scale_factor(uint8_t *frame);

#endif // __PROTOCOL_H__
