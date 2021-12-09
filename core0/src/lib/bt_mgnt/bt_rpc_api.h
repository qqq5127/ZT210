#ifndef _BT_RPC_API_H_
#define _BT_RPC_API_H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup BT_MGNT
 * @{
 * This section introduces the LIB BT_MGNT module's enum, structure, functions and how to use this module.
 */

#include "types.h"

#define MAX_NAME_LEN        32
#define MAX_PIN_CODE_LEN    16
#define MAX_HFP_CMD_LEN     64
#define MAX_DIAL_NUMBER_LEN 32
#define MAX_VERSION_EXT_LEN 64
#define MAX_PAIR_LIST       10
#define MAX_CONNECTION      3
#define MAX_HFP_VOLUME      15
#define MAX_AVRCP_VOLUME    127

#define IN
#define OUT

#define BLE_ADDR_TYPE_PUBLIC 0
#define BLE_ADDR_TYPE_RANDOM 1

#define GATT_LINK_TYPE_NONE  0
#define GATT_LINK_TYPE_BREDR 1
#define GATT_LINK_TYPE_BLE   2

/** @defgroup lib_bt_mgmt_enum Enum
 * @{
 */

typedef enum {
    BT_RESULT_SUCCESS,
    BT_RESULT_DISABLED,
    BT_RESULT_NOT_EXISTS,
    BT_RESULT_ALREADY_EXISTS,
} bt_result_t;

/******************************** commands *******************************/

typedef enum {
    BT_CMD_UNUSED,

    BT_CMD_SET_ENABLED,
    BT_CMD_FACTORY_RESET,
    BT_CMD_IS_ENABLED,
    BT_CMD_GET_VERSION,
    BT_CMD_ENTER_FT_MODE,
    BT_CMD_ENTER_DUT_MODE,
    BT_CMD_SET_TX_PWR_LEVEL,
    BT_CMD_SET_RX_GAIN,
    BT_CMD_SET_3M_FEATURE_ENABLE,
    //rpc commands which support cli should be added here

    BT_CMD_GET_CONNECTION_STATE,
    BT_CMD_CONNECT,
    BT_CMD_DISCONNECT,
    BT_CMD_SET_VISIBILITY,
    BT_CMD_SET_LOCAL_NAME,
    BT_CMD_SET_PIN_CODE,
    BT_CMD_SET_COD,
    BT_CMD_SET_SSP_ENABLED,
    BT_CMD_REMOVE_PAIR,
    BT_CMD_CLEAR_PAIR_LIST,
    BT_CMD_GET_LOCAL_ADDR,
    BT_CMD_GET_LOCAL_NAME,
    BT_CMD_GET_PIN_CODE,
    BT_CMD_GET_COD,
    BT_CMD_GET_SSP_ENABLED,
    BT_CMD_GET_REMOTE_NAME,
    BT_CMD_GET_CONNECTED_DEVICES,
    BT_CMD_GET_PAIRED_DEVICES,
    BT_CMD_REQUEST_LINK_QUALITY_REPORT,

    BT_CMD_SDP_ADD_UUID,
    BT_CMD_SDP_GET_UUID_INDEX,
    BT_CMD_SDP_REGISTER_RECORD,

    BT_CMD_HFP_GET_STATE,
    BT_CMD_HFP_SET_ENABLED,
    BT_CMD_HFP_DIAL,
    BT_CMD_HFP_SEND_DTMF,
    BT_CMD_HFP_REDIAL,
    BT_CMD_HFP_ANSWER,
    BT_CMD_HFP_REJECT,
    BT_CMD_HFP_HANGUP,
    BT_CMD_HFP_ACTIVATE_VOICE_RECOGNITION,
    BT_CMD_HFP_DEACTIVATE_VOICE_RECOGNITION,
    BT_CMD_HFP_CONNECT_SCO,
    BT_CMD_HFP_DISCONNECT_SCO,
    BT_CMD_HFP_TWC_REJECT_WAITING,
    BT_CMD_HFP_TWC_RELEASE_ACTIVE_ACCEPT_WAITING,
    BT_CMD_HFP_TWC_HOLD_ACTIVE_ACCEPT_WAITING,
    BT_CMD_HFP_TWC_CONFERENCE,
    BT_CMD_HFP_REPORT_VOLUME,
    BT_CMD_HFP_REPORT_BATTERY_LEVEL,
    BT_CMD_HFP_SEND_COMMAND,

    BT_CMD_A2DP_GET_STATE,
    BT_CMD_A2DP_SET_ENABLED,

    BT_CMD_AVRCP_GET_STATE,
    BT_CMD_AVRCP_SET_ENABLED,
    BT_CMD_AVRCP_PLAY,
    BT_CMD_AVRCP_PAUSE,
    BT_CMD_AVRCP_STOP,
    BT_CMD_AVRCP_FORWARD,
    BT_CMD_AVRCP_BACKWARD,
    BT_CMD_AVRCP_REPORT_VOLUME,

    BT_CMD_TWS_GET_STATE,
    BT_CMD_TWS_SET_ENABLED,
    BT_CMD_TWS_SET_DEFAULT_ROLE,
    BT_CMD_TWS_START_PAIR,
    BT_CMD_TWS_ROLE_SWITCH,
    BT_CMD_TWS_GET_PEER_ADDR,
    BT_CMD_TWS_SEND_DATA,
    BT_CMD_TWS_SET_TDS_DATA,
    BT_CMD_TWS_SET_AUTO_ROLE_SWITCH_ENABLED,
    BT_CMD_TWS_SET_PEER_ADDR,
    BT_CMD_TWS_ENTER_SINGLE_MODE,

    BT_CMD_LE_SET_ADV_PARAM,
    BT_CMD_LE_SET_ADV_DATA,
    BT_CMD_LE_SET_SCAN_RESPONSE,
    BT_CMD_LE_SET_ADV_ENABLED,
    BT_CMD_LE_SET_RANDOM_ADDR,
    BT_CMD_LE_CONN_UPDATE_PARAM,

    BT_CMD_L2CAP_GET_STATE,
    BT_CMD_L2CAP_REGISTER,
    BT_CMD_L2CAP_CONNECT,
    BT_CMD_L2CAP_DISCONNECT,
    BT_CMD_L2CAP_SEND,

    BT_CMD_SPP_GET_STATE,
    BT_CMD_SPP_REGISTER,
    BT_CMD_SPP_REGISTER_UUID128,
    BT_CMD_SPP_CONNECT,
    BT_CMD_SPP_DISCONNECT,
    BT_CMD_SPP_SEND,

    BT_CMD_GATT_CLIENT_GET_STATE,
    BT_CMD_GATT_CLIENT_CONNECT,
    BT_CMD_GATT_CLIENT_DISCONNECT,
    BT_CMD_GATT_CLIENT_DISCOVER,
    BT_CMD_GATT_CLIENT_DISCOVER_UUID128,
    BT_CMD_GATT_CLIENT_READ,
    BT_CMD_GATT_CLIENT_WRITE,

    BT_CMD_GATT_SERVER_GET_STATE,
    BT_CMD_GATT_SERVER_SET_ENABED,
    BT_CMD_GATT_SERVER_REGISTER_SERVICE,
    BT_CMD_GATT_SERVER_REGISTER_SERVICE_UUID128,
    BT_CMD_GATT_SERVER_REGISTER_CHARACTERISTIC,
    BT_CMD_GATT_SERVER_REGISTER_CHARACTERISTIC_UUID128,
    BT_CMD_GATT_SERVER_NOTIFY,
    BT_CMD_GATT_SERVER_INDICATE,
    BT_CMD_GATT_SERVER_SEND_READ_RESPONSE,

    BT_CMD_SET_HCI_EVT_REPORT_ENABLED,
    BT_CMD_SEND_HCI_COMMAND,
    BT_CMD_SEND_HCI_DATA,

    BT_CMD_SYNC_PLAY_TONE_BY_SN,
    BT_CMD_SYNC_PLAY_TONE_BY_BTCLK,
    BT_CMD_SET_BT_TRIGGER_TS,
    BT_CMD_SYNC_CANCEL_TONE,
    BT_CMD_SYNC_LED_ACTION,

    BT_CMD_LAST
} bt_cmd_t;

/******************************** events *******************************/

typedef enum {
    BT_EVT_UNUSED,

    BT_EVT_ENABLE_STATE_CHANGED,
    BT_EVT_FACTORY_RESET_DONE,
    BT_EVT_ENTER_DUT_MODE,
    BT_EVT_HW_ERROR,

    BT_EVT_CONNECTION_STATE_CHANGED,
    BT_EVT_CONNECTION_MODE_CHANGED,
    BT_EVT_VISIBILITY_CHANGED,
    BT_EVT_LOCAL_NAME_CHANGED,
    BT_EVT_COD_CHANGED,
    BT_EVT_PIN_CODE_CHANGED,
    BT_EVT_PAIR_ADDED,
    BT_EVT_PAIR_REMOVED,
    BT_EVT_AUTH_RESULT,
    BT_EVT_PHONE_TYPE_REPORT,

    BT_EVT_SDP_RFCOMM_FOUND,

    BT_EVT_HFP_STATE_CHANGED,
    BT_EVT_HFP_RING,
    BT_EVT_HFP_SCO_STATE_CHANGED,
    BT_EVT_HFP_SIGNAL_CHANGED,
    BT_EVT_HFP_BATTERY_LEVEL_CHANGED,
    BT_EVT_HFP_VOLUME_CHANGED,
    BT_EVT_HFP_VOICE_RECOGNITION_STATE_CHANGED,
    BT_EVT_HFP_EXTENDED_AT,

    BT_EVT_A2DP_STATE_CHANGED,

    BT_EVT_AVRCP_STATE_CHANGED,
    BT_EVT_AVRCP_VOLUME_CHANGED,
    BT_EVT_AVRCP_VOLUME_UP,
    BT_EVT_AVRCP_VOLUME_DOWN,
    BT_EVT_AVRCP_ABSOLUTE_VOLUME_ENABLED,

    BT_EVT_TWS_STATE_CHANGED,
    BT_EVT_TWS_ROLE_CHANGED,
    BT_EVT_TWS_RECV_DATA,
    BT_EVT_TWS_TDS_DATA,
    BT_EVT_TWS_PAIR_RESULT,
    BT_EVT_TWS_MODE_CHANGED,

    BT_EVT_L2CAP_STATE_CHANGED,
    BT_EVT_L2CAP_DATA,

    BT_EVT_SPP_STATE_CHANGED,
    BT_EVT_SPP_REGISTERED,
    BT_EVT_SPP_UUID128_REGISTERED,
    BT_EVT_SPP_DATA,

    BT_EVT_GATT_CLIENT_STATE_CHANGED,
    BT_EVT_GATT_CLIENT_SERVICE_FOUND,
    BT_EVT_GATT_CLIENT_SERVICE_UUID128_FOUND,
    BT_EVT_GATT_CLIENT_CHARACTERISTIC_FOUND,
    BT_EVT_GATT_CLIENT_CHARACTERISTIC_UUID128_FOUND,
    BT_EVT_GATT_CLIENT_DISCOERY_DONE,
    BT_EVT_GATT_CLIENT_DATA_NOTIFY,
    BT_EVT_GATT_CLIENT_READ_RESPONSE,
    BT_EVT_GATT_CLIENT_MTU_CHANGED,

    BT_EVT_GATT_SERVER_STATE_CHANGED,
    BT_EVT_GATT_SERVER_SERVICE_REGISTERED,
    BT_EVT_GATT_SERVER_SERVICE_UUID128_REGISTERED,
    BT_EVT_GATT_SERVER_CHARACTERISTIC_REGISTERED,
    BT_EVT_GATT_SERVER_CHARACTERISTIC_UUID128_REGISTERED,
    BT_EVT_GATT_SERVER_WRITE,
    BT_EVT_GATT_SERVER_READ,
    BT_EVT_GATT_SERVER_MTU_CHANGED,

    BT_EVT_LE_CONN_PARAM_CHANGED,

    BT_EVT_HCI_EVT_REPORT,
    BT_EVT_HCI_DATA_RECEIVED,

    BT_EVT_MULTIPOINT_PRIMARY_CHANGED,

    BT_EVT_LAST,
} bt_evt_t;

typedef enum {
    CONNECTION_STATE_DISABLED,
    CONNECTION_STATE_DISCONNECTED,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_CONNECTED,
} bt_connection_state_t;

typedef enum {
    CONNECTION_MODE_ACTIVE = 0,
    CONNECTION_MODE_HOLD,
    CONNECTION_MODE_SNIFF,
    CONNECTION_MODE_PARK,
} bt_connection_mode_t;

typedef enum {
    HFP_STATE_DISABLED,
    HFP_STATE_DISCONNECTED,
    HFP_STATE_CONNECTING,
    HFP_STATE_CONNECTED,
    HFP_STATE_INCOMING_CALL,
    HFP_STATE_OUTGOING_CALL,
    HFP_STATE_ACTIVE_CALL,
    HFP_STATE_TWC_CALL_WAITING,
    HFP_STATE_TWC_CALL_ON_HELD,
    HFP_STATE_ON_HELD_NO_ACTIVE
} bt_hfp_state_t;

typedef enum {
    HFP_CODEC_UNKNOWN,
    HFP_CODEC_CVSD,
    HFP_CODEC_MSBC,
    HFP_CODEC_MAX,
} bt_hfp_codec_t;

typedef enum {
    A2DP_STATE_DISABLED,
    A2DP_STATE_DISCONNECTED,
    A2DP_STATE_CONNECTING,
    A2DP_STATE_CONNECTED,
    A2DP_STATE_STREAMING
} bt_a2dp_state_t;

typedef enum {
    A2DP_CODEC_UNKNOWN,
    A2DP_CODEC_SBC,
    A2DP_CODEC_AAC,
    A2DP_CODEC_MAX,
} bt_a2dp_codec_t;

typedef enum {
    AVRCP_STATE_DISABLED,
    AVRCP_STATE_DISCONNECTED,
    AVRCP_STATE_CONNECTING,
    AVRCP_STATE_CONNECTED,
    AVRCP_STATE_PLAYING,
} bt_avrcp_state_t;

typedef enum {
    TWS_STATE_DISABLED,
    TWS_STATE_DISCONNECTED,
    TWS_STATE_PAIRING,
    TWS_STATE_CONNECTING,
    TWS_STATE_CONNECTED,
    TWS_STATE_ROLE_SWITCHING
} bt_tws_state_t;

typedef enum {
    TWS_ROLE_UNKNOWN,   //
    TWS_ROLE_MASTER,    //
    TWS_ROLE_SLAVE      //
} bt_tws_role_t;

typedef enum {
    TWS_CHANNEL_LEFT,    //
    TWS_CHANNEL_RIGHT,   //
    TWS_CHANNEL_STEREO   //
} bt_tws_channel_t;

typedef enum {
    L2CAP_STATE_DISABLED,
    L2CAP_STATE_DISCONNECTED,
    L2CAP_STATE_CONNECTING,
    L2CAP_STATE_CONNECTED,
} bt_l2cap_state_t;

typedef enum {
    SPP_STATE_DISABLED,
    SPP_STATE_DISCONNECTED,
    SPP_STATE_CONNECTING,
    SPP_STATE_CONNECTED,
} bt_spp_state_t;

typedef enum {
    GATT_STATE_DISABLED,
    GATT_STATE_DISCONNECTED,
    GATT_STATE_CONNECTING,
    GATT_STATE_CONNECTED,
} bt_gatt_state_t;

typedef enum {
    BT_PHONE_TYPE_UNKNOWN,
    BT_PHONE_TYPE_IOS,
    BT_PHONE_TYPE_OTHER,
} bt_phone_type_t;

/**
 * @}
 */

/** @defgroup lib_bt_mgmt_struct Struct
 * @{
 */
typedef struct {
    uint8_t addr[6];
} BD_ADDR_T;

/******************************* general api **********************/
typedef struct {
    IN bool_t enabled;
} bt_cmd_set_enabled_t;

typedef struct {
    IN bool_t keep_peer_addr;
} bt_cmd_factory_reset_t;

typedef struct {
    OUT bool_t enabled;
} bt_cmd_get_enabled_t;

typedef struct {
    OUT uint32_t major;
    OUT uint32_t minor;
    OUT char ext[MAX_VERSION_EXT_LEN];
} bt_cmd_get_version_t;

typedef struct {
} bt_cmd_enter_ft_mode_t;

typedef struct {
} bt_cmd_enter_dut_mode_t;

/************ events **********/

typedef struct {
    bool_t enabled;
} bt_evt_enable_state_changed_t;

typedef struct {

} bt_evt_factory_reset_done_t;

typedef struct {
} bt_evt_enter_dut_mode_t;

typedef struct {
    uint8_t error_code;
} bt_evt_hw_error_t;

/******************************* gap api **********************/
typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_connection_state_t state;
} bt_cmd_get_connection_state_t;

typedef struct {
    IN BD_ADDR_T addr;
    bt_a2dp_codec_t prefered_a2dp_codec;
} bt_cmd_connect_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_disconnect_t;

typedef struct {
    IN bool_t visible;
    IN bool_t connectable;
} bt_cmd_set_visibility_t;

typedef struct {
    IN char power_type;
    IN char power_lvl;
} bt_cmd_set_tx_pwr_level_t;

typedef struct {
    IN char fix_gain;
    IN char gain_lna;
    IN char gain_abb;
} bt_cmd_set_rx_gain_t;

typedef struct {
    IN bool_t enable;
} bt_cmd_set_3m_feature_enable_t;

typedef struct {
    IN char name[MAX_NAME_LEN];
} bt_cmd_set_local_name_t;

typedef struct {
    IN char pin_code[MAX_PIN_CODE_LEN];
} bt_cmd_set_pin_code_t;

typedef struct {
    IN uint32_t cod;
} bt_cmd_set_cod_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_set_ssp_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_remove_pair_t;

typedef struct {
} bt_cmd_clear_pair_list_t;

typedef struct {
    OUT BD_ADDR_T addr;
} bt_cmd_get_local_addr_t;

typedef struct {
    OUT char name[MAX_NAME_LEN];
} bt_cmd_get_local_name_t;

typedef struct {
    OUT char pin_code[MAX_PIN_CODE_LEN];
} bt_cmd_get_pin_code_t;

typedef struct {
    OUT uint32_t cod;
} bt_cmd_get_cod_t;

typedef struct {
    OUT bool_t enabled;
} bt_cmd_get_ssp_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
    OUT char name[MAX_NAME_LEN];
} bt_cmd_get_remote_name_t;

typedef struct {
    OUT BD_ADDR_T addrs[MAX_CONNECTION];
    OUT uint8_t count;
} bt_cmd_get_connected_devices_t;

typedef struct {
    OUT BD_ADDR_T addrs[MAX_PAIR_LIST];
    OUT uint8_t count;
} bt_cmd_get_paired_devices_t;

typedef struct {
} bt_cmd_request_link_quality_report_t;
/************ events **********/

typedef struct {
    bt_connection_state_t state;
    BD_ADDR_T addr;
    uint8_t reason;   //local_disconnect, remote_disconnect, link_loss, ...
} bt_evt_connection_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
    bt_connection_mode_t mode;
} bt_evt_connection_mode_changed_t;

typedef struct {
    bool_t visible;
    bool_t connectable;
} bt_evt_visibility_changed_t;

typedef struct {
    char local_name[MAX_NAME_LEN];
} bt_evt_local_name_changed_t;

typedef struct {
    uint32_t cod;
} bt_evt_cod_changed_t;

typedef struct {
    char pin_code[MAX_PIN_CODE_LEN];
} bt_evt_pin_code_changed_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_pair_added_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_pair_removed_t;

typedef struct {
    BD_ADDR_T addr;
    uint8_t result;
} bt_evt_auth_result_t;

typedef struct {
    BD_ADDR_T addr;
    bt_phone_type_t type;
} bt_evt_phone_type_report_t;

/******************************* sdp api **********************/
typedef struct {
    /** service attribute data */
    const uint8_t *data;
    /** service attribute data size */
    uint8_t size;
} bt_service_attribute_t;

typedef struct {
    IN uint8_t uuid[16];
    OUT uint8_t profile_index;
} bt_cmd_sdp_add_uuid_t;

typedef struct {
    IN uint8_t uuid[16];
    OUT uint8_t profile_index;
} bt_cmd_sdp_get_uuid_index_t;

typedef struct {
    IN uint8_t profile_index_num;
    IN uint8_t *profile_index_list;
    IN uint8_t attribute_num;
    IN const bt_service_attribute_t *attribute_list;
} bt_cmd_sdp_register_record_t;

/************ events **********/

typedef struct {
    uint8_t channel;
    uint8_t uuid128[16];
} bt_rfcomm_channel_t;

typedef struct {
    IN BD_ADDR_T addr;
    uint8_t count;
    bt_rfcomm_channel_t rfcomm_channels[];
} bt_evt_sdp_rfcomm_found_t;

/******************************* hfp api **********************/

typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_hfp_state_t state;
} bt_cmd_hfp_get_state_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_hfp_set_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN char number[MAX_DIAL_NUMBER_LEN];
} bt_cmd_hfp_dial_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN char code;
} bt_cmd_hfp_send_dtmf_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_redial_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_answer_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_reject_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_hangup_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_activate_voice_recognition_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_deactivate_voice_recognition_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_connect_sco_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_disconnect_sco_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_twc_reject_waiting_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_twc_release_active_accept_waiting_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_twc_hold_active_accept_waiting_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_hfp_twc_conference_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint8_t volume;
} bt_cmd_hfp_report_volume_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint8_t level;
} bt_cmd_hfp_report_battery_level_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN char command[MAX_HFP_CMD_LEN];
} bt_cmd_hfp_send_command_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    bt_hfp_state_t state;
} bt_evt_hfp_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_hfp_ring_t;

typedef struct {
    BD_ADDR_T addr;
    bt_hfp_codec_t codec;
    bool_t connected;
} bt_evt_hfp_sco_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
    uint8_t signal;
} bt_evt_hfp_signal_changed_t;

typedef struct {
    BD_ADDR_T addr;
    uint8_t level;
} bt_evt_hfp_battery_level_changed_t;

typedef struct {
    BD_ADDR_T addr;
    uint8_t volume;
} bt_evt_hfp_volume_changed_t;

typedef struct {
    BD_ADDR_T addr;
    bool_t activated;
} bt_evt_hfp_voice_recognition_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
    char at[MAX_HFP_CMD_LEN];
} bt_evt_hfp_extended_at_t;

/******************************* a2dp api **********************/
typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_a2dp_state_t state;
} bt_cmd_a2dp_get_state_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_a2dp_set_enabled_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    bt_a2dp_state_t state;
    bt_a2dp_codec_t codec;
    uint32_t sample_rate;
} bt_evt_a2dp_state_changed_t;

/******************************* avrcp api **********************/
typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_avrcp_state_t state;
} bt_cmd_avrcp_get_state_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_avrcp_set_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_avrcp_play_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_avrcp_pause_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_avrcp_stop_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_avrcp_forward_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_avrcp_backward_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint8_t volume;
} bt_cmd_avrcp_report_volume_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    bt_avrcp_state_t state;
} bt_evt_avrcp_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
    uint8_t volume;
} bt_evt_avrcp_volume_changed_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_avrcp_volume_up_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_avrcp_volume_down_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_avrcp_absolute_volume_enabled_t;
/******************************* tws api **********************/
typedef enum {
    TWS_ROLE_CHANGED_REASON_USER,
    TWS_ROLE_CHANGED_REASON_CONNECT_FAIL,
    TWS_ROLE_CHANGED_REASON_RSSI,
    TWS_ROLE_CHANGED_REASON_LINK_LOSS,
    TWS_ROLE_CHANGED_REASON_PEER_POWER_OFF,
    TWS_ROLE_CHANGED_REASON_PEER_NOT_FOUND,
} tws_role_changed_reason_t;

typedef struct {
    OUT bt_tws_state_t state;
    OUT bt_tws_role_t role;
    OUT bt_tws_channel_t channel;
} bt_cmd_tws_get_state_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_tws_set_enabled_t;

typedef struct {
    IN bt_tws_role_t role;
} bt_cmd_tws_set_default_role_t;

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint8_t magic;
    uint32_t timeout_ms;
} bt_cmd_tws_start_pair_t;

typedef struct {
} bt_cmd_tws_role_switch_t;

typedef struct {
    OUT BD_ADDR_T addr;
} bt_cmd_tws_get_peer_addr_t;

typedef struct {
    IN bool_t exit_sniff;
    IN uint8_t *data;
    IN uint16_t len;
} bt_cmd_tws_send_data_t;

typedef struct {
    IN uint8_t *data;
    IN uint8_t len;
} bt_cmd_tws_set_tds_data_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_tws_set_auto_role_switch_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_tws_set_peer_addr_t;

typedef struct {
} bt_cmd_tws_enter_single_mode_t;

/************ events **********/

typedef struct {
    bt_tws_state_t state;
    BD_ADDR_T peer_addr;
    uint8_t reason;   //remote_poweroff, local_disconnect, link_loss, ...
} bt_evt_tws_state_changed_t;

typedef struct {
    bt_tws_role_t role;
    BD_ADDR_T local_addr;
    BD_ADDR_T peer_addr;
    tws_role_changed_reason_t reason;
} bt_evt_tws_role_changed_t;

typedef struct {
    uint8_t *data;
    uint16_t len;
} bt_evt_tws_recv_data_t;

typedef struct {
    BD_ADDR_T remote_addr;
    uint8_t *data;
    uint8_t len;
} bt_evt_tws_tds_data_t;

typedef struct {
    BD_ADDR_T peer_addr;
    bool_t succeed;
} bt_evt_tws_pair_result_t;

typedef struct {
    bool_t single_mode;
} bt_evt_tws_mode_changed_t;
/******************************* ble api **********************/

typedef struct {
    IN uint16_t interval_min;
    IN uint16_t interval_max;
    IN uint8_t adv_type;
    IN uint8_t own_addr_type;
    IN uint8_t peer_addr_type;
    IN BD_ADDR_T peer_addr;
    IN uint8_t channel_map;
    IN uint8_t filter_policy;
} bt_cmd_le_set_adv_param_t;

typedef struct {
    IN uint8_t *data;
    IN uint8_t len;
} bt_cmd_le_set_adv_data_t;

typedef struct {
    IN uint8_t *data;
    IN uint8_t len;
} bt_cmd_le_set_scan_response_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_le_set_adv_enabled_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_le_set_random_addr_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t conn_interval_min;
    IN uint16_t conn_interval_max;
    IN uint16_t conn_latency;
    IN uint16_t supervision_timeout;
    IN uint16_t minimum_ce_length;
    IN uint16_t maximum_ce_length;
} bt_cmd_le_conn_update_param_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
} bt_evt_le_conn_param_changed_t;

/******************************* l2cap api **********************/

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t local_psm;
    OUT bt_l2cap_state_t state;
} bt_cmd_l2cap_get_state_t;

typedef struct {
    IN uint16_t local_psm;
} bt_cmd_l2cap_register_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t local_psm;
    IN uint16_t remote_psm;
} bt_cmd_l2cap_connect_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t local_psm;
} bt_cmd_l2cap_disconnect_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t local_psm;
    IN const uint8_t *data;
    IN uint16_t length;
} bt_cmd_l2cap_send_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    uint16_t local_psm;
    bt_l2cap_state_t state;
} bt_evt_l2cap_state_changed_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t local_psm;
    uint8_t *data;
    uint16_t len;
} bt_evt_l2cap_data_t;

/******************************* spp api **********************/

typedef struct {
    IN BD_ADDR_T addr;
    IN bool_t is_client;   // true if local is spp client
    IN uint8_t channel;
    OUT bt_spp_state_t state;
} bt_cmd_spp_get_state_t;

typedef struct {
    IN uint16_t uuid16;
} bt_cmd_spp_register_t;

typedef struct {
    IN uint8_t uuid128[16];
} bt_cmd_spp_register_uuid128_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint8_t channel;
} bt_cmd_spp_connect_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN bool_t is_client;   // true if local is spp client
    IN uint8_t channel;
} bt_cmd_spp_disconnect_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN bool_t is_client;   // true if local is spp client
    IN uint8_t channel;
    IN const uint8_t *data;
    IN uint16_t length;
} bt_cmd_spp_send_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    IN bool_t is_client;   // true if local is spp client
    uint8_t channel;
    bt_spp_state_t state;
} bt_evt_spp_state_changed_t;

typedef struct {
    uint16_t uuid16;
    uint8_t channel;
} bt_evt_spp_registered_t;

typedef struct {
    uint8_t uuid128[16];
    uint8_t channel;
} bt_evt_spp_uuid128_registered_t;

typedef struct {
    BD_ADDR_T addr;
    IN bool_t is_client;   // true if local is spp client
    uint8_t channel;
    uint8_t *data;
    uint16_t len;
} bt_evt_spp_data_t;

/******************************* gatt client api **********************/

#define GATT_PROP_BROADCAST         0x01
#define GATT_PROP_READ              0x02
#define GATT_PROP_WRITE_WITHOUT_RSP 0x04
#define GATT_PROP_WRITE             0x08
#define GATT_PROP_NOTIFY            0x10
#define GATT_PROP_INDICATE          0x20

typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_gatt_state_t state;
} bt_cmd_gatt_client_get_state_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint8_t addr_type;
    IN uint8_t link_type;
} bt_cmd_gatt_client_connect_t;

typedef struct {
    IN BD_ADDR_T addr;
} bt_cmd_gatt_client_disconnect_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t chr_uuid16;
} bt_cmd_gatt_client_discover_t;

typedef struct {
    IN BD_ADDR_T addr;
    uint8_t chr_uuid128[16];
} bt_cmd_gatt_client_discover_uuid128_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t handle;
    IN uint16_t offset;
    IN uint16_t length;
} bt_cmd_gatt_client_read_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t handle;
    IN uint8_t *data;
    IN uint16_t length;
} bt_cmd_gatt_client_write_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    uint8_t addr_type; /*public,random*/
    uint8_t link_type; /*bredr,ble*/
    bt_gatt_state_t state;
} bt_evt_gatt_client_state_changed;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint16_t uuid16;
} bt_evt_gatt_client_service_found_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint8_t uuid128[16];
} bt_evt_gatt_client_service_uuid128_found_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint16_t uuid16;
    uint8_t property;
} bt_evt_gatt_client_characteristic_found_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint8_t uuid128[16];
    uint8_t property;
} bt_evt_gatt_client_characteristic_uuid128_found_t;

typedef struct {
    BD_ADDR_T addr;
} bt_evt_gatt_client_discoery_done_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint8_t *data;
    uint16_t len;
} bt_evt_gatt_client_data_notify_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint8_t *data;
    uint16_t offset;
    uint16_t len;
} bt_evt_gatt_client_read_response_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t mtu;
} bt_evt_gatt_client_mtu_changed_t;

/******************************* gatt server api **********************/

typedef struct {
    IN BD_ADDR_T addr;
    OUT bt_gatt_state_t state;
} bt_cmd_gatt_server_get_state_t;

typedef struct {
    IN bool_t enabled;
} bt_cmd_gatt_server_set_enabed_t;

typedef struct {
    IN uint16_t uuid16;
} bt_cmd_gatt_server_register_service_t;

typedef struct {
    IN uint8_t uuid128[16];
} bt_cmd_gatt_server_register_service_uuid128_t;

typedef struct {
    IN uint16_t service_handle;
    IN uint16_t chr_uuid16;
    IN uint8_t property;
} bt_cmd_gatt_server_register_characteristic_t;

typedef struct {
    IN uint16_t service_handle;
    IN uint8_t chr_uuid128[16];
    IN uint8_t property;
} bt_cmd_gatt_server_register_characteristic_uuid128_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t handle;
    IN const uint8_t *data;
    IN uint16_t length;
} bt_cmd_gatt_server_notify_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t handle;
    IN const uint8_t *data;
    IN uint16_t length;
} bt_cmd_gatt_server_indicate_t;

typedef struct {
    IN BD_ADDR_T addr;
    IN uint16_t handle;
    IN const uint8_t *data;
    IN uint16_t offset;
    IN uint16_t length;
} bt_cmd_gatt_server_send_read_response_t;

/************ events **********/

typedef struct {
    BD_ADDR_T addr;
    uint8_t addr_type /*public,random*/;
    uint8_t link_type; /*bredr,ble*/
    bt_gatt_state_t state;
} bt_evt_gatt_server_state_changed_t;

typedef struct {
    uint16_t handle;
    uint16_t uuid16;
} bt_evt_gatt_server_service_registered_t;

typedef struct {
    uint16_t handle;
    uint8_t uuid128[16];
} bt_evt_gatt_server_service_uuid128_registered_t;

typedef struct {
    uint16_t handle;
    uint16_t ccd_handle;
    uint16_t uuid16;
} bt_evt_gatt_server_characteristic_registered_t;

typedef struct {
    uint16_t handle;
    uint16_t ccd_handle;
    uint8_t uuid128[16];
} bt_evt_gatt_server_characteristic_uuid128_registered_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint8_t *data;
    uint16_t len;
} bt_evt_gatt_server_write_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t handle;
    uint16_t offset;
    uint16_t len;
} bt_evt_gatt_server_read_t;

typedef struct {
    BD_ADDR_T addr;
    uint16_t mtu;
} bt_evt_gatt_server_mtu_changed_t;

/******************************* hci api **********************/
typedef struct {
    IN bool_t enabled;
} bt_cmd_set_hci_evt_report_enabled_t;

typedef struct {
    IN uint16_t cmd;
    IN uint8_t param_len;
    IN uint8_t *param;
} bt_cmd_send_hci_command_t;

typedef struct {
    IN uint16_t data_len;
    IN uint8_t *data;
} bt_cmd_send_hci_data_t;

typedef struct {
    /** id of event which trigger the led action. */
    uint32_t led_evt_id;
    /* time in ms to delay before perform the led action. */
    uint32_t delay_ms;
} bt_sync_led_action_cmd_t;

typedef struct {
    /** id of tone to be played. */
    uint32_t tone_id;
    /* 1 to use extended delay allowed for this tone, 0 use normal delay. */
    uint8_t use_ext_delay;
} bt_sync_play_tone_by_btclk_cmd_t;

typedef struct {
    /** id of tone to be played. */
    uint32_t tone_id;
    /** time dur for each frame. */
    uint16_t ms_per_sn;
    /** sn of frame just played */
    uint32_t current_sn;
    /** rtc time when get current sn */
    uint32_t rtc_time_ms;
} bt_sync_play_tone_by_sn_cmd_t;

typedef struct {
    /* corresponding event id of the tone to be cancelled.*/
    uint32_t event_id;
} bt_sync_cancel_tone_t;

typedef struct {
    /** bt clock for ASRC to start work. */
    uint64_t bt_clock;
} bt_set_bt_trigger_ts_command_t;

/************ events **********/
typedef struct {
    uint8_t evt;
    uint8_t param_len;
    uint8_t *param;
} bt_evt_hci_evt_report_t;

typedef struct {
    uint16_t data_len;
    uint8_t *data;
} bt_evt_hci_data_received_t;

/******************************* milti-point api **********************/

/************ events **********/
typedef struct {
    BD_ADDR_T addr; /*bluetooth address of new primary device*/
} bt_evt_multipoint_primary_changed_t;
/**************************************************************/

/**
 * @}
 */

/**
 * @brief callback to handle user commands
 * @param cmd user commaand
 * @param param command parameters
 * @param param_len length of parameters
 * @return result
 */
typedef bt_result_t (*user_cmd_handle_cb)(bt_cmd_t cmd, void *param, uint32_t param_len);

/**
 * @brief register user command callback
 *
 * @param cb callback function
 */
void register_user_cmd_handler_cb(user_cmd_handle_cb cb);

/**
 * @brief callback to handle bt events
 * @param evt bt event
 * @param param event parameters
 * @param param_len length of parameters
 * @return result
 */
typedef bt_result_t (*bt_evt_cb_t)(bt_evt_t evt, void *param, uint32_t param_len);

/**
 * @brief register bt event callback
 *
 * @param cb callback function
 */
void bt_register_evt_cb(bt_evt_cb_t cb);

/**
 * @brief handle bt event
 * @param evt bt event
 * @param param event parameters
 * @param param_len length of parameters
 * @return result
 */
bt_result_t user_handle_bt_evt(bt_evt_t evt, void *param, uint32_t param_len);
/**
 * @brief handle user command
 * @param cmd user command
 * @param param command parameters
 * @param param_len length of parameters
 * @return result
 */
bt_result_t bt_handle_user_cmd(bt_cmd_t cmd, void *param, uint32_t param_len);

/**
 * @brief handle user shutdown command
 */
void bt_handle_user_shutdown(void);

/**
 * @brief dummy call
 */
void rpc_bt_mgnt_dummy(void);

/**
 * @}
 * addtogroup BT_MGNT
 */

/**
 * @}
 * addtogroup LIB
 */

#endif
