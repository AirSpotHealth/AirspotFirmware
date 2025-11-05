#include "protocol.h"
#include "history/cfg_fstorage.h"
#include <stdint.h>

QUEUE_DEF(queue_proto, 1, 512);

extern uint8_t history_erase_all;
extern int16_t get_cc_value(void);
extern void    update_calibration_target(uint16_t target);
extern uint8_t get_asc_day_count(void);
extern uint8_t get_sensor_variant(void);
extern void    toggle_connection_interval(void);
extern void    set_fast_interval_timer(void);
extern void    update_flight_mode_activation_time(void);

/**
 * @brief 将接收的串口数据压入缓存
 *
 * @param data 数据
 * @param len 数据长度
 */
void proto_put_data(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        queue_in(&queue_proto, data + i);
    }
}

/**
 * @brief 校验帧
 *
 * @param frame 帧缓存
 * @param len 帧长度
 * @return uint8_t 校验通过返回1，否则0
 */
static uint8_t verify_frame_checksum(uint8_t *frame, uint16_t len)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < len - 1; i++)
    {
        sum += frame[i];
    }
    return ((uint8_t)(sum & 0xFF)) == frame[len - 1];
}

/**
 * @brief 为帧设置校验和
 *
 * @param frame 帧缓存
 * @param len 帧长度
 */
void set_frame_checksum(uint8_t *frame, uint16_t len)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < len - 1; i++)
    {
        sum += frame[i];
    }
    frame[len - 1] = (uint8_t)(sum & 0xFF);
}

/**
 * @brief Frame received handler
 *
 * @param frame the frame
 * @param len the frame length
 */
static void frame_received_handler(uint8_t *frame, uint16_t len)
{
    uint8_t tx_frame[32] = {0};

    if (!verify_frame_checksum(frame, len))
    {
        print("verify_frame_checksum failed\n");
        return;
    }

    switch (frame[2])
    {
    case CMD_GET_CO2_VALUE: // Send Realtime CO2
    {
        if (get_co2_value() != 0)
        {
            send_realtime_co2();
        }
        break;
    }
    case CMD_SET_BUZZER_STATE: // Set Alarm/Buzzer
    {
        set_buzzer_state(frame[4]);
        print("set_buzzer_state %d\n", frame[4]);
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_BUZZER_STATE;
        tx_frame[3] = 0x01;
        tx_frame[4] = frame[4];
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_SET_VIBRATOR_STATE: // Set Vibration
    {
        set_vibrator_state(frame[4]);
        print("set_vibrator_state %d\n", frame[4]);
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_VIBRATOR_STATE;
        tx_frame[3] = 0x01;
        tx_frame[4] = frame[4];
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_SET_TIMEBASE: // Set Timebase
    {
        uint32_t t     = (frame[4] << 24) | (frame[5] << 16) | (frame[6] << 8) | frame[7];
        uint16_t t_now = (uint16_t)get_time_now();

        if (get_time_set() == false)
        {
            set_time_set(true);
            set_timebase(t);

            // add a record to the history with value as the current time as uint16_t
            add_record(t_now, RECORD_TYPE_TIME_SET);
        }
        else
        {
            set_timebase(t);
        }

        print("set_timebase %d\n", t);
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_TIMEBASE;
        tx_frame[3] = 0x01;
        tx_frame[4] = 0x01;
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_SET_POWER_MODE: // Set Power Mode
    {
        set_power_mode((power_mode_t)frame[4]);
        EventGroupSetBits(event_group_system, EVT_BAT_UPDATE); // 使得电池更新图标从而更新电源模式
        print("set_power_mode %d\n", frame[4]);
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_POWER_MODE;
        tx_frame[3] = 0x01;
        tx_frame[4] = frame[4];
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_SET_BLE_DISCONNECT: // Set BLE Disconnect
    {
        print("set_ble_disconnect\n");
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_BLE_DISCONNECT;
        tx_frame[3] = 0x01;
        tx_frame[4] = 0x01;
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        advertising_stop();
        cfg_fstorage_set_bluetooth_enabled(0);
        // set_ble_disconnect();
        break;
    }
    case CMD_SET_GRAPH_YELLOW_AND_RED_START: // Set CO2 Graph Yellow And Red Start
    {
        print("set graph yellow and red start\n");
        if (len > 7)
        {
            cfg_fstorage_set_graph_yellow_and_red_start((frame[4] << 8) | frame[5], (frame[6] << 8) | frame[7]);
            EventGroupSetBits(event_group_system, EVT_UI_GRAPH_UPDATE);
        }
        break;
    }
    case CMD_GET_DEVICE_STATE: // Send Device State/Config
    {
        proto_send_device_state();
        break;
    }
    case CMD_GET_DEVICE_NAME: // Send Device Name
    {
        char *name = get_device_name();
        print("get_device_name: %s\n", name);
        uint8_t name_len = strlen(name);
        tx_frame[0]      = CMD_FIRST_BYTE;
        tx_frame[1]      = CMD_SECOND_BYTE;
        tx_frame[2]      = CMD_GET_DEVICE_NAME;
        tx_frame[3]      = name_len;
        if (name_len != 0)
        {
            strcpy((char *)tx_frame + 4, name);
        }
        set_frame_checksum(tx_frame, 5 + name_len);
        proto_send_frame(tx_frame, 5 + name_len);
        break;
    }
    case CMD_SET_DEVICE_NAME: // Set Device Name
    {
        frame[len - 1] = '\0'; // 给字符串添加结束符
        set_device_name((char *)frame + 4);
        print("set_device_name: %s\n", (char *)frame + 4);
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_DEVICE_NAME;
        tx_frame[3] = 0x01;
        tx_frame[4] = 0x01;
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_GET_STORAGE_PAGE_NUM: // Page number request
    {
        toggle_connection_interval();
        proto_send_current_half_page();
        break;
    }
    case CMD_GET_HISTORY_PAGE: // History upload
    {
        // check if 4th and 5th bytes are there
        if (len < 6) break;

        // since the history request is still in progress, we need to reset the fast interval timer
        set_fast_interval_timer();
        history_request((frame[4] << 8) | frame[5]);
        EventGroupSetBits(event_group_system, EVT_REQUEST_HISTORY);

        break;
    }
    case CMD_CALIB_START: // Calibration Start
    {
        print("start task_calibration\n");
        // check if 4th and 5th bytes are there
        if (len < 6)
        {
            co2_start_calibration();
        } // start calibration with default target
        else
        {
            update_calibration_target((frame[4] << 8) | frame[5]);
        } // start calibration with target
        break;
    }
    case CMD_SET_SCREEN_CONST: // Set Screen Const On
    {
        if (frame[4] == 1)
        {
            print("set screen const on\n");
            set_screen_const_on(UI_BOOL_TRUE);
        }
        else if (frame[4] == 2)
        {
            print("chancel screen const on\n");
            set_screen_const_on(UI_BOOL_FALSE);
        }
        else
            break;
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_SCREEN_CONST;
        tx_frame[3] = 0x01;
        tx_frame[4] = 0x03;
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_FIND_MY_AIRSPOT: // Find my AirSpot
    {
        if (frame[4] == 1)
        {
            print("find my airspot\n");
            EventGroupSetBits(event_group_system, EVT_FIND_DEVICE);
            tx_frame[0] = CMD_FIRST_BYTE;
            tx_frame[1] = CMD_SECOND_BYTE;
            tx_frame[2] = CMD_FIND_MY_AIRSPOT;
            tx_frame[3] = 0x01;
            tx_frame[4] = 0x01;
            set_frame_checksum(tx_frame, 6);
            proto_send_frame(tx_frame, 6);
        }
        break;
    }
    case CMD_SET_SELF_CALIB: // Set Self Calibration
    {
        if (frame[4] == 0 || frame[4] == 1)
        {
            if (0 == co2_set_self_calibration(frame[4]))
            {
                print("set self caliberation success\n");
                proto_send_set_self_recali_result(2);
            }
            else
            {
                print("set self caliberation failed\n");
                proto_send_set_self_recali_result(3);
            }
        }
        break;
    }
    case CMD_GET_HARDWARE_VERSION: // Send Hardware Version
    {
        proto_send_hardware_version();
        break;
    }
    case CMD_GET_SOFTWARE_VERSION: // Send Software Version
    {
        proto_send_software_version();
        break;
    }
    case CMD_ERASE_HISTORY: // Erase History
    {
        print("erase history\n");
        history_erase_all = 1; // 测试用格式化所有记录
        break;
    }
    case CMD_GET_SYSTEM_TIME: // Get System Time
    {
        print("Get the system time");
        proto_send_system_time();
        break;
    }
    case CMD_SET_SENSOR_ERROR_DEBUG: // Set Sensor Error Flag (Debug)
    {
        // if frame[3] == 0x01, set the sensor error flag
        // if it is 0x02 clear the sensor error flag
        if (frame[4] == 0x01)
        {
            print("set sensor error\n");
            EventGroupSetBits(event_group_system, EVT_CO2_SENSOR_ERROR | EVT_CO2_UP_HIS);
            // set a fake sensor error record
            add_record(ERROR_CODE_SENSOR_READ, RECORD_TYPE_SENSOR_ERROR);
            EventGroupSetBits(event_group_system, EVT_SCREEN_ON_ONETIME);
        }
        else
        {
            print("clear sensor error\n");
            EventGroupClearBits(event_group_system, EVT_CO2_SENSOR_ERROR);
        }
        break;
    }
    case CMD_GET_BATTERY_LEVEL: // Send Battery Level
    {
        proto_send_battery_level();
        break;
    }
    case CMD_REFRESH_CO2_VALUE: // Refresh co2 value or reread co2 value
    {
        print("refresh co2 value\n");
        EventGroupSetBits(event_group_system, EVT_CO2_UPDATE_ONCE);
        break;
    }
    case CMD_SET_DND: // Set DND off/on
    {
        print("set DND\n");
        if (frame[4] == 0)
        {
            print("set DND off\n");
            cfg_fstorage_set_dnd_mode(0);
            EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
        }
        else if (frame[4] == 1)
        {
            print("set DND on\n");
            cfg_fstorage_set_dnd_start_end_time(frame[5], frame[6], frame[7], frame[8]);
            EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
        }

        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_SET_DND;
        tx_frame[3] = 0x01;
        tx_frame[4] = cfg_fstorage_get_dnd_mode();
        set_frame_checksum(tx_frame, 6);

        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_POPULATE_FAKE_RECORDS: // Populate Fake Records
    {
        print("populate fake records\n");
        EventGroupSetBits(event_group_system, EVT_POPULATE_FAKE_DATA);
        break;
    }
    case CMD_RESET_SENSOR: // Reset the co2 sensor and CC value
    {
        print("reset the co2 sensor and CC value\n");
        EventGroupSetBits(event_group_system, EVT_CO2_FACTORY_RESET);
        break;
    }
    case CMD_GET_ASC_COUNT: // get the asc_count and last_correction value
    {
        print("get the asc_count and last_correction value\n");
        int16_t  last_correction = get_cc_value();
        uint16_t correction_abs  = (uint16_t)(last_correction < 0 ? -last_correction : last_correction);

        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_GET_ASC_COUNT;
        tx_frame[3] = 0x01;

        tx_frame[4] = (uint8_t)((uint16_t)correction_abs >> 8);
        tx_frame[5] = (uint8_t)((uint16_t)correction_abs & 0xFF);
        tx_frame[6] = (last_correction < 0) ? 0x01 : 0x00; // Sign flag
        set_frame_checksum(tx_frame, 8);
        proto_send_frame(tx_frame, 8);
        break;
    }
    case CMD_GET_DEVICE_STATE_DEBUG: // dump the state of the system
    {
        print("dump system state\n");

        proto_dump_device_state();

        break;
    }
    case CMD_RESET_DEVICE: // Reset the device
    {
        print("reset the device\n");
        print("TIMESTAMP BEFORE RESET: %u\n", get_time_now());
        NVIC_SystemReset();
        break;
    }
    case CMD_SET_GRAPH_MODE: // Change the graph type
    {
        print("change the graph type\n");
        if (frame[4] == 0)
        {
            print("change to bar graph\n");
            if (len > 7)
            {
                cfg_fstorage_set_ui_mode(0, (frame[5] << 8) | frame[6], (frame[7] << 8) | frame[8]);
            }
            else
            {
                cfg_fstorage_set_ui_mode(0, cfg_fstorage_get_graph_max_value(), cfg_fstorage_get_graph_min_value());
            }
        }
        else if (frame[4] == 1)
        {
            cfg_fstorage_set_ui_mode(1, cfg_fstorage_get_graph_max_value(), cfg_fstorage_get_graph_min_value());
        }
        else
        {
            cfg_fstorage_set_ui_mode(2, cfg_fstorage_get_graph_max_value(), cfg_fstorage_get_graph_min_value());
        }
        EventGroupSetBits(event_group_system, EVT_TOGGLE_UI_MODE);

        break;
    }
    case CMD_SET_ASC_CHECK_DURATION: // Set ASC Check Duration
    {
        print("set ASC check duration\n");
        set_asc_check_duration((frame[4] << 8) | frame[5]);
        break;
    }
    case CMD_GET_ASC_DAY_COUNT: // get the asc day count
    {
        print("get the asc day count\n");
        tx_frame[0] = CMD_FIRST_BYTE;
        tx_frame[1] = CMD_SECOND_BYTE;
        tx_frame[2] = CMD_GET_ASC_DAY_COUNT;
        tx_frame[3] = 0x01;
        tx_frame[4] = get_asc_day_count();
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_GET_SENSOR_VARIANT: // Get the sensor variant
    {
        print("get the sensor variant\n");
        uint8_t variant = get_sensor_variant();
        tx_frame[0]     = CMD_FIRST_BYTE;
        tx_frame[1]     = CMD_SECOND_BYTE;
        tx_frame[2]     = CMD_GET_SENSOR_VARIANT;
        tx_frame[3]     = 0x01;
        tx_frame[4]     = variant;
        set_frame_checksum(tx_frame, 6);
        proto_send_frame(tx_frame, 6);
        break;
    }
    case CMD_SET_ALARM_CO2_FALLING:
    {
        if (frame[3] == 1) // Expect 1 byte payload (0 or 1)
        {
            uint8_t enabled_state = frame[4];
            cfg_fstorage_set_alarm_co2_falling(enabled_state);
            print("Set alarm on CO2 falling: %d\n", enabled_state);

            // Send ACK
            tx_frame[0] = CMD_FIRST_BYTE;
            tx_frame[1] = CMD_SECOND_BYTE;
            tx_frame[2] = CMD_SET_ALARM_CO2_FALLING;
            tx_frame[3] = 0x01;          // Payload length: 1 byte (status)
            tx_frame[4] = enabled_state; // Return the set state
            set_frame_checksum(tx_frame, 6);
            proto_send_frame(tx_frame, 6);
        }
        else
        {
            print("CMD_SET_ALARM_CO2_FALLING: Invalid payload length %d\n", frame[3]);
        }
        break;
    }
    case CMD_SET_DEVICE_SLEEP: // Sleep the device
    {
        print("Sleep the sensor");
        // check if the screen is on, if it is, turn it off
        EventGroupSetBits(event_group_system, EVT_DEVICE_SLEEP | EVT_UI_OFF_SCREEN);
        // Sleep the sensor, stop the app timer and will only be woken up by button press for 5 seconds
        break;
    }
    case CMD_SET_ILLUMINATE_SCREEN_ON_ALARM:
    {
        if (frame[3] == 1) // Expect 1 byte payload (0 or 1)
        {
            uint8_t enabled_state = frame[4];
            cfg_fstorage_set_illuminate_screen_on_alarm(enabled_state);
            print("Set illuminate screen on alarm: %d\n", enabled_state);

            // Send ACK
            tx_frame[0] = CMD_FIRST_BYTE;
            tx_frame[1] = CMD_SECOND_BYTE;
            tx_frame[2] = CMD_SET_ILLUMINATE_SCREEN_ON_ALARM;
            tx_frame[3] = 0x01;          // Payload length: 1 byte (status)
            tx_frame[4] = enabled_state; // Return the set state
            set_frame_checksum(tx_frame, 6);
            proto_send_frame(tx_frame, 6);
        }
        else
        {
            print("CMD_SET_ILLUMINATE_SCREEN_ON_ALARM: Invalid payload length %d\n", frame[3]);
        }
        break;
    }
    case CMD_SET_ADVANCED_ALARM: // Set a single Advanced Alarm setting
    {
        // Expected payload: frame[4]=index (0-9), frame[5-6]=CO2 level, frame[7]=repeats, frame[8]=enabled
        // Total payload length (frame[3]) should be 5 bytes.
        if (frame[3] == 5)
        {
            uint8_t alarm_index = frame[4];
            if (alarm_index < 10) // Validate index
            {
                advanced_alarm_setting_t alarm_setting;
                alarm_setting.co2_level            = (frame[5] << 8) | frame[6];
                alarm_setting.alarm_repeats        = frame[7];
                alarm_setting.notification_enabled = frame[8];

                print("Set advanced alarm index %d: co2 %d, repeats %d, enabled %d\n",
                      alarm_index, alarm_setting.co2_level, alarm_setting.alarm_repeats, alarm_setting.notification_enabled);

                cfg_fstorage_set_advanced_alarm(alarm_index, &alarm_setting);

                // Send ACK
                tx_frame[0] = CMD_FIRST_BYTE;
                tx_frame[1] = CMD_SECOND_BYTE;
                tx_frame[2] = CMD_SET_ADVANCED_ALARM;
                tx_frame[3] = 0x01;        // Payload length: 1 byte (index of alarm updated)
                tx_frame[4] = alarm_index; // Return the index that was set
                set_frame_checksum(tx_frame, 6);
                proto_send_frame(tx_frame, 6);
            }
            else
            {
                print("CMD_SET_ADVANCED_ALARM: Invalid alarm index %d\n", alarm_index);
                // Optionally send a NACK or error response here
            }
        }
        else
        {
            print("CMD_SET_ADVANCED_ALARM: Invalid payload length %d, expected 5\n", frame[3]);
        }
        break;
    }
    case CMD_RESET_ADVANCED_ALARMS:
    {
        print("Reset advanced alarms\n");
        cfg_fstorage_reset_advanced_alarms_to_default();
        break;
    }
    case CMD_GET_SENSOR_DETAILS:
    {
        print("Get sensor details\n");
        EventGroupSetBits(event_group_system, EVT_GET_SENSOR_DETAILS);
        break;
    }
    case CMD_SET_CO2_SCALE_FACTOR:
    {
        print("Set co2 scale factor\n");
        if (frame[3] == 4) // Ensure payload is 4 bytes for a float
        {
            proto_set_co2_scale_factor(frame);
        }
        else
        {
            print("CMD_SET_CO2_SCALE_FACTOR: Invalid payload length %d, expected 4\n", frame[3]);
        }
        break;
    }
    case CMD_SET_FLIGHT_MODE:
    {
        print("Set flight mode\n");
        cfg_fstorage_set_flight_mode(frame[4]);
        EventGroupSetBits(event_group_system, EVT_FLIGHT_MODE_UPDATE);
        update_flight_mode_activation_time();
        add_history_record(frame[4], RECORD_TYPE_FLIGHT_MODE);
        EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
        break;
    }
    case CMD_FACTORY_RESET:
    {
        print("Factory reset\n");
        cfg_fstorage_erase_all(1); // 1 means erase all
        NVIC_SystemReset();        // Reset the device
        break;
    }
    default:
        print("Unknown Command Received: %d", frame[2]);
        break;
    }
}

/**
 * Set the co2 scale factor
 */
void proto_set_co2_scale_factor(uint8_t *frame)
{
    float    co2_scale_factor = 0.0f;
    uint32_t temp_val         = 0;

    // Reconstruct float from 4 bytes (big-endian)
    temp_val = ((uint32_t)frame[4] << 24) |
               ((uint32_t)frame[5] << 16) |
               ((uint32_t)frame[6] << 8) |
               ((uint32_t)frame[7]);

    memcpy(&co2_scale_factor, &temp_val, sizeof(co2_scale_factor));

    print("Received co2_scale_factor (from bytes: %02X %02X %02X %02X -> 0x%08X). Parsed as: %d.%04d\n",
          frame[4], frame[5], frame[6], frame[7], temp_val,
          (int)co2_scale_factor,
          (int)((co2_scale_factor - (int)co2_scale_factor) * 10000));

    if (co2_scale_factor < 0.5f || co2_scale_factor > 2.0f)
    {
        print("Invalid co2 scale factor %f\n", co2_scale_factor);
    }
    else
    {
        cfg_fstorage_set_co2_scale_factor(co2_scale_factor);

        // 🔽 Encode float as uint16_t for history log
        uint16_t encoded_scale = (uint16_t)(((co2_scale_factor - 0.5f) * 65535.0f) / 1.5f + 0.5f); // rounding

        add_history_record(encoded_scale, RECORD_TYPE_CO2_SCALE_FACTOR);

        EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
    }
}

/**
 * @brief Dump the device state
 *
 */
void proto_dump_device_state(void)
{
    static uint8_t tx_frame[244] = {0};

    // add event group state
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_DEVICE_STATE_DEBUG;
    tx_frame[3] = 0x01;
    memcpy(&tx_frame[4], &event_group_system, sizeof(event_group_system));
    memcpy(&tx_frame[12], (uint8_t *)cfg_fstorage_dump(), sizeof(cfg_fstorage_t));

    set_frame_checksum(tx_frame, 12 + sizeof(cfg_fstorage_t));
    proto_send_frame(tx_frame, 12 + sizeof(cfg_fstorage_t));
}

/**
 * @brief send factory reset done
 */
void send_factory_reset_done(uint8_t self_cali_enable)
{
    uint8_t tx_frame[32] = {0};

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_RESET_SENSOR;
    tx_frame[3] = 0x01;
    tx_frame[4] = self_cali_enable;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**
 * @brief Send the current battery level
 *
 */
void proto_send_battery_level()
{
    uint8_t tx_frame[32]  = {0};
    uint8_t battery_level = get_battery_level();
    uint8_t charging      = USB_IN();
    print("send_battery_level %d\n", battery_level);
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_BATTERY_LEVEL;
    tx_frame[3] = 0x01;
    tx_frame[4] = battery_level;
    tx_frame[5] = charging;
    set_frame_checksum(tx_frame, 7);
    proto_send_frame(tx_frame, 7);
}

/**
 * @brief Send the current half page number
 *
 */
void proto_send_current_half_page()
{
    uint8_t tx_frame[32] = {0};
    // This is a request for the latest page number of the flash that the data is stored in.
    uint16_t page_number = get_current_half_page();
    print("send_current_half_page %04X\n", page_number);
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_HISTORY_PAGE;
    tx_frame[3] = 0x01;
    tx_frame[4] = (uint8_t)(page_number >> 8);
    tx_frame[5] = (uint8_t)(page_number & 0xFF);
    set_frame_checksum(tx_frame, 7);
    proto_send_frame(tx_frame, 7);
}

void proto_send_system_time(void)
{
    uint8_t  tx_frame[32] = {0};
    uint32_t time         = get_time_now();
    tx_frame[0]           = CMD_FIRST_BYTE;
    tx_frame[1]           = CMD_SECOND_BYTE;
    tx_frame[2]           = CMD_GET_SYSTEM_TIME;
    tx_frame[3]           = 0x04;
    tx_frame[4]           = (uint8_t)(time >> 24);
    tx_frame[5]           = (uint8_t)(time >> 16);
    tx_frame[6]           = (uint8_t)(time >> 8);
    tx_frame[7]           = (uint8_t)(time);
    set_frame_checksum(tx_frame, 8);
    proto_send_frame(tx_frame, 8);
}

/**
 * @brief 回复APP校准开始
 *
 */
void proto_send_recali_start(void)
{
    uint8_t tx_frame[32] = {0};

    print("calibration start\n");
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_CALIB_START;
    tx_frame[3] = 0x01;
    tx_frame[4] = 0x02;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**
 * @brief 回复APP校准已完成
 *
 */
void proto_send_recali_done(int16_t cc)
{
    uint8_t  tx_frame[32] = {0};
    uint16_t cc_abs       = cc < 0 ? -cc : cc;

    print("CC ORIGINAL VALUE %d, ABS %d\n", cc, cc_abs);

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_CALIB_START;
    tx_frame[3] = 0x01;
    // add cc value as hex not decimal
    tx_frame[4] = (uint8_t)(cc_abs >> 8);
    tx_frame[5] = (uint8_t)(cc_abs & 0xFF);
    // if cc is positive, set the 6th byte to 0x01, otherwise 0x02
    if (cc > 0)
        tx_frame[6] = 0x00;
    else
        tx_frame[6] = 0x01;

    set_frame_checksum(tx_frame, 8);
    log_hex_dump("Calibration Frame", tx_frame, 8);
    proto_send_frame(tx_frame, 8);
}

/**
 * @brief 上报倒计时计数
 *
 */
void proto_send_recali_countdown(uint16_t countdown)
{
    uint8_t tx_frame[32] = {0};

    print("count down %d\n", countdown);
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = 0x0f; // CMD_RECALI_COUNTDOWN - Assuming this is missing or defined elsewhere, will leave as is if not in protocol.h
    tx_frame[3] = 0x01;
    tx_frame[4] = (uint8_t)(countdown >> 8);
    tx_frame[5] = (uint8_t)(countdown);
    set_frame_checksum(tx_frame, 7);
    proto_send_frame(tx_frame, 7);
}

/**
 * @brief 上报设置自校准结果, 2成功, 3失败
 *
 */
void proto_send_set_self_recali_result(uint8_t result)
{
    uint8_t tx_frame[32] = {0};

    // print("calibration done\n");
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_SET_SELF_CALIB;
    tx_frame[3] = 0x01;
    tx_frame[4] = result;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**回复发送历史数据完毕 */
void proto_send_history_cplt(void)
{
    uint8_t tx_frame[32] = {0};

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_HISTORY_PAGE;
    tx_frame[3] = 0x01;
    tx_frame[4] = 0x02;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**回复硬件版本 */
void proto_send_hardware_version(void)
{
    uint8_t tx_frame[32] = {0};
    char    ver[]        = HARDWARE_REVISION;

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_HARDWARE_VERSION;
    tx_frame[3] = strlen(ver);
    memcpy(tx_frame + 4, ver, tx_frame[3]);
    set_frame_checksum(tx_frame, 5 + tx_frame[3]);
    proto_send_frame(tx_frame, 5 + tx_frame[3]);
}

/**回复软件版本 */
void proto_send_software_version(void)
{
    uint8_t tx_frame[32] = {0};
    char    ver[]        = SOFTWARE_REVISION;

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_SOFTWARE_VERSION;
    tx_frame[3] = strlen(ver);
    memcpy(tx_frame + 4, ver, tx_frame[3]);
    set_frame_checksum(tx_frame, 5 + tx_frame[3]);
    proto_send_frame(tx_frame, 5 + tx_frame[3]);
}

/**回复设备状态 */
void proto_send_device_state(void)
{
    uint8_t  tx_frame[80]; // Increased size, initialized below or by parts
    uint16_t yellow_start;
    uint16_t red_start;
    float    actual_co2_scale_factor;
    uint32_t co2_scale_factor_bits;
    int      offset = 0; // To keep track of current position in tx_frame
    uint8_t  payload_length;
    uint8_t  total_frame_length;

    // Initialize frame
    memset(tx_frame, 0, sizeof(tx_frame));

    yellow_start            = cfg_fstorage_get_graph_yellow_start();
    red_start               = cfg_fstorage_get_graph_red_start();
    actual_co2_scale_factor = cfg_fstorage_get_co2_scale_factor();                           // Get the float value
    memcpy(&co2_scale_factor_bits, &actual_co2_scale_factor, sizeof(co2_scale_factor_bits)); // Copy bits

    // Set up basic device state
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_DEVICE_STATE;
    // tx_frame[3] will be set after calculating payload length

    offset = 4; // Start populating payload after the header

    tx_frame[offset++] = get_buzzer_state();
    tx_frame[offset++] = get_vibrator_state();
    tx_frame[offset++] = get_power_mode();
    tx_frame[offset++] = (uint8_t)(yellow_start >> 8);
    tx_frame[offset++] = (uint8_t)(yellow_start & 0xFF);
    tx_frame[offset++] = (uint8_t)(red_start >> 8);
    tx_frame[offset++] = (uint8_t)(red_start & 0xFF);
    tx_frame[offset++] = (uint8_t)get_screen_const_on_state();
    tx_frame[offset++] = cfg_fstorage_get_auto_calibrate();
    tx_frame[offset++] = cfg_fstorage_get_dnd_mode();
    tx_frame[offset++] = cfg_fstorage_get_dnd_start_hour();
    tx_frame[offset++] = cfg_fstorage_get_dnd_start_minute();
    tx_frame[offset++] = cfg_fstorage_get_dnd_end_hour();
    tx_frame[offset++] = cfg_fstorage_get_dnd_end_minute();
    tx_frame[offset++] = cfg_fstorage_get_calib_target() >> 8;
    tx_frame[offset++] = cfg_fstorage_get_calib_target() & 0xFF;
    tx_frame[offset++] = cfg_fstorage_get_ui_mode();
    tx_frame[offset++] = cfg_fstorage_get_graph_max_value() >> 8;
    tx_frame[offset++] = cfg_fstorage_get_graph_max_value() & 0xFF;
    tx_frame[offset++] = cfg_fstorage_get_graph_min_value() >> 8;
    tx_frame[offset++] = cfg_fstorage_get_graph_min_value() & 0xFF; // offset is now 4 + 21 = 25

    // Add illuminate screen on alarm and alarm on CO2 falling
    tx_frame[offset++] = cfg_fstorage_get_illuminate_screen_on_alarm(); // offset = 26
    tx_frame[offset++] = cfg_fstorage_get_alarm_co2_falling();          // offset = 27

    // Add all advanced alarm settings
    advanced_alarm_setting_t alarms[10];
    cfg_fstorage_get_all_advanced_alarms(alarms);
    for (int i = 0; i < 10; i++)
    {
        tx_frame[offset++] = (uint8_t)(alarms[i].co2_level >> 8);
        tx_frame[offset++] = (uint8_t)(alarms[i].co2_level & 0xFF);
        tx_frame[offset++] = alarms[i].alarm_repeats;
        tx_frame[offset++] = alarms[i].notification_enabled;
    } // offset = 27 + 40 = 67

    // add co2 scale factor
    tx_frame[offset++] = (uint8_t)(co2_scale_factor_bits >> 24);
    tx_frame[offset++] = (uint8_t)(co2_scale_factor_bits >> 16);
    tx_frame[offset++] = (uint8_t)(co2_scale_factor_bits >> 8);
    tx_frame[offset++] = (uint8_t)(co2_scale_factor_bits); // offset = 67 + 4 = 71

    // add flight mode
    tx_frame[offset++] = cfg_fstorage_get_flight_mode();

    payload_length = offset - 4; // Total bytes written after header (tx_frame[4] to tx_frame[offset-1])
    tx_frame[3]    = payload_length;

    total_frame_length = 4 + payload_length + 1; // Header + Payload + Checksum

    print("DeviceState: PayloadLen=%d (0x%02X), TotalFrameLen=%d. ScaleFactor (float): %d.%03d (bits: 0x%08X)\n",
          payload_length, payload_length,
          total_frame_length,
          (int)actual_co2_scale_factor,
          (int)((actual_co2_scale_factor - (int)actual_co2_scale_factor) * 1000),
          co2_scale_factor_bits);

    set_frame_checksum(tx_frame, total_frame_length);
    proto_send_frame(tx_frame, total_frame_length);
}

#define FRAME_MAX_LEN (244) // Maximum allowed frame size

/**
 * @brief Sends a page of historical data over Bluetooth.
 *
 * @param rec_page Pointer to the raw buffer containing the page data (8 bytes per record).
 * @param record_count Number of valid records in the page.
 * @param page_number Page number of the flash the data is from
 */

void send_history_data(const record_page_t *rec_page, uint8_t record_count, uint16_t page_number)
{
    // the transmit frame
    static uint8_t tx_frame[FRAME_MAX_LEN] = {0};

    // Each transmitted record consists of 8 bytes (4 bytes timestamp + 1 byte type + 2 bytes CO₂ value + 1 reserved byte)
    static uint16_t record_size = sizeof(record_t);

    // Frame header size
    static uint8_t header_size = 4; // Example: 2 bytes start, 1 byte type, 1 byte data length

    print("Sending history data: Records=%d\n", record_count);

    // Construct frame header
    tx_frame[0] = CMD_FIRST_BYTE;       // Frame start byte 1
    tx_frame[1] = CMD_SECOND_BYTE;      // Frame start byte 2
    tx_frame[2] = CMD_GET_HISTORY_PAGE; // Frame type
    tx_frame[3] = 16 * record_size;     // Data length (excluding header and checksum)

    // Append records
    uint8_t frame_offset = header_size;

    for (uint8_t i = 0; i < 16; i++)
    {
        uint32_t timestamp = rec_page->records[i].timestamp;
        uint8_t  type      = rec_page->records[i].type;
        uint16_t value     = rec_page->records[i].value;

        // Append timestamp in little-endian
        tx_frame[frame_offset++] = (uint8_t)(timestamp >> 24); // MSB
        tx_frame[frame_offset++] = (uint8_t)(timestamp >> 16);
        tx_frame[frame_offset++] = (uint8_t)(timestamp >> 8);
        tx_frame[frame_offset++] = (uint8_t)(timestamp >> 0); // LSB

        // Append CO₂ value in little-endian
        tx_frame[frame_offset++] = (uint8_t)(value >> 0); // LSB
        tx_frame[frame_offset++] = (uint8_t)(value >> 8); // MSB

        // Append Record type
        tx_frame[frame_offset++] = (uint8_t)(type);

        // Append the reserved byte
        tx_frame[frame_offset++] = 0;
    }

    // append page number
    tx_frame[frame_offset++] = (uint8_t)(page_number >> 8);
    tx_frame[frame_offset++] = (uint8_t)(page_number & 0xFF);

    // Calculate checksum (simple sum of all bytes)
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < frame_offset; i++)
    {
        checksum += tx_frame[i];
    }

    tx_frame[frame_offset++] = checksum; // Append checksum

    // Log and send the frame
    // log_hex_dump("tx_frame", tx_frame, frame_offset);
    proto_send_frame(tx_frame, frame_offset);
}

void add_history_record(uint16_t value, uint8_t type)
{
    add_record(value, type);
}

/**
 * @brief 发送实时二氧化碳值
 *
 */
void send_realtime_co2(void)
{
    uint8_t tx_frame[32] = {0};

    uint16_t co2 = get_co2_value();
    // print("get_co2_value %d\n", co2);
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_CO2_VALUE;
    tx_frame[3] = 0x02;
    // add the current time as well to the frame
    uint32_t time = get_time_now();

    tx_frame[4] = (uint8_t)(time >> 24);
    tx_frame[5] = (uint8_t)(time >> 16);
    tx_frame[6] = (uint8_t)(time >> 8);
    tx_frame[7] = (uint8_t)(time);

    tx_frame[8] = (uint8_t)(co2 >> 8);
    tx_frame[9] = (uint8_t)(co2 & 0xFF);
    set_frame_checksum(tx_frame, 11);
    proto_send_frame(tx_frame, 11);
}

/**
 * @brief 格式化历史完毕
 *
 */
void send_erase_done(void)
{
    uint8_t tx_frame[32] = {0};

    print("send_erase_done\n");
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_ERASE_HISTORY;
    tx_frame[3] = 0x01;
    tx_frame[4] = 0x01;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**
 * @brief send populating fake records done
 */
void send_populate_done(void)
{
    uint8_t tx_frame[32] = {0};

    print("send_populate_done\n");
    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_POPULATE_FAKE_RECORDS;
    tx_frame[3] = 0x01;
    tx_frame[4] = 0x01;
    set_frame_checksum(tx_frame, 6);
    proto_send_frame(tx_frame, 6);
}

/**
 * @brief 协议数据处理任务
 *
 */
TaskDefine(task_protocol)
{
    static uint8_t rx_frame[32];
    static uint8_t rx_frame_idx = 0;

    TTS while (1)
    {
        TaskWait(queue_get_count(&queue_proto) != 0, 1000 / TICK_RATE_MS);

        if (TaskTimeoutExpired()) // 超时，清除可能的未接收完全的帧
        {
            rx_frame_idx = 0;
            continue;
        }

        while (queue_get_count(&queue_proto) != 0)
        {
            // 填入数据，索引后移
            queue_out(&queue_proto, rx_frame + rx_frame_idx);
            // print("rx %02X\n", *(rx_frame + rx_frame_idx));
            rx_frame_idx += 1;

            // 帧头1错误重新接收
            if (rx_frame_idx == 1 && rx_frame[0] != CMD_FIRST_BYTE)
            {
                rx_frame_idx = 0;
            }
            // 帧头2错误，重新接收
            if (rx_frame_idx == 2 && rx_frame[1] != CMD_SECOND_BYTE)
            {
                rx_frame_idx = 0;
            }
            // 接收完成
            if (rx_frame_idx == (5 + rx_frame[3]))
            {
                NUS_TAKE();
                frame_received_handler(rx_frame, rx_frame_idx);
                log_hex_dump("rx", rx_frame, rx_frame_idx);
                NUS_GIVE();
                rx_frame_idx = 0;
                break;
            }
        }
    }
    TTE
}

/**
 * @brief 发送传感器详细信息 (Send Sensor Details)
 *
 * @param temp_offset Temperature offset in ticks
 * @param altitude Altitude in meters
 * @param ambient_pressure_mbar Ambient pressure in mBar (placeholder if not readable)
 * @param asc_enabled ASC enabled state (0 or 1)
 * @param asc_target ASC target in ppm
 * @param serial_no Pointer to 6-byte serial number
 * @param sensor_variant Sensor variant ID
 */
void proto_send_sensor_details_response(uint16_t temp_offset,
                                        uint16_t altitude,
                                        uint16_t ambient_pressure_mbar,
                                        uint8_t  asc_enabled,
                                        uint16_t asc_target,
                                        uint8_t *serial_no, // 6 bytes
                                        uint8_t  sensor_variant)
{
    uint8_t tx_frame[32] = {0}; // Enough for header + 16 byte payload + checksum
    uint8_t payload_len  = 16;
    int     offset       = 4; // Start after CMD_FIRST, CMD_SECOND, CMD_ID, PAYLOAD_LEN

    tx_frame[0] = CMD_FIRST_BYTE;
    tx_frame[1] = CMD_SECOND_BYTE;
    tx_frame[2] = CMD_GET_SENSOR_DETAILS; // Use the same command ID for response
    tx_frame[3] = payload_len;

    // Temperature Offset (uint16_t, 2 bytes, big-endian)
    tx_frame[offset++] = (uint8_t)(temp_offset >> 8);
    tx_frame[offset++] = (uint8_t)(temp_offset & 0xFF);

    // Sensor Altitude (uint16_t, 2 bytes, big-endian)
    tx_frame[offset++] = (uint8_t)(altitude >> 8);
    tx_frame[offset++] = (uint8_t)(altitude & 0xFF);

    // Ambient Pressure (uint16_t, 2 bytes, big-endian, in mBar)
    tx_frame[offset++] = (uint8_t)(ambient_pressure_mbar >> 8);
    tx_frame[offset++] = (uint8_t)(ambient_pressure_mbar & 0xFF);

    // ASC Enabled (uint8_t, 1 byte)
    tx_frame[offset++] = asc_enabled;

    // ASC Target (uint16_t, 2 bytes, big-endian)
    tx_frame[offset++] = (uint8_t)(asc_target >> 8);
    tx_frame[offset++] = (uint8_t)(asc_target & 0xFF);

    // Serial Number (6 bytes)
    if (serial_no != NULL)
    {
        memcpy(&tx_frame[offset], serial_no, 6);
    }
    else
    {
        // Send 0s if serial_no is NULL (error case)
        memset(&tx_frame[offset], 0, 6);
    }
    offset += 6;

    // Sensor Variant (uint8_t, 1 byte)
    tx_frame[offset++] = sensor_variant;

    set_frame_checksum(tx_frame, 5 + payload_len); // 4 header bytes + payload_len byte + payload
    proto_send_frame(tx_frame, 5 + payload_len);
}
