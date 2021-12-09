/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_BT
 * @{
 * This section introduces the APP BT module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_BT__H_
#define _APP_BT__H_

#include <types.h>
#include "string.h"
#include "stdio.h"
#include "bt_rpc_api.h"
#include "userapp_dbglog.h"

#define DBGLOG_BT_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[bt] " fmt, ##__VA_ARGS__)
#define DBGLOG_BT_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[bt] " fmt, ##__VA_ARGS__)

#define BT_AUTH_RESULT_AUTH_FAIL 0x05
#define BT_AUTH_RESULT_KEY_MISS  0x06

#define BT_DISCONNECT_REASON_CONN_TIMEOUT           0x04
#define BT_DISCONNECT_REASON_AUTH_FAIL              0x05
#define BT_DISCONNECT_REASON_KEY_MISS               0x06
#define BT_DISCONNECT_REASON_LINK_LOSS              0x08
#define BT_DISCONNECT_REASON_ALREADY_EXISTS         0x0B
#define BT_DISCONNECT_REASON_INSUFFICIENT_RESOURCES 0x0D
#define BT_DISCONNECT_REASON_REMOTE                 0x13
#define BT_DISCONNECT_REASON_REMOTE_POWER_OFF       0x15
#define BT_DISCONNECT_REASON_LOCAL                  0x16
#define BT_DISCONNECT_REASON_LMP_TIMEOUT            0x22
#define BT_DISCONNECT_REASON_TWS_PAIRING            0xF1
#define BT_DISCONNECT_REASON_SWITCHED_SLAVE         0xF2

#define STATE_DISABLED         BIT(0)    //0x01 bt not enabled
#define STATE_WWS_PAIRING      BIT(1)    //0x02 wws pairing, no action is allowed except power off
#define STATE_IDLE             BIT(2)    //0x04 !visible + !connectable
#define STATE_CONNECTABLE      BIT(3)    //0x08 !visible + connectable
#define STATE_AG_PAIRING       BIT(4)    //0x10 visible + connectable
#define STATE_CONNECTED        BIT(5)    //0x20 connected
#define STATE_A2DP_STREAMING   BIT(6)    //0x40 a2dp playing
#define STATE_INCOMING_CALL    BIT(7)    //0x80 incomming call
#define STATE_OUTGOING_CALL    BIT(8)    //0x100 outgoing call
#define STATE_ACTIVE_CALL      BIT(9)    //0x200 active call
#define STATE_TWC_CALL_WAITING BIT(10)   //0x400 three way call waiting
#define STATE_TWC_CALL_ON_HELD BIT(11)   //0x800 three way call on held
#define STATE_CUSTOM_1         BIT(14)   //0x4000 cusomized state 1
#define STATE_CUSTOM_2         BIT(15)   //0x8000 cusomized state 2

typedef struct {
    uint8_t crc_err_rate;
    uint8_t seq_err_rate;
    int8_t plink_rssi_avg;
    int8_t rlink_rssi_avg;
} link_quality_t;

/**
 * @brief check if two bluetooth address is same
 *
 * @param addr1 first bluetooth address
 * @param addr2 second bluetooth address
 *
 * @return true if same, false if not
 */
static inline bool_t bdaddr_is_equal(const BD_ADDR_T *addr1, const BD_ADDR_T *addr2)
{
    if ((!addr1) || (!addr2)) {
        return false;
    }

    if (!memcmp(addr1, addr2, sizeof(BD_ADDR_T))) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief check if a bluetooth address is all zero
 *
 * @param addr the bluetooth address
 *
 * @return true if zero, false if not
 */
static inline bool_t bdaddr_is_zero(const BD_ADDR_T *addr)
{
    BD_ADDR_T zero_addr = {0};
    if (!addr) {
        return true;
    }

    return bdaddr_is_equal(addr, &zero_addr);
}

/**
 * @brief convert a string to bluetooth address
 *
 * @param str the string to convert
 * @param addr pointer to store the result
 *
 * @return true if convert succeed, false if not
 */
static inline bool_t str2bdaddr(const char *str, BD_ADDR_T *addr)
{
    char ch;
    uint8_t value;

    assert(addr);
    assert(str);

    if (strlen(str) != 12) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        ch = str[2 * i];
        if ((ch >= '0') && (ch <= '9')) {
            value = ch - '0';
        } else if ((ch >= 'a') && (ch <= 'f')) {
            value = ch - 'a' + 0xa;
        } else if ((ch >= 'A') && (ch <= 'F')) {
            value = ch - 'A' + 0xa;
        } else {
            return false;
        }
        value <<= 4;

        ch = str[2 * i + 1];
        if ((ch >= '0') && (ch <= '9')) {
            value += ch - '0';
        } else if ((ch >= 'a') && (ch <= 'f')) {
            value += ch - 'a' + 0xa;
        } else if ((ch >= 'A') && (ch <= 'F')) {
            value += ch - 'A' + 0xa;
        } else {
            return false;
        }

        addr->addr[5 - i] = value;
    }

    return true;
}

/**
 * @brief convert a bluetooth address to string
 *
 * @param addr the bluetooth address to convert
 * @param str pointer to store the result, length >= 13
 *
 * @return true if convert succeed, false if not
 */
static inline bool_t bdaddr2str(const BD_ADDR_T *addr, char *str)
{
    int len;

    assert(addr);
    assert(str);

    len = snprintf(str, 13, "%02X%02X%02X%02X%02X%02X", addr->addr[5], addr->addr[4], addr->addr[3],
                   addr->addr[2], addr->addr[1], addr->addr[0]);

    return (len == 12);
}

/**
 * @brief print the bluetooth address
 *
 * @param addr bluetooth address to print
 */
void bdaddr_print(BD_ADDR_T *addr);

/**
 * @brief init the app bt module
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_init(void);

/**
 * @brief deinit the app bt module
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_deinit(void);

/**
 * @brief reset the bt module, clear all saved data
 *
 * @param keep_peer_addr true if keep peer address, false if clear peer address
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_factory_reset(bool_t keep_peer_addr);

/**
 * @brief power on the bt module
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_power_on(void);

/**
 * @brief power off the bt module
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_power_off(void);

/**
 * @brief connect to last connected device
 *
 * @param addr the device to connect
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_connect(BD_ADDR_T *addr);

/**
 * @brief disconnect the current connection
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_disconnect(void);

/**
 * @brief disconnect all connection and clear all paired devices
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_clear_pair_list(void);

/**
 * @brief enter the state STATE_AG_PAIRING which the
 *        earbud can be found and connected by phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_enter_ag_pairing(void);

/**
 * @brief activate the voice recognition on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_activate_voice_recognition(void);

/**
 * @brief toggle the voice recognition on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_toggle_voice_recognition(void);

/**
 * @brief close the voice recognition on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_deactivate_voice_recognition(void);

/**
 * @brief redail the last call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_redail(void);

/**
 * @brief answer the incomming call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_answer(void);

/**
 * @brief reject the incomming call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_reject(void);

/**
 * @brief hangup the current call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_hangup(void);

/**
 * @brief three way call action, reject the waiting call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_twc_reject_waiting(void);

/**
 * @brief three way call action, release current call and accept the held or waiting call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_twc_release_active_accept_held_waiting(void);

/**
 * @brief three way call action, hold current call and accept the held or waiting call
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_twc_hold_active_accept_held_waiting(void);

/**
 * @brief transfer the call audio between earbud and phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_transfer_toggle(void);

/**
 * @brief transfer the call audio to earbud
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_connect_sco(void);

/**
 * @brief transfer the call audio to phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_disconnect_sco(void);

/**
 * @brief play/pause the music on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_play_pause(void);

/**
 * @brief play the music on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_play(void);

/**
 * @brief pause the music on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_pause(void);

/**
 * @brief play the next music on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_forward(void);

/**
 * @brief play the last music on phone
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_backward(void);

/**
 * @brief report current call volume to phone
 *
 * @param level 0-15
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_report_call_volume(uint8_t level);

/**
 * @brief report current music volume to phone
 *
 * @param level 0-15
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_report_music_volume(uint8_t level);

/**
 * @brief report battery level to phone
 *
 * @param level 0-100
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_report_battery_level(uint8_t level);

/**
 * @brief enter factory test mode
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_enter_ft_mode(void);

/**
 * @brief enter audio test mode
 *
 * @param close_bt close bluetooth or not
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_enter_audio_test_mode(bool_t close_bt);

/**
 * @brief enter dut mode
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_enter_dut_mode(void);

/**
 * @brief check if bt is in dut mode
 *
 * @return true for dut mode, false for normal mode
 */
bool_t app_bt_is_in_dut_mode(void);

/**
 * @brief check if bt is in factory test mode
 *
 * @return true for factory test mode, false for normal mode
 */
bool_t app_bt_is_in_ft_mode(void);

/**
 * @brief check if bt is in audio test mode
 *
 * @return true for factory test mode, false for normal mode
 */
bool_t app_bt_is_in_audio_test_mode(void);

/**
 * @brief get current system state of bt
 *
 * @return current state, see STATE_XXX for detail
 */
uint32_t app_bt_get_sys_state(void);

/**
 * @brief get current a2dp state
 *
 * @return @see bt_a2dp_state_t for detail
 */
bt_a2dp_state_t app_bt_get_a2dp_state(void);

/**
 * @brief get current avrcp state
 *
 * @return @see bt_avrcp_state_t for detail
 */
bt_avrcp_state_t app_bt_get_avrcp_state(void);

/**
 * @brief get current hfp state
 *
 * @return @see bt_hfp_state_t for detail
 */
bt_hfp_state_t app_bt_get_hfp_state(void);

/**
 * @brief check if sco connected
 *
 * @return true if sco connected, false if not
 */
bool_t app_bt_is_sco_connected(void);

/**
 * @brief get current connected device
 *
 * @param addr buffer to save the returned device address
 *
 * @return int 0 for success, else for the error reason
 */
int app_bt_get_connected_device(BD_ADDR_T *addr);

/**
 * @brief get local bluetooth address
 *
 * @return local bluetooth address
 */
const BD_ADDR_T *app_bt_get_local_address(void);

/**
 * @brief get local device name
 *
 * @return local device name
 */
const char *app_bt_get_local_name(void);

/**
 * @brief check if discoverable
 *
 * @return true if discoverable, false if not
 */
bool_t app_bt_is_discoverable(void);

/**
 * @brief check if connectable
 *
 * @return true if connectable, false if not
 */
bool_t app_bt_is_connectable(void);

/**
 * @brief check if bt disable is in progress
 *
 * @return true if bt disable is running, false if not
 */
bool_t app_bt_is_disabling(void);

/**
 * @brief set local bluetooth name
 *
 * @param name new local name
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_local_name(const char *name);

/**
 * @brief force set tws peer bluetooth address
 *
 * @param addr new peer bluetooth address
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_peer_addr(const BD_ADDR_T *addr);

/**
 * @brief set bluetooth visible
 *
 * @param discoverable discoverable or not
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_discoverable(bool_t discoverable);

/**
 * @brief set bluetooth connectable
 *
 * @param connectable connectable or not
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_connectable(bool_t connectable);

/**
 * @brief set bluetooth visible and connectable
 *
 * @param discoverable discoverable or not
 * @param connectable connectable or not
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_discoverable_and_connectable(bool_t discoverable, bool_t connectable);

/**
 * @brief set bluetooth enabel 3M EDR
 *
 * @param enable enable or not
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_3m_feature_enable(bool_t enable);

/**
 * @brief use fix tx power for BT
 *
 * @param type: always 0
 * @param level: the tx power level range: 0, 1, 2
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_set_fix_tx_power(uint8_t type, uint8_t level);

/**
 * @brief send rpc command to bt core
 *
 * @param cmd the command to send
 * @param param parameter of the command
 * @param param_len lenght of the parameter
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_send_rpc_cmd(uint32_t cmd, void *param, uint32_t param_len);

/**
 * @brief send rpc command to start tws pair
 *
 * @param vid vendor id
 * @param pid product id
 * @param magic magic number
 * @param timeout tws pair timeout in millisecond
 *
 * @return 0 for success, else for the error reason
 */
int app_bt_send_tws_pair_cmd(uint16_t vid, uint16_t pid, uint8_t magic, uint32_t timeout);

/**
 * @brief private function to handle peer visibility changed event
 */
void app_bt_handle_peer_visibility_changed(void);

/**
 * @brief get recognition if activated
 *
 * @return false:not activated true:activated
 */
bool_t app_bt_is_recognition_activated(void);

/**
 * @brief get if in sniff mode
 *
 * @return false:other connection mode  true:in sniff mode
 */
bool_t app_bt_is_in_sniff_mode(void);

/**
 * @brief force discoverable when phone connection not exists
 *
 * @param force_discoverable true if force discoverable, false if not
 */
void app_bt_set_force_discoverable(bool_t force_discoverable);

/**
 * @brief get the link quality of each connection
 *
 * @return link quality of connections
 */
const link_quality_t *app_bt_get_link_quality(void);

/**
 * @brief update the connection parameters of a connection
 *
 * @param addr connection addr
 * @param conn_interval_min Minimum value for the connection event interval
 *                          Range: 0x0006 to 0x0C80
 *                          Time = N * 1.25 msec
 * @param conn_interval_max Maximum value for the connection event interval
 *                          Range: 0x0006 to 0x0C80
 *                          Time = N * 1.25 msec
 * @param conn_latency Slave Latency in number of connection events
 *                          Range: 0x0000 to 0x01F3
 * @param supervision_timeout connection supervison timeout for the connection
 *                            Range: 0x000A to 0x0C80
 *                            Time = N * 10 msec
 *
 */
void app_bt_update_le_conn_param(BD_ADDR_T *addr, uint16_t conn_interval_min,
                                 uint16_t conn_interval_max, uint16_t conn_latency,
                                 uint16_t supervision_timeout);

/**
 * @}
 * addtogroup APP_BT
 */

/**
 * @}
 * addtogroup APP
 */

#endif
