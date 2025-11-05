/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */

#include "app_scheduler.h"
#include "app_timer.h"
#include "app_util_platform.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "ble_dfu.h"
#include "ble_dis.h"
#include "ble_hci.h"
#include "ble_nus.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_strerror.h"
#include <stdint.h>
#include <string.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_drv_wdt.h"
#include "nrfx_wdt.h"

#include "button.h"
#include "cfg_fstorage.h"
#include "log.h"
#include "protocol.h"
#include "st7301.h"
#include "ttask.h"
#include "user.h"

#include "flash_spi.h"
#include "spi.h"

/// Added for bonding
#include "peer_manager.h"
#include "peer_manager_handler.h"

#define SEC_PARAM_BOND            1                            /**< Perform bonding. */
#define SEC_PARAM_MITM            1                            /**< Man In The Middle protection is enabled. */
#define SEC_PARAM_LESC            0                            /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS        0                            /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_DISPLAY_ONLY /**< Display Only. */
#define SEC_PARAM_OOB             0                            /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE    7                            /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE    16                           /**< Maximum encryption key size. */
/// Added for bonding

#define APP_BLE_CONN_CFG_TAG 1 /**< A tag identifying the SoftDevice BLE configuration. */

#define NUS_SERVICE_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO 3 /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_FAST_INTERVAL 40   /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_SLOW_INTERVAL 480  /**< Slow advertising interval (in units of 0.625 ms. This value corresponds to 2 seconds). */
#define APP_ADV_FAST_DURATION 3000 /**< The advertising duration of fast advertising in units of 10 milliseconds. */
#define APP_ADV_SLOW_DURATION 0

#define MIN_CONN_INTERVAL_SLOW         MSEC_TO_UNITS(50, UNIT_1_25_MS)  /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL_SLOW         MSEC_TO_UNITS(100, UNIT_1_25_MS) /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define MIN_CONN_INTERVAL_FAST         MSEC_TO_UNITS(25, UNIT_1_25_MS)  /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL_FAST         MSEC_TO_UNITS(75, UNIT_1_25_MS)  /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                  0                                /**< Slave latency. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000)            /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(30000)           /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT   3                                /**< Number of attempts before giving up the connection parameter negotiation. */
#define CONN_SUP_TIMEOUT               MSEC_TO_UNITS(4000, UNIT_10_MS)  /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE 256 /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256 /**< UART RX buffer size. */

#define RESET_REASON_CODE_POWER_ON     5
#define RESET_REASON_CODE_FACTORY_TEST 9

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT); /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                         /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                           /**< Context for the Queued Write module.*/
BLE_BAS_DEF(m_bas);                               /**< Battery service instance. */
BLE_ADVERTISING_DEF(m_advertising);               /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;      /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3; /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                               /**< Universally unique service identifier. */
    {
        {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE},
        {BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE},
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
};

// BLE state
typedef enum
{
    IDLE,
    CONNECTED,
    ADVERTISING
} BleState;

BleState ble_state = IDLE;

uint8_t get_ble_state(void)
{
    return (uint8_t)ble_state;
}

// Reset reason code
uint16_t reset_reason_code = 0;

// Global CO2 history array
uint16_t co2_history[BAR_COUNT] = {0};

/// whether to stop advertising
bool stop_advertising = false;

/// connection interval tracker
bool     is_fast_interval          = false;
uint16_t fast_interval_timer_ticks = 0;
#define FAST_INTERVAL_SECONDS_MAX 30 // 30 seconds
/**
 * @brief Update CO2 history array with new value
 * @param new_value The new CO2 value to add
 */
void update_co2_history(uint16_t new_value)
{
    // Shift existing values left
    for (int i = BAR_COUNT - 1; i > 0; i--)
    {
        co2_history[i] = co2_history[i - 1];
    }
    co2_history[0] = new_value;
}

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

char *get_ble_name(void)
{
    ble_gap_addr_t addr;
    static char    id[7] = {0};
    static char    name[32];

    if (id[0] != 0) return name; // 调用过一次

    sd_ble_gap_addr_get(&addr);
    sprintf(id, "%02X%02X%02X", addr.addr[3], addr.addr[4], addr.addr[5]);
    sprintf(name, "%s-%s", DEVICE_NAME, id);
    return name;
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)get_ble_name(),
                                          strlen(get_ble_name()));
    APP_ERROR_CHECK(err_code);

    sd_ble_gap_appearance_set(0x123b); // 设置外观为outdoor sensor
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL_SLOW;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL_SLOW;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

void set_fast_interval_timer(void)
{
    fast_interval_timer_ticks = FAST_INTERVAL_SECONDS_MAX;
}

void toggle_connection_interval(void)
{
    if (fast_interval_timer_ticks > 0) return;

    is_fast_interval = !is_fast_interval;

    ret_code_t            err_code;
    ble_gap_conn_params_t gap_conn_params;

    print("toggle connection interval: %d\n", is_fast_interval);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    if (is_fast_interval)
    {
        gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL_FAST;
        gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL_FAST;
    }
    else
    {
        gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL_SLOW;
        gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL_SLOW;
    }

    gap_conn_params.slave_latency    = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_conn_param_update(m_conn_handle, &gap_conn_params);
    APP_ERROR_CHECK(err_code);

    // if it is fast, switch back to slow after 3 minutes
    if (is_fast_interval)
    {
        set_fast_interval_timer();
    }
    else
    {
        fast_interval_timer_ticks = 0;
    }
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Queued Write Module.
 */
static void qwr_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init_obj = {0};

    qwr_init_obj.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing Device Information Service.
 */
static void dis_init(void)
{
    ret_code_t       err_code;
    ble_dis_init_t   dis_init_obj;
    ble_dis_pnp_id_t pnp_id;

    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

    ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, MANUFACTURER_NAME);
    ble_srv_ascii_to_utf8(&dis_init_obj.hw_rev_str, (char *)HARDWARE_REVISION);
    ble_srv_ascii_to_utf8(&dis_init_obj.sw_rev_str, (char *)SOFTWARE_REVISION);
    ble_srv_ascii_to_utf8(&dis_init_obj.fw_rev_str, (char *)FIRMWARE_REVISION);
    dis_init_obj.p_pnp_id = &pnp_id;

    dis_init_obj.dis_char_rd_sec = SEC_JUST_WORKS;

    err_code = ble_dis_init(&dis_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing Battery Service.
 */
static void bas_init(void)
{
    ret_code_t     err_code;
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler          = NULL;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = NULL;
    bas_init_obj.initial_batt_level   = 100;

    bas_init_obj.bl_rd_sec        = SEC_JUST_WORKS;
    bas_init_obj.bl_cccd_wr_sec   = SEC_JUST_WORKS;
    bas_init_obj.bl_report_rd_sec = SEC_JUST_WORKS;

    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
void battery_level_update(uint8_t battery_level)
{
    ret_code_t err_code;

    err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t *p_evt)
{
    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        // uint32_t err_code;

        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

        proto_put_data((uint8_t *)p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
    }
    else if (p_evt->type == BLE_NUS_EVT_TX_RDY)
    {
        EventGroupSetBits(event_group_system, EVT_NUS_TX_RDY);
    }
}
/**@snippet [Handling the data received over BLE] */

static void nus_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t _nus_init;

    // Initialize NUS.
    memset(&_nus_init, 0, sizeof(_nus_init));

    _nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &_nus_init);
    APP_ERROR_CHECK(err_code);
}

void proto_send_frame(uint8_t *frame, uint16_t len)
{
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) return;

    uint32_t err_code = ble_nus_data_send(&m_nus, frame, &len, m_conn_handle);
    if (err_code != NRF_SUCCESS)
    {
        print("nus send frame failed with: %s\n", nrf_strerror_get(err_code));
    }
}

static void advertising_config_get(ble_adv_modes_config_t *p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled  = true;
    p_config->ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_FAST_DURATION;

    p_config->ble_adv_slow_enabled  = true;
    p_config->ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
    p_config->ble_adv_slow_timeout  = APP_ADV_SLOW_DURATION;
}
static void disconnect(uint16_t conn_handle, void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
    }
    else
    {
        NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
    }
}
// YOUR_JOB: Update this code if you want to do anything given a DFU event (optional).
/**@brief Function for handling dfu events from the Buttonless Secure DFU service
 *
 * @param[in]   event   Event from the Buttonless Secure DFU service.
 */
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
    case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
    {
        NRF_LOG_INFO("Device is preparing to enter bootloader mode.");

        // Prevent device from advertising on disconnect.
        ble_adv_modes_config_t config;
        advertising_config_get(&config);
        config.ble_adv_on_disconnect_disabled = true;
        ble_advertising_modes_config_set(&m_advertising, &config);
        add_history_record(1, RECORD_TYPE_DFU);
        EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);

        // Disconnect all other bonded devices that currently are connected.
        // This is required to receive a service changed indication
        // on bootup after a successful (or aborted) Device Firmware Update.
        uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
        NRF_LOG_INFO("Disconnected %d links.", conn_count);
        break;
    }

    case BLE_DFU_EVT_BOOTLOADER_ENTER:
        // YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
        //           by delaying reset by reporting false in app_shutdown_handler
        NRF_LOG_INFO("Device will enter bootloader mode.");
        break;

    case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
        NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
        add_history_record(0, RECORD_TYPE_DFU_FAIL);
        EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        break;

    case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
        NRF_LOG_ERROR("Request to send a response to client failed.");
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        APP_ERROR_CHECK(false);
        break;

    default:
        NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
        break;
    }
}

/** dfu service init */
static void dfu_init(void)
{
    uint32_t err_code;

    // dfu init service
    ble_dfu_buttonless_init_t dfus_init = {0};

    dfus_init.evt_handler = ble_dfu_evt_handler;

    err_code = ble_dfu_buttonless_init(&dfus_init);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    qwr_init();
    dis_init();
    bas_init();
    nus_init();
    dfu_init();
}

/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**关闭蓝牙 */
void set_ble_disconnect(void)
{

    if (m_conn_handle == BLE_CONN_HANDLE_INVALID) return;

    ble_state = IDLE;

    uint32_t err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

    if (err_code != NRF_SUCCESS)
    {
        // 发生错误，处理错误情况
    }
}

/// Added for bonding/
/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
void advertising_start(void)
{
    if (m_conn_handle != BLE_CONN_HANDLE_INVALID || ble_state != IDLE) return;
    print("start advertising\n");

    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    if (err_code != NRF_SUCCESS)
    {
        print("adv start err: %s\n", nrf_strerror_get(err_code));
    }
    APP_ERROR_CHECK(err_code);

    ble_state = ADVERTISING;
    EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
}

// Stop advertising
void advertising_stop(void)
{
    uint32_t err_code;

    if (ble_state == IDLE) return;
    print("stop advertising\n");

    if (ble_state == CONNECTED && m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        stop_advertising = true;
        err_code         = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        print("disconnect err: %s\n", nrf_strerror_get(err_code));
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        err_code = sd_ble_gap_adv_stop(m_advertising.adv_handle);
        print("adv stop err: %s\n", nrf_strerror_get(err_code));
        ble_state = IDLE;
    }
}

// 开始广播
void ble_delete_bonds(void)
{
    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        set_ble_disconnect();
    }
    delete_bonds();
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected");
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        err_code      = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);
        ble_state = CONNECTED;
        EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);

        // // Added for bonding - only initiate security in normal mode, not in factory test
        // if (!EventGroupCheckBits(event_group_system, EVT_FACTORY_TEST_MODE_ACTIVE))
        // {
        //     err_code = pm_conn_secure(p_ble_evt->evt.gap_evt.conn_handle, false);
        //     if (err_code != NRF_ERROR_BUSY)
        //     {
        //         APP_ERROR_CHECK(err_code);
        //     }
        // }
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected");
        // LED indication will be changed when advertising starts.
        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        print("stop_advertising: %d\n", stop_advertising);

        if (stop_advertising)
        {
            err_code = sd_ble_gap_adv_stop(m_advertising.adv_handle);
            print("adv stop err: %s\n", nrf_strerror_get(err_code));
            ble_state        = IDLE;
            stop_advertising = false;
        }
        else
        {
            ble_state = ADVERTISING;
            EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
        }
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        NRF_LOG_DEBUG("PHY update request.");
        ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
        break;
    }

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        // Pairing not supported
        // err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
        // APP_ERROR_CHECK(err_code);
        // pairng_request();
        break;

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        // No system attributes have been stored.
        err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;
        // 连接参数更新
    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        NRF_LOG_INFO("conn_Param Update: %d,%d,%d,%d",
                     p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval,
                     p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval,
                     p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.slave_latency,
                     p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout);
        break;
    case BLE_GAP_EVT_PASSKEY_DISPLAY:
        // Only handle passkey display in normal mode, not in factory test mode
        if (!EventGroupCheckBits(event_group_system, EVT_FACTORY_TEST_MODE_ACTIVE))
        {
            NRF_LOG_INFO("passkey: %c,%c,%c,%c,%c,%c",
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[0],
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[1],
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[2],
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[3],
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[4],
                         p_ble_evt->evt.gap_evt.params.passkey_display.passkey[5]);
            ui_set_passkey((const char *)p_ble_evt->evt.gap_evt.params.passkey_display.passkey);
            EventGroupSetBits(event_group_system, EVT_SHOW_PASSKEY);
        }
        else
        {
            NRF_LOG_INFO("Factory test mode: Passkey display skipped");
        }
        break;
        case BLE_GAP_EVT_AUTH_STATUS:
        // Authentication status - handle differently for factory test mode
        if (p_ble_evt->evt.gap_evt.params.auth_status.auth_status == BLE_GAP_SEC_STATUS_SUCCESS)
        {
            NRF_LOG_INFO("pairing success");
        }
        else
        {
            NRF_LOG_INFO("pairing failed");
            // Only disconnect on auth failure in normal mode, not in factory test mode
            if (!EventGroupCheckBits(event_group_system, EVT_FACTORY_TEST_MODE_ACTIVE))
            {
                sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            }
        }
        
        // Only cancel passkey display in normal mode
        if (!EventGroupCheckBits(event_group_system, EVT_FACTORY_TEST_MODE_ACTIVE))
        {
            EventGroupSetBits(event_group_system, EVT_CHANCEL_SHOW_PASSKEY);
        }
        break;
    default:
        // No implementation needed.
        break;
    }
}

/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code           = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    print("RAM START: %08X\n", ram_start);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t *p_gatt, nrf_ble_gatt_evt_t const *p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}

/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    // uint32_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        break;
    case BLE_ADV_EVT_SLOW:
        break;
    case BLE_ADV_EVT_IDLE:
        // sleep_mode_enter();
        ble_state = IDLE;
        EventGroupSetBits(event_group_system, EVT_UI_UP_BLE);
        break;
    default:
        break;
    }
}

static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = 0;
    init.advdata.uuids_complete.p_uuids  = NULL;

    memset(&init.srdata, 0, sizeof(init.srdata));
    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    // init.config.ble_adv_on_disconnect_disabled     = true;
    init.config.ble_adv_on_disconnect_disabled     = false;
    init.config.ble_adv_whitelist_enabled          = false;
    init.config.ble_adv_directed_high_duty_enabled = false;
    init.config.ble_adv_directed_enabled           = false;
    init.config.ble_adv_directed_interval          = 0;
    init.config.ble_adv_directed_timeout           = 0;
    init.config.ble_adv_fast_enabled               = true;
    init.config.ble_adv_fast_interval              = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout               = APP_ADV_FAST_DURATION;
    init.config.ble_adv_slow_enabled               = true;
    init.config.ble_adv_slow_interval              = APP_ADV_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout               = APP_ADV_SLOW_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{

    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

/// Added for bonding/
/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const *p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
        NRF_LOG_INFO("pm delete bonds, start advertising");
        advertising_start();
        break;
    case PM_EVT_CONN_SEC_CONFIG_REQ:
    {
        NRF_LOG_INFO("PM_EVT_CONN_SEC_CONFIG_REQ: peer_id=%d, accept to fix bonding\r\n",
                     p_evt->peer_id);
        // Accept pairing request from an already bonded peer.

        pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};
        pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
    }
    break;
    case PM_EVT_CONN_SEC_FAILED:
    {
        NRF_LOG_INFO("PM_EVT_CONN_SEC_FAILED: peer_id=%d, procedure=%d, error=0x%04x\r\n",
                     p_evt->peer_id,
                     p_evt->params.conn_sec_failed.procedure,
                     p_evt->params.conn_sec_failed.error);
        if (p_evt->params.conn_sec_failed.procedure == PM_CONN_SEC_PROCEDURE_ENCRYPTION &&
            p_evt->params.conn_sec_failed.error == PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING)
        {
            // Local device lost bond info, don't disconnect and wait for re-bond
            NRF_LOG_INFO("Waiting for host to fix bonding\r\n");
        }
        else
        {
            // sprintf(m_message, "Security procedure failed, disconnect.\r\n");
            // dev_ctrl_send_msg(m_message, strlen(m_message));
            (void)sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        }
    }
    break;
    default:
        break;
    }
}
/// Added for bonding
/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);

    ble_gap_sec_params_t sec_param;

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    sec_param.bond           = SEC_PARAM_BOND;  // Enable bonding
    sec_param.mitm           = 0;               // No MITM => Just Works
    sec_param.lesc           = 0;
    sec_param.keypress       = 0;
    sec_param.io_caps        = BLE_GAP_IO_CAPS_NONE; // No input/output
    sec_param.oob            = 0;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);
}

static void scheduler_init(void)
{
    APP_SCHED_INIT(APP_TIMER_SCHED_EVENT_DATA_SIZE, 32);
}

uint8_t ttask_slow_mode = 0; // ttask 200ms

static void tick_tasks()
{
    TaskTick(task_protocol);
    TaskTick(task_ui);
    TaskTick(task_co2_read);
    TaskTick(task_co2_alarm);
    TaskTick(task_batery);
    TaskTick(task_history_recover);
    TaskTick(task_history_storage);
    TaskTick(task_history_upload);
    TaskTick(task_populate_fake_records);
    TaskTick(task_factory_test);
}

static void tick_factory_test_tasks()
{
    TaskTick(task_protocol);
    TaskTick(task_factory_test);
}

// Add this flag at module level
static volatile bool timer_mode_change_pending = false;
static volatile bool new_timer_mode_slow       = false;

// main loop timer
APP_TIMER_DEF(ttask_timer);
void ttask_timer_timerout_handler(void *pcontext)
{
    static uint32_t drift_correction = 0; // drift correction
    static uint32_t ms               = 0;

    uint32_t increment = (ttask_slow_mode == 0) ? 10 : 200;
    ms += increment;

    if (ms >= 1000)
    {
        ms = 0;
        inc_second(); // Increase the second counter

        // Adjust drift correction based on actual ms increment
        drift_correction += 960; //  Adjust for slow mode timing

        if (drift_correction >= 1000000) // Every ~10 minutes
        {
            print("drift correction applied\n");
            inc_second(); // Increment second counter to account for drift
            drift_correction = 0;
        }

        if (fast_interval_timer_ticks > 0)
        {
            fast_interval_timer_ticks--;
            if (fast_interval_timer_ticks == 0)
            {
                print("fast interval timer timeout, toggle connection interval\n");
                toggle_connection_interval();
            }
        }
    }

    IncSystemTickCount();
    tick_tasks();

    // Apply any pending timer mode change at top of handler
    if (timer_mode_change_pending)
    {
        timer_mode_change_pending = false;
        app_timer_stop(ttask_timer);

        if (new_timer_mode_slow)
        {
            app_timer_start(ttask_timer, APP_TIMER_TICKS(200), NULL);
            ttask_slow_mode = 1;
        }
        else
        {
            app_timer_start(ttask_timer, APP_TIMER_TICKS(10), NULL);
            ttask_slow_mode = 0;
        }
    }
}

void power_on_timer_timeout_handler(void *pcontext)
{
    IncSystemTickCount();
    TaskTick(task_power_on);
}

void factory_test_timer_timeout_handler(void *pcontext)
{
    IncSystemTickCount();
    tick_factory_test_tasks();
}

void run_task(void)
{
    if (reset_reason_code == RESET_REASON_CODE_POWER_ON)
    {
        // print("task_power_on\n");
        TaskRun(task_power_on);
        return;
    }

    if (reset_reason_code == RESET_REASON_CODE_FACTORY_TEST)
    {
        TaskRun(task_protocol);
        TaskRun(task_factory_test);
        return;
    }

    // print("task_protocol\n");
    TaskRun(task_protocol);

    // print("task_ui\n");
    TaskRun(task_ui);

    // print("task_co2_read\n");
    TaskRun(task_co2_read);

    // print("task_co2_alarm\n");
    TaskRun(task_co2_alarm);

    // print("task_batery\n");
    TaskRun(task_batery);

    // print("task_history_storage\n");
    TaskRun(task_history_storage);

    // print("task_history_upload\n");
    TaskRun(task_history_upload);

    TaskRun(task_populate_fake_records);
}

void check_button_and_erase(void)
{
    // set the button as input
    nrf_gpio_cfg_input(BUTTON_0, NRF_GPIO_PIN_PULLUP);

    uint8_t button_0 = nrf_gpio_pin_read(BUTTON_0);
    print("button 0 %d\n", button_0);
    if (button_0 == 0)
    {
        print("button 0 pressed\n");
        cfg_fstorage_erase_all(1);
        // NVIC_SystemReset();
    }
}

void check_reset_reason(void)
{
    uint32_t reset_reason = 0;
    uint32_t reset_code   = 0;

    sd_power_reset_reason_get(&reset_reason);
    sd_power_reset_reason_clr(reset_reason);
    NRF_LOG_INFO("system reset reason %d", reset_reason);

    sd_power_gpregret_get(0, &reset_code);
    sd_power_gpregret_clr(0, reset_code);

    NRF_LOG_INFO("system gpregret %d", reset_code);

    if (reset_code == RESET_REASON_WAKEUP_FROM_OFF)
    {
        reset_reason_code = RESET_REASON_CODE_POWER_ON;
    }
    else if (reset_code == RESET_REASON_FACTORY_TEST)
    {
        reset_reason_code = RESET_REASON_CODE_FACTORY_TEST;
    }
    else
    {
        switch (reset_reason)
        {
        case 1:
            print("Reset from pin-reset detected\n");
            reset_reason_code = 1;
            check_button_and_erase();
            break;
        case (1 << 1):
            print("Reset from watchdog detected\n");
            reset_reason_code = 2;
            break;
        case (1 << 2):
            print("Reset from soft reset detected\n");
            reset_reason_code = 3;
            break;
        case (1 << 3):
            print("Reset from CPU lock-up detected\n");
            reset_reason_code = 4;
            break;
        case (1 << 16):
            print("Reset due to wake up from System OFF mode when wakeup is triggered from GPIO");
            reset_reason_code = 5;
            break;
        case (1 << 17):
            print("Reset due to wake up from System OFF mode when wakeup is triggered from ANADETECT signal from LPCOMP");
            reset_reason_code = 6;
            break;
        case (1 << 18):
            print("Reset due to wake up from System OFF mode when wakeup is triggered from entering into debug interface mode");
            reset_reason_code = 7;
            break;
        case (1 << 19):
            print("Reset due to wake up from System OFF mode by NFC field detect");
            reset_reason_code = 8;
            break;
        default:
            print("Reset from unknown reason detected %8X\n", reset_reason);
            reset_reason_code = 0;
            break;
        }
    }

    // print the current timestamp to see if it is preserved after reset
    print("current timestamp: %u\n", get_time_now());
    if (reset_reason_code == 3)
    {
        set_time_set(true);
    }
    else
    {
        // Reset the timestamp if the reset reason is not soft reset because the timestamp is only preserved for soft reset
        set_timebase(0);
        set_time_set(false);
    }

    // save the reset reason to flash
    add_record(reset_reason_code, RECORD_TYPE_RESET_REASON);
}

nrf_drv_wdt_channel_id m_channel_id;

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(uint16_t bat_vol_mv)
{
    print("sleep_mode_enter\n");
    if (EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING)) return;

    EventGroupClearBits(event_group_system, EVT_SCREEN_FORCE_ON | EVT_CHARGING);
    EventGroupSetBits(event_group_system, EVT_BAT_LOW_WARNING | EVT_SCREEN_ON_ONETIME);
    add_record(bat_vol_mv, RECORD_TYPE_BATTERY_LOW);

    if (cfg_fstorage_get_flight_mode())
    {
        // Disable flight mode and add history record
        cfg_fstorage_set_flight_mode(0);
        update_flight_mode_activation_time();
        add_record(0, RECORD_TYPE_FLIGHT_MODE);
    }

    EventGroupSetBits(event_group_system, EVT_CO2_UP_HIS);

    advertising_stop();

    app_timer_stop(ttask_timer);
    ttask_slow_mode = 1;

    fast_interval_timer_ticks = 0;
    is_fast_interval          = false;

    app_timer_start(ttask_timer, APP_TIMER_TICKS(200), NULL);
}

void sleep_mode_exit(void)
{
    print("sleep_mode_exit called, ttask_slow_mode: %d\n", ttask_slow_mode);
    if (!EventGroupCheckBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING)) return;

    ttask_slow_mode = 0;
    advertising_start();
    user_init();
    EventGroupClearBits(event_group_system, EVT_BAT_LOW | EVT_BAT_LOW_WARNING);
    TaskStart(task_co2_read);
    TaskStart(task_history_storage);
    TaskStart(task_co2_alarm);
    // Power up peripherals
    app_timer_stop(ttask_timer);
    app_timer_start(ttask_timer, APP_TIMER_TICKS(10), NULL);
}

void change_timer_power_mode(bool is_sleep)
{
    timer_mode_change_pending = true;
    new_timer_mode_slow       = is_sleep;
}

void system_off()
{
    print("Enter System OFF mode\n");

    // Configure the wake-up pin
    nrf_gpio_cfg_sense_input(BUTTON_0, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

    sd_power_gpregret_set(0, RESET_REASON_WAKEUP_FROM_OFF);

    NRF_POWER->SYSTEMOFF = 1; // Enter System OFF mode

    // reset the system off flag
    EventGroupClearBits(event_group_system, EVT_DEVICE_SLEEP);

    // Enter System OFF mode (this function will not return; wake-up will cause a reset)
    sd_power_system_off();
}

void sleep_device(void)
{
    app_timer_pause();
    advertising_stop();
    app_timer_resume();

    app_timer_stop_all();
    button_sleep_mode_enter();

    nrf_gpio_pin_clear(PIN_VIBRATOR);
    nrf_gpio_pin_clear(PIN_BUZZER);
    co2_twi_uninit();
    nrf_gpio_pin_clear(PIN_CO2_PWR);
    nrf_gpio_pin_clear(PIN_LCD_PWR);

    NRF_WDT->CONFIG &= ~WDT_CONFIG_SLEEP_Msk; // Disable WDT in sleep mode
    nrf_drv_wdt_channel_feed(m_channel_id);   // Last feed before sleep

    system_off();
}

void start_factory_test(void)
{
    print("factory_test_mode_enter\n");

    app_timer_pause();
    advertising_stop();
    app_timer_resume();
    app_timer_stop_all();

    sd_power_gpregret_set(0, RESET_REASON_FACTORY_TEST);

    nrf_drv_wdt_channel_feed(m_channel_id); // Last feed before reset

    print("Resetting to factory test mode...\n");

    sd_nvic_SystemReset();
}

static uint32_t button_held_time = 0;

void init(void)
{
    // Initialize all required subsystems for factory test mode

    timers_init();
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();

    services_init();
    advertising_init();
    conn_params_init();

    scheduler_init();

    cfg_fstorage_init();
    cfg_fstorage_load();

    user_init();

    check_reset_reason();

    peer_manager_init();

    nrf_drv_wdt_config_t config   = NRF_DRV_WDT_DEAFULT_CONFIG;
    uint32_t             err_code = nrf_drv_wdt_init(&config, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
    APP_ERROR_CHECK(err_code);
    nrf_drv_wdt_enable();
}

void exec_main_loop(void)
{
    for (;;)
    {
        nrf_drv_wdt_channel_feed(m_channel_id);
        run_task();
        app_sched_execute();
        idle_state_handle();
    }
}

void start_timer_and_main_loop()
{
    app_timer_stop(ttask_timer);
    app_timer_start(ttask_timer, APP_TIMER_TICKS(10), NULL);

    button_init();

    exec_main_loop();
}

void init_startup(void)
{

    app_timer_create(&ttask_timer, APP_TIMER_MODE_REPEATED, power_on_timer_timeout_handler);
    start_timer_and_main_loop();
}

void init_system_on(void)
{

    app_timer_create(&ttask_timer, APP_TIMER_MODE_REPEATED, ttask_timer_timerout_handler);

    if (cfg_fstorage_get_bluetooth_enabled() == 1)
    {
        print("start advertising from main\n");
        advertising_start();
    }

    start_timer_and_main_loop();
}

void init_factory_test(void)
{
    app_timer_create(&ttask_timer, APP_TIMER_MODE_REPEATED, factory_test_timer_timeout_handler);

    advertising_start();

    start_timer_and_main_loop();
}

int main(void)
{
    log_init();

    init();

    if (reset_reason_code == RESET_REASON_CODE_POWER_ON)
    {
        init_startup();
    }
    else if (reset_reason_code == RESET_REASON_CODE_FACTORY_TEST)
    {
        init_factory_test();
    }
    else
    {
        init_system_on();
    }
}

/**
 * @}
 */
