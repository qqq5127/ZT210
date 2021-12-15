#define ECONN_ZT210 1

#pragma GCC diagnostic ignored "-Wundef"

#if ECONN == ECONN_ZT210
#include "os_mem.h"
#include "os_utils.h"
#include "assert.h"
#include "app_main.h"
#include "app_econn.h"
#include "app_gatts.h"
#include "app_pm.h"
#include "app_adv.h"
#include "app_spp.h"
#include "app_charger.h"
#include "app_bat.h"
#include "app_inear.h"
#include "usr_cfg.h"
#include "app_wws.h"
#include "app_btn.h"
#include "app_evt.h"
#include "app_audio.h"
#include "ro_cfg.h"
#include "data_dump.h"
#include "app_econn_zt210.h"
#include "app_led.h"
#include "app_tone.h"
#include "battery_charger.h"
#include "dtopcore_data_mgr.h"
#include "key_sensor.h"

#include "app_user_flash.h"
#include "app_user_spp.h"
#include "aw8686x.h"
#include "iot_gpio.h"
#include "app_user_battery.h"


#define BATTERY_LOW_INTERVAL_MS 870000   //14.5*60*1000
#define SPP_UUID16              0x1101

#define UUID_CHARACTER_RX 0x7777
#define UUID_CHARACTER_TX 0x8888

#define STATE_MASK \
    (STATE_IDLE | STATE_CONNECTABLE | STATE_AG_PAIRING | STATE_CONNECTED | STATE_A2DP_STREAMING)
#define EQ_INDEX_CUSTOM 0xFEFE

#define ECONN_CFG_KV_ID APP_CUSTOMIZED_KEY_ID_START

typedef enum {
    ECONN_RESULT_FAILED = 0,
    ECONN_RESULT_SUCCESS = 1,
    ECONN_RESULT_CHECKSUM_ERROR = 2,
} econn_result_e;

typedef enum {
    CMD_GROUP_INFO = 0x01,
    CMD_GROUP_EQ = 0x02,
    CMD_GROUP_UPLOAD = 0x03,
    CMD_GROUP_ANC = 0x04,
} cmd_group_e;

typedef enum {
    INFO_GET_ALL = 0x01,
    INFO_GET_CONN_STATE = 0x02,
    INFO_GET_BAT_LVL = 0x03,
    INFO_GET_CHARGING = 0x04,
    INFO_GET_VERSION = 0x05,
} info_get_cmd_e;

typedef enum {
    INFO_SET_WEAR_DETECT = 0x81,
    INFO_SET_TOUCH_TONE = 0x83,
    INFO_SET_SIDE_TONE = 0x84,
} info_set_cmd_e;

typedef enum {
    EQ_GET_ALL = 0x01,
} eq_get_cmd_e;

typedef enum {
    EQ_SWITCH = 0x81,
    EQ_SWITCH_WITH_EFFECTED = 0x83,
} eq_set_cmd_e;

typedef enum {
    UI_GET_UNKNOWN = 0x00,
} ui_get_cmd_e;

typedef enum {
    UI_SET_BTN = 0x81,
    UI_RESET_BTN = 0x82,
    UI_SET_BTN_DISABLED = 0x83,
    UI_SET_ALL_BTN = 0x84
} ui_set_cmd_e;

typedef enum {
    UPLOAD_GET_ALL = 0x01,
} upload_get_cmd_e;

typedef enum {
    UPLOAD_CLEAR = 0x81,
} upload_set_cmd_e;

typedef enum {
    ANC_GET_MODE = 0x01,
    ANC_GET_TOGGLE_CFG = 0x02,
} anc_get_cmd_e;

typedef enum {
    ANC_SET_MODE = 0x81,
    ANC_SET_TOGGLE_CFG = 0x82,
} anc_set_cmd_e;

typedef enum {
    EQ_INDEX_BALANCED = 0,
    EQ_INDEX_ACOUSTIC = 1,
    EQ_INDEX_BASSBOOSTER = 2,
    EQ_INDEX_BASSREDUCER = 3,
    EQ_INDEX_CLASSICAL = 4,
    EQ_INDEX_DANCE = 5,
    EQ_INDEX_DEEP = 6,
    EQ_INDEX_ELECTRONIC = 7,
    EQ_INDEX_FLAT = 8,
    EQ_INDEX_HIPHOP = 9,
    EQ_INDEX_JAZZ = 10,
    EQ_INDEX_LATIN = 11,
    EQ_INDEX_LOUNGE = 12,
    EQ_INDEX_PIANO = 13,
    EQ_INDEX_POP = 14,
    EQ_INDEX_RANDB = 15,
    EQ_INDEX_ROCK = 16,
    EQ_INDEX_SMALLSPEAKER = 17,
    EQ_INDEX_SPOKENWORD = 18,
    EQ_INDEX_TREBLEBOOSTER = 19,
    EQ_INDEX_TREBLEREDUCER = 20,
    EQ_INDEX_VOCALBOOSTER = 21,
} eq_index_e;

typedef enum {
    ECONN_ANC_MODE_ANC = 0,
    ECONN_ANC_MODE_TRANC = 1,
    ECONN_ANC_MODE_NORMAL = 2,
} econn_anc_mode_e;

typedef enum {
    ECONN_ANC_LEVEL_FLIGHT = 0,
    ECONN_ANC_LEVEL_OUTDOOR = 1,
    ECONN_ANC_LEVEL_INDOOR = 2,
    ECONN_ANC_LEVEL_COMPLETE = 3,
} econn_anc_level_e;

typedef enum {
    ECONN_TRANS_LEVEL_COMPLETE = 0,
    ECONN_TRANS_LEVEL_VOCAL = 1,
} econn_trans_level_e;

typedef struct {
    uint8_t len_1;
    uint8_t type_1;
    uint8_t flag;
    uint8_t len_2;
    uint8_t type_2;
    uint8_t bdaddr[6];
    uint8_t reserved[2];
    uint8_t len_3;
    uint8_t type_3;
    uint8_t uuid[4];
	uint8_t name_len;
    uint8_t name_type;
    uint8_t name[10];
} adv_info_t;

typedef struct {
    uint16_t eq_index;
    uint8_t left_cus_eq[8];
    uint8_t right_cus_eq[8];
    uint8_t left_effected_eq[8];
    uint8_t right_effected_eq[8];
    bool_t touch_tone_enabled;
} econn_cfg_t;

static bool_t gatts_connected = false;
static bool_t spps_connected = false;
static bool_t adv_enabled = false;
static bool_t link_loss = false;
static bool_t force_discoverable = false;
static bool_t box_battery_low = false;
static uint32_t last_battery_low = 0;
static uint16_t ntc_current_limit = 150;

static BD_ADDR_T remote_addr = {0};
static BD_ADDR_T last_connected_addr = {0};

static gatts_character_t *character_tx = NULL;
static gatts_character_t *character_rx = NULL;

static econn_cfg_t econn_cfg;

//reverse sequence
static const uint8_t service_uuid128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xDA, 0xF5, 0x25, 0x01,
};

static adv_info_t adv_info = {.len_1 = 0x02,
                              .type_1 = 0x01,
                              .flag = 0x06,
                              .len_2 = 0x09,
                              .type_2 = 0xff,
                              .bdaddr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                              .reserved = {0x00, 0x00},
                              .len_3 = 0x05,
                              .type_3 = 0x05,
                              .uuid = {0x01, 0x25, 0xF5, 0xDA},
                              .name_len = 0x0b,                        
						      .name_type = 0x09,                       
						      .name = {'E', 'a', 'r', 'b', 'u', 'd', 's', ' ', 'X', '3'} 
};

static const uint8_t CMD_HEADER[] = {0x08, 0xEE, 0x00, 0x00, 0x00};
static const uint8_t RSP_HEADER[] = {0x09, 0xFF, 0x00, 0x00};
static bool_t power_off_by_box = false;
static bool_t app_listen_mode_changing = false;
static bool_t disconnect_for_spp = false;

static bool_t econn_send_data(uint8_t *data, uint16_t len);

static void econn_enable_adv(void)
{
    const BD_ADDR_T *addr;

    if (adv_enabled) {
        DBGLOG_ECONN_DBG("econn_enable_adv already enabled\n");
        return;
    }
    adv_enabled = true;
    DBGLOG_ECONN_DBG("econn_enable_adv\n");

    addr = app_bt_get_local_address();

    for (uint8_t i = 0; i < 6; i++) {
        adv_info.bdaddr[i] = addr->addr[5 - i];
    }
    app_adv_set_adv_data((uint8_t *)(&adv_info), sizeof(adv_info));
    app_adv_set_enabled(true);
}

static void econn_disable_adv(void)
{
    DBGLOG_ECONN_DBG("econn_disable_adv\n");

    app_adv_set_enabled(false);
    adv_enabled = false;
}

static uint8_t get_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }

    return sum;
}

static econn_anc_mode_e get_current_anc_mode(void)
{
    if (!app_wws_is_connected()) {
        return ECONN_ANC_MODE_NORMAL;
    }

    switch (usr_cfg_get_listen_mode()) {
        case LISTEN_MODE_NORMAL:
            return ECONN_ANC_MODE_NORMAL;
        case LISTEN_MODE_ANC:
            return ECONN_ANC_MODE_ANC;
        case LISTEN_MODE_TRANSPARENCY:
            return ECONN_ANC_MODE_TRANC;
        default:
            return ECONN_ANC_MODE_NORMAL;
    }
}

static econn_anc_level_e get_current_anc_level(void)
{

    switch (usr_cfg_get_anc_level()) {
        case ANC_LEVEL_FLIGHT:
            return ECONN_ANC_LEVEL_FLIGHT;
        case ANC_LEVEL_IN_DOOR:
            return ECONN_ANC_LEVEL_INDOOR;
        case ANC_LEVEL_OUT_DOOR:
            return ECONN_ANC_LEVEL_OUTDOOR;
        default:
            return ECONN_ANC_LEVEL_FLIGHT;
    }
}

static econn_trans_level_e get_current_trans_level(void)
{
    switch (usr_cfg_get_transparency_level()) {
        case TRANSPARENCY_LEVEL_FULL:
            return ECONN_TRANS_LEVEL_COMPLETE;
        case TRANSPARENCY_LEVEL_VOICE:
            return ECONN_TRANS_LEVEL_VOCAL;
        default:
            return ECONN_TRANS_LEVEL_COMPLETE;
    }
}

static void update_player_eq(void)
{
    int16_t tmp;
    int16_t gain[8];
    const uint8_t *data;

    if (app_wws_is_left()) {
        data = econn_cfg.left_effected_eq;
    } else {
        data = econn_cfg.right_effected_eq;
    }

    for (int i = 0; i < 8; i++) {
        tmp = data[i];   // effected eq: gain + 120
        tmp -= 120;
        tmp *= 100;
        gain[i] = tmp;   // eq_gain_list = gain * 100
    }

    DBGLOG_ECONN_DBG("[econn] update_player_eq index:0x%X %d,%d,%d,%d,%d,%d,%d,%d\n",
                     econn_cfg.eq_index, gain[0], gain[1], gain[2], gain[3], gain[4], gain[5],
                     gain[6], gain[7]);

    player_set_eq_coeff_gain(8, gain);
}

static void econn_cfg_default(void)
{
    memset(&econn_cfg, 0, sizeof(econn_cfg_t));

    econn_cfg.eq_index = 0;
    memset(econn_cfg.left_cus_eq, 120, 8);
    memset(econn_cfg.right_cus_eq, 120, 8);
    memset(econn_cfg.left_effected_eq, 120, 8);
    memset(econn_cfg.right_effected_eq, 120, 8);
}

static void econn_cfg_save(void)
{
    if (storage_write(APP_BASE_ID, ECONN_CFG_KV_ID, &econn_cfg, sizeof(econn_cfg_t)) != RET_OK) {
        DBGLOG_ECONN_ERR("econn_cfg_save failed\n");
    }
}

static void econn_cfg_load(void)
{
    uint32_t len = sizeof(econn_cfg_t);

    if ((storage_read(APP_BASE_ID, ECONN_CFG_KV_ID, &econn_cfg, &len) != RET_OK)
        || (len != sizeof(econn_cfg_t))) {
        DBGLOG_ECONN_DBG("econn_cfg_load failed len = %d\n", len);
        econn_cfg_default();
        econn_cfg_save();
    }
}

static uint8_t bat_lvl_convert(uint8_t level)
{
    //0: 1%~5%; 1: 6%~20%; 2: 21%~40%; 3:41%~60%; 4:61%~80%; 5:81%~100%
    if (level <= 5) {
        return 0;
    } else if (level <= 20) {
        return 1;
    } else if (level <= 40) {
        return 2;
    } else if (level <= 60) {
        return 3;
    } else if (level <= 80) {
        return 4;
    } else {
        return 5;
    }
}

static void send_rsp(econn_result_e result, cmd_group_e group, uint8_t cmd, const uint8_t *param,
                     uint8_t len)
{
    uint8_t *rsp = os_mem_malloc(IOT_APP_MID, len + 10);

    memcpy(rsp, RSP_HEADER, 4);
    rsp[4] = result;
    rsp[5] = group;
    rsp[6] = cmd;
    rsp[7] = len + 10;
    rsp[8] = 0;
    memcpy(rsp + 9, param, len);
    rsp[len + 9] = get_checksum(rsp, len + 9);

    econn_send_data(rsp, len + 10);

    dump_bytes(rsp, len + 10);

    os_mem_free(rsp);
}

static void send_all_info(void)
{
    uint8_t param[44];

    memset(param, 0xFF, sizeof(param));

    //conn state (2 byte)
    param[0] = app_wws_is_left() ? 0 : 1;
    param[1] = app_wws_is_connected() ? 1 : 0;

    //battery level (2 byte)
    if (app_wws_is_left()) {
        param[2 + 0] = bat_lvl_convert(app_bat_get_level());
        param[2 + 1] = bat_lvl_convert(app_wws_peer_get_battery_level());
    } else {
        param[2 + 1] = bat_lvl_convert(app_bat_get_level());
        param[2 + 0] = bat_lvl_convert(app_wws_peer_get_battery_level());
    }

    //charging (2 byte)
    if (app_wws_is_left()) {
        param[4 + 0] = app_charger_is_charging() ? 1 : 0;
        if (app_wws_is_connected()) {
            param[4 + 1] = app_wws_peer_is_charging() ? 1 : 0;
        }
    } else {
        param[4 + 1] = app_charger_is_charging() ? 1 : 0;
        if (app_wws_is_connected()) {
            param[4 + 0] = app_wws_peer_is_charging() ? 1 : 0;
        }
    }

    //eq index (2 byte)
    param[6] = econn_cfg.eq_index & 0xFF;
    param[7] = econn_cfg.eq_index >> 8;

    //left eq (8 byte)
    memcpy(param + 8, econn_cfg.left_cus_eq, 8);

    //right eq (8 byte)
    memcpy(param + 16, econn_cfg.right_cus_eq, 8);

    //ANC (4 byte) anc_mode, anc_level, trans_level, custom
    param[24] = get_current_anc_mode();
    param[25] = get_current_anc_level();
    param[26] = get_current_trans_level();
    param[27] = 0;

    //SideTone(1 byte)
    param[28] = 0;

    //TouchTone(1 byte)
    param[29] = econn_cfg.touch_tone_enabled ? 1 : 0;
	
    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_INFO, INFO_GET_ALL, param, sizeof(param));
}

static void send_conn_state(void)
{
    uint8_t param[2];

    param[0] = app_wws_is_left() ? 0 : 1;
    param[1] = app_wws_is_connected() ? 1 : 0;

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_INFO, INFO_GET_CONN_STATE, param, sizeof(param));
}

static void send_battery_level(void)
{
    uint8_t param[2];

    memset(param, 0, sizeof(param));

    if (app_wws_is_left()) {
        param[0] = bat_lvl_convert(app_bat_get_level());
        param[1] = bat_lvl_convert(app_wws_peer_get_battery_level());
    } else {
        param[1] = bat_lvl_convert(app_bat_get_level());
        param[0] = bat_lvl_convert(app_wws_peer_get_battery_level());
    }

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_INFO, INFO_GET_BAT_LVL, param, sizeof(param));
}

static void send_charging_state(void)
{
    uint8_t param[2];

    memset(param, 0, sizeof(param));

    if (app_wws_is_left()) {
        param[0] = app_charger_is_charging() ? 1 : 0;
        if (app_wws_is_connected()) {
            param[1] = app_wws_peer_is_charging() ? 1 : 0;
        }
    } else {
        param[1] = app_charger_is_charging() ? 1 : 0;
        if (app_wws_is_connected()) {
            param[0] = app_wws_peer_is_charging() ? 1 : 0;
        }
    }

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_INFO, INFO_GET_CHARGING, param, sizeof(param));
}

static void send_version(void)
{
    uint8_t param[26];
    char str_addr[13];
    char version[6];

    const BD_ADDR_T *addr = app_bt_get_local_address();

    memset(param, 0, sizeof(param));

    memcpy(param + 10, "3935", 4);
    bdaddr2str(addr, str_addr);
    memcpy(param + 14, str_addr, 12);

    snprintf(version, sizeof(version), "%02d.%02d", CUSTOM_VERSION / 100, CUSTOM_VERSION % 100);

    memcpy(param, version, 5);
    memcpy(param + 5, version, 5);

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_INFO, INFO_GET_VERSION, param, sizeof(param));
}

static void handle_info_get_cmd(info_get_cmd_e cmd)
{
    switch (cmd) {
        case INFO_GET_ALL:
            DBGLOG_ECONN_DBG("[econn] INFO_GET_ALL");
            send_all_info();
            break;
        case INFO_GET_CONN_STATE:
            DBGLOG_ECONN_DBG("[econn] INFO_GET_CONN_STATE");
            send_conn_state();
            break;
        case INFO_GET_BAT_LVL:
            DBGLOG_ECONN_DBG("[econn] INFO_GET_BAT_LVL");
            send_battery_level();
            break;
        case INFO_GET_CHARGING:
            DBGLOG_ECONN_DBG("[econn] INFO_GET_CHARGING");
            send_charging_state();
            break;
        case INFO_GET_VERSION:
            DBGLOG_ECONN_DBG("[econn] INFO_GET_VERSION");
            send_version();
            break;
        default:
            DBGLOG_ECONN_ERR("[econn] handle_info_get_cmd invalid cmd:%d\n", cmd);
            send_rsp(ECONN_RESULT_FAILED, CMD_GROUP_INFO, cmd, NULL, 0);
            break;
    }
}

static bool_t handle_set_wear_detect_cmd(const uint8_t *param, uint8_t len)
{
    UNUSED(param);
    UNUSED(len);

    DBGLOG_ECONN_ERR("[econn] handle_set_wear_detect_cmd unsupported");

    return false;
}

static bool_t hanlde_set_touch_tone_cmd(const uint8_t *param, uint8_t len)
{
    uint8_t enable;

    if (len != 1) {
        DBGLOG_ECONN_ERR("[econn] hanlde_set_touch_tone_cmd invalid len:%d\n", len);
        return false;
    }

    enable = param[0];

    DBGLOG_ECONN_DBG("[econn] hanlde_set_touch_tone_cmd enable:%d\n", enable);

    if (enable) {
        econn_cfg.touch_tone_enabled = true;
    } else {
        econn_cfg.touch_tone_enabled = false;
    }

    app_wws_send_remote_msg(MSG_TYPE_ECONN, ECONN_REMOTE_MSG_ID_UPDATE_TOUCH_TONE,
                            sizeof(econn_cfg_t), (uint8_t *)&econn_cfg);
    econn_cfg_save();
    return true;
}

static bool_t hanlde_set_side_tone_cmd(const uint8_t *param, uint8_t len)
{
    UNUSED(param);
    UNUSED(len);

    DBGLOG_ECONN_ERR("[econn] hanlde_set_side_tone_cmd unsupported");

    return false;
}

static bool_t handle_info_set_cmd(info_set_cmd_e cmd, const uint8_t *param, uint8_t len)
{
    bool_t ret = false;

    switch (cmd) {
        case INFO_SET_WEAR_DETECT:
            ret = handle_set_wear_detect_cmd(param, len);
            break;
        case INFO_SET_TOUCH_TONE:
            ret = hanlde_set_touch_tone_cmd(param, len);
            break;
        case INFO_SET_SIDE_TONE:
            ret = hanlde_set_side_tone_cmd(param, len);
            break;
        default:
            DBGLOG_ECONN_ERR("[econn] handle_info_set_cmd invalid cmd:%d\n", cmd);
            break;
    }

    return ret;
}

static void send_eq_rsp(void)
{
    uint16_t eq_index;
    uint8_t param[2];

    eq_index = econn_cfg.eq_index;

    param[0] = eq_index & 0xFF;
    param[1] = eq_index >> 8;

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_EQ, EQ_GET_ALL, param, sizeof(param));
}

static void handle_eq_get_cmd(eq_get_cmd_e cmd)
{

    if (cmd != EQ_GET_ALL) {
        DBGLOG_ECONN_ERR("[econn] handle_eq_get_cmd unknown cmd:%d\n", cmd);
        send_rsp(ECONN_RESULT_FAILED, CMD_GROUP_EQ, cmd, NULL, 0);
        return;
    }
    send_eq_rsp();
}

static bool_t handle_eq_switch_cmd(const uint8_t *param, uint8_t len)
{
    uint16_t eq_index;

    if (len != 18) {
        DBGLOG_ECONN_ERR("[econn] handle_eq_switch_cmd invalid len:%d\n", len);
        return false;
    }

    eq_index = param[1];
    eq_index <<= 8;
    eq_index += param[0];

    DBGLOG_ECONN_DBG("[econn] handle_eq_switch_cmd eq_index:0x%X", eq_index);

    econn_cfg.eq_index = eq_index;
    memcpy(econn_cfg.left_cus_eq, param + 2, 8);
    memcpy(econn_cfg.right_cus_eq, param + 10, 8);
    econn_cfg_save();

    app_wws_send_remote_msg(MSG_TYPE_ECONN, ECONN_REMOTE_MSG_ID_UPDATE_EQ, sizeof(econn_cfg_t),
                            (uint8_t *)&econn_cfg);

    update_player_eq();

    return true;
}

static bool_t handle_eq_switch_with_effected_cmd(const uint8_t *param, uint8_t len)
{
    uint16_t eq_index;

    if (len != 34) {
        DBGLOG_ECONN_ERR("[econn] handle_eq_switch_with_effected_cmd invalid len:%d\n", len);
        return false;
    }

    eq_index = param[1];
    eq_index <<= 8;
    eq_index += param[0];

    DBGLOG_ECONN_DBG("[econn] handle_eq_switch_with_effected_cmd eq_index:0x%X", eq_index);

    econn_cfg.eq_index = eq_index;
    memcpy(econn_cfg.left_cus_eq, param + 2, 8);
    memcpy(econn_cfg.right_cus_eq, param + 10, 8);
    memcpy(econn_cfg.left_effected_eq, param + 18, 8);
    memcpy(econn_cfg.right_effected_eq, param + 18, 8);
    econn_cfg_save();
    app_wws_send_remote_msg(MSG_TYPE_ECONN, ECONN_REMOTE_MSG_ID_UPDATE_EQ, sizeof(econn_cfg_t),
                            (uint8_t *)&econn_cfg);

    update_player_eq();

    return true;
}

static bool_t handle_eq_set_cmd(eq_set_cmd_e cmd, const uint8_t *param, uint8_t len)
{
    if (cmd == EQ_SWITCH) {
        return handle_eq_switch_cmd(param, len);
    } else if (cmd == EQ_SWITCH_WITH_EFFECTED) {
        return handle_eq_switch_with_effected_cmd(param, len);
    } else {
        DBGLOG_ECONN_ERR("[econn] handle_eq_set_cmd unknown cmd:%d\n", cmd);
        return false;
    }
}

static void handle_upload_get_cmd(upload_get_cmd_e cmd)
{
    UNUSED(cmd);

    DBGLOG_ECONN_ERR("[econn] handle_upload_get_cmd unsupported\n");

    send_rsp(ECONN_RESULT_FAILED, CMD_GROUP_UPLOAD, cmd, NULL, 0);
}

static bool_t handle_upload_set_cmd(upload_set_cmd_e cmd, const uint8_t *param, uint8_t len)
{
    UNUSED(cmd);
    UNUSED(param);
    UNUSED(len);

    //DBGLOG_ECONN_ERR("[econn] handle_upload_set_cmd unsupported\n");

    return false;
}

static void send_listen_mode(void)
{
    //anc_mode, anc_level, trans_level, custom
    uint8_t param[4];

    DBGLOG_ECONN_DBG("[econn] handle_anc_get_mode_cmd\n");

    param[0] = get_current_anc_mode();
    param[1] = get_current_anc_level();
    param[2] = get_current_trans_level();
    param[3] = 0;

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_ANC, ANC_GET_MODE, param, sizeof(param));
}

static void send_listen_mode_toggle_cfg(void)
{
    //anc selected, trans selected, normal selected
    uint8_t param[3] = {0};
    listen_mode_toggle_cfg_t cfg = app_audio_listen_mode_get_toggle_cfg();

    DBGLOG_ECONN_DBG("[econn] handle_anc_get_toggle_cfg_cmd cfg:0x%X\n", cfg);

    if (cfg & LISTEN_MODE_TOGGLE_ANC) {
        param[0] = 1;
    }

    if (cfg & LISTEN_MODE_TOGGLE_TRANSPARENCY) {
        param[1] = 1;
    }

    if (cfg & LISTEN_MODE_TOGGLE_NORMAL) {
        param[2] = 1;
    }

    send_rsp(ECONN_RESULT_SUCCESS, CMD_GROUP_ANC, ANC_GET_TOGGLE_CFG, param, sizeof(param));
}

static void handle_anc_get_cmd(anc_get_cmd_e cmd)
{
    if (cmd == ANC_GET_MODE) {
        send_listen_mode();
    } else if (cmd == ANC_GET_TOGGLE_CFG) {
        send_listen_mode_toggle_cfg();
    } else {
        DBGLOG_ECONN_ERR("[econn] handle_anc_get_cmd invalid cmd:%d\n", cmd);
        send_rsp(ECONN_RESULT_FAILED, CMD_GROUP_ANC, cmd, NULL, 0);
    }
}

static bool_t handle_anc_set_mode_cmd(const uint8_t *param, uint8_t len)
{
    //anc_mode, anc_level, trans_level, custom
    econn_anc_mode_e mode;
    econn_anc_level_e anc_level;
    econn_trans_level_e trans_level;
    uint8_t custom;
    listen_mode_t listen_mode = LISTEN_MODE_NORMAL;

    if (len != 4) {
        DBGLOG_ECONN_ERR("[econn] handle_anc_set_mode_cmd invalid len:%d\n", len);
        return false;
    }

    mode = param[0];
    anc_level = param[1];
    trans_level = param[2];
    custom = param[3];

    DBGLOG_ECONN_DBG("[econn] handle_anc_set_mode_cmd mode:%d anc_lvl:%d trans_lvl:%d cus:%d\n",
                     mode, anc_level, trans_level, custom);

    switch (anc_level) {
        case ECONN_ANC_LEVEL_FLIGHT:
            app_audio_anc_level_set(ANC_LEVEL_FLIGHT);
            break;
        case ECONN_ANC_LEVEL_INDOOR:
            app_audio_anc_level_set(ANC_LEVEL_IN_DOOR);
            break;
        case ECONN_ANC_LEVEL_OUTDOOR:
            app_audio_anc_level_set(ANC_LEVEL_OUT_DOOR);
            break;
        default:
            app_audio_anc_level_set(ANC_LEVEL_FLIGHT);
            break;
    }

    switch (trans_level) {
        case ECONN_TRANS_LEVEL_COMPLETE:
            app_audio_transparency_level_set(TRANSPARENCY_LEVEL_FULL);
            break;
        case ECONN_TRANS_LEVEL_VOCAL:
            app_audio_transparency_level_set(TRANSPARENCY_LEVEL_VOICE);
            break;
        default:
            app_audio_transparency_level_set(TRANSPARENCY_LEVEL_FULL);
            break;
    }

    switch (mode) {
        case ECONN_ANC_MODE_NORMAL:
            listen_mode = LISTEN_MODE_NORMAL;
            break;
        case ECONN_ANC_MODE_TRANC:
            listen_mode = LISTEN_MODE_TRANSPARENCY;
            break;
        case ECONN_ANC_MODE_ANC:
            listen_mode = LISTEN_MODE_ANC;
            break;
        default:
            listen_mode = LISTEN_MODE_NORMAL;
            break;
    }

    if (app_audio_listen_mode_get() != listen_mode) {
        app_listen_mode_changing = true;
        app_audio_listen_mode_set_silently(listen_mode);
        app_listen_mode_changing = false;
    }

    return true;
}

static bool_t handle_anc_set_toggle_cfg_cmd(const uint8_t *param, uint8_t len)
{
    listen_mode_toggle_cfg_t cfg = 0;

    //anc selected, trans selected, normal selected
    if (len != 3) {
        DBGLOG_ECONN_ERR("[econn] handle_anc_set_toggle_cfg_cmd invalid len:%d\n", len);
        return false;
    }

    DBGLOG_ECONN_DBG("[econn] handle_anc_set_toggle_cfg_cmd anc:%d trans:%d normal:%d\n", param[0],
                     param[1], param[2]);

    if (param[0]) {
        cfg |= LISTEN_MODE_TOGGLE_ANC;
    }

    if (param[1]) {
        cfg |= LISTEN_MODE_TOGGLE_TRANSPARENCY;
    }

    if (param[2]) {
        cfg |= LISTEN_MODE_TOGGLE_NORMAL;
    }

    app_audio_listen_mode_set_toggle_cfg(cfg);

    return true;
}

static bool_t handle_anc_set_cmd(anc_set_cmd_e cmd, const uint8_t *param, uint8_t len)
{

    switch (cmd) {
        case ANC_SET_MODE:
            return handle_anc_set_mode_cmd(param, len);
        case ANC_SET_TOGGLE_CFG:
            return handle_anc_set_toggle_cfg_cmd(param, len);
        default:
            DBGLOG_ECONN_ERR("[econn] handle_anc_set_cmd invalid cmd:%d\n", cmd);
            return false;
    }
}

static void handle_get_cmd(cmd_group_e group, uint8_t cmd)
{
    switch (group) {
        case CMD_GROUP_INFO:
            handle_info_get_cmd(cmd);
            break;
        case CMD_GROUP_EQ:
            handle_eq_get_cmd(cmd);
            break;
        case CMD_GROUP_UPLOAD:
            handle_upload_get_cmd(cmd);
            break;
        case CMD_GROUP_ANC:
            handle_anc_get_cmd(cmd);
            break;
        default:
            DBGLOG_ECONN_ERR("[econn] handle_get_cmd unknown group:%d\n", group);
            send_rsp(ECONN_RESULT_FAILED, group, cmd, NULL, 0);
            break;
    }
}

static bool_t handle_set_cmd(cmd_group_e group, uint8_t cmd, const uint8_t *param, uint8_t len)
{
    bool_t ret = false;

    switch (group) {
        case CMD_GROUP_INFO:
            ret = handle_info_set_cmd(cmd, param, len);
            break;
        case CMD_GROUP_EQ:
            ret = handle_eq_set_cmd(cmd, param, len);
            break;
        case CMD_GROUP_UPLOAD:
            ret = handle_upload_set_cmd(cmd, param, len);
            break;
        case CMD_GROUP_ANC:
            ret = handle_anc_set_cmd(cmd, param, len);
            break;
        default:
            DBGLOG_ECONN_ERR("[econn] handle_set_cmd unknown group:%d\n", group);
            break;
    }

    return ret;
}

static void econn_handle_cmd(cmd_group_e group, uint8_t cmd, const uint8_t *param, uint8_t len)
{
    bool_t is_set_cmd = cmd & 0x80;

    if (is_set_cmd) {
        if (handle_set_cmd(group, cmd, param, len)) {
            send_rsp(ECONN_RESULT_SUCCESS, group, cmd, NULL, 0);
        } else {
            send_rsp(ECONN_RESULT_FAILED, group, cmd, NULL, 0);
        }
    } else {
        handle_get_cmd(group, cmd);
    }
}

static void econn_handle_data(const uint8_t *data, uint8_t len)
{
    uint8_t checksum;
    uint8_t param_len;
    uint8_t group;
    uint8_t cmd;

    dump_bytes(data, len);

    if (len < 10) {
        DBGLOG_ECONN_ERR("econn_handle_data invalid len:%d\n", len);
        return;
    }

    if (memcmp(data, CMD_HEADER, sizeof(CMD_HEADER))) {
        DBGLOG_ECONN_ERR("econn_handle_data invalid header\n");
        return;
    }

    checksum = get_checksum(data, len - 1);
    if (checksum != data[len - 1]) {
        DBGLOG_ECONN_ERR("econn_handle_data checksum:0x%X != data:0x%X\n", checksum, data[len - 1]);
        send_rsp(ECONN_RESULT_CHECKSUM_ERROR, data[5], data[6], NULL, 0);
        return;
    }

    if (data[8] != 0) {
        DBGLOG_ECONN_DBG("econn_handle_data invalid len_high:%d\n", data[8]);
        send_rsp(ECONN_RESULT_FAILED, data[5], data[6], NULL, 0);
        return;
    }

    if (data[7] != len) {
        DBGLOG_ECONN_ERR("econn_handle_data data_len:%d + 10 != len:%d\n", data[7], len);
        send_rsp(ECONN_RESULT_FAILED, data[5], data[6], NULL, 0);
        return;
    }

    param_len = len - 10;

    group = data[5];
    cmd = data[6];

    if (gatts_connected && (!spps_connected) && app_wws_is_slave() && (data[5] & 0x80)) {
        app_wws_send_remote_msg(MSG_TYPE_ECONN, ECONN_REMOTE_MSG_ID_DELIVER_CMD, len, data);
        send_rsp(ECONN_RESULT_SUCCESS, group, cmd, NULL, 0);
        DBGLOG_ECONN_DBG("[econn] deliver set cmd to master\n");
        return;
    }

    if (param_len) {
        econn_handle_cmd(group, cmd, data + 9, param_len);
    } else {
        econn_handle_cmd(group, cmd, NULL, 0);
    }
}

static void econn_msg_handler(uint16_t msg_id, void *param)
{
    switch (msg_id) {
        case ECONN_MSG_ID_FACTORY_RESET:
            DBGLOG_ECONN_DBG("ECONN_MSG_ID_FACTORY_RESET\n");
            app_bt_factory_reset(false);
            usr_cfg_reset();
            app_btn_cus_key_reset();
            app_inear_cfg_reset();
            econn_cfg_default();
            econn_cfg_save();
            break;
        case ECONN_REMOTE_MSG_ID_UPDATE_EQ:
            DBGLOG_ECONN_DBG("ECONN_REMOTE_MSG_ID_UPDATE_EQ\n");
            memcpy(&econn_cfg, param, sizeof(econn_cfg_t));
            econn_cfg_save();
            update_player_eq();
            break;
        case ECONN_REMOTE_MSG_ID_UPDATE_TOUCH_TONE:
            DBGLOG_ECONN_DBG("ECONN_REMOTE_MSG_ID_UPDATE_TOUCH_TONE\n");
            memcpy(&econn_cfg, param, sizeof(econn_cfg_t));
            econn_cfg_save();
            break;
        case ECONN_REMOTE_MSG_ID_DELIVER_CMD: {
            uint8_t len;
            uint8_t *data = param;
            len = data[7];
            econn_handle_data(data, len);
            break;
        }
        case ECONN_MSG_ID_FORCE_DISCOVERABLE:
            force_discoverable = true;
            DBGLOG_ECONN_DBG("ECONN_MSG_ID_FORCE_DISCOVERABLE\n");
            if (app_wws_is_master()) {
                app_bt_set_force_discoverable(true);
                app_led_set_custom_state(STATE_AG_PAIRING);
            }
            break;
        case ECONN_MSG_ID_CLAER_LAST_BATTERY_LOW:
            DBGLOG_ECONN_DBG("ECONN_MSG_ID_CLAER_LAST_BATTERY_LOW\n");
            last_battery_low = 0;
            break;
        case ECONN_MSG_ID_REPORT_TWS_STATE:
            send_all_info();
            break;
        case ECONN_MSG_ID_CHECK_SPP_STATE:
            if ((app_bt_get_a2dp_state() < A2DP_STATE_CONNECTED)
                && (app_bt_get_hfp_state() < HFP_STATE_CONNECTED) && spps_connected) {
                DBGLOG_ECONN_ERR("spp connected when no profile connected");
                disconnect_for_spp = true;
                app_bt_disconnect();
                return;
            }
            break;
        default:
            break;
    }
}

static void spp_data_callback(BD_ADDR_T *addr, uint16_t uuid, uint8_t *data, uint16_t len)
{
    UNUSED(addr);
    UNUSED(uuid);

    if (app_wws_is_slave()) {
        DBGLOG_ECONN_DBG("spp_data_callback ignored for slave\n");
    } else {
        econn_handle_data(data, len);
    }

    os_mem_free(data);
}

static void spp_connection_callback(BD_ADDR_T *addr, uint16_t uuid, bool_t connected)
{
    UNUSED(uuid);
    DBGLOG_ECONN_DBG("[econn] spp_connection_callback connected:%d\n", connected);

    if (connected) {
        spps_connected = true;
        remote_addr = *addr;
        econn_disable_adv();

        if ((app_bt_get_a2dp_state() < A2DP_STATE_CONNECTED)
            && (app_bt_get_hfp_state() < HFP_STATE_CONNECTED)) {
            app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_CHECK_SPP_STATE);
            app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_CHECK_SPP_STATE, NULL, 0, 6000);
        }
    } else {
        spps_connected = false;
        if (app_wws_is_master()) {
            econn_enable_adv();
        }
    }
}

static void write_callback(BD_ADDR_T *addr, gatts_character_t *character, uint8_t *data,
                           uint16_t len)
{
    UNUSED(character);
    if (bdaddr_is_equal(&remote_addr, addr)) {
        econn_handle_data(data, len);
    } else {
        DBGLOG_ECONN_DBG("[econn] write_callback ignore unknown device data");
    }

    os_mem_free(data);
}

static void notify_enable_callback(BD_ADDR_T *addr, gatts_character_t *character, bool_t enabled)
{
    UNUSED(character);

    DBGLOG_ECONN_DBG("[econn] notify_enable_callback enabled:%d\n", enabled);

    if (enabled) {
        if (gatts_connected) {
            remote_addr = *addr;
            send_conn_state();
            send_listen_mode();
        }
    } else {
        gatts_connected = false;
    }
}

static void gatts_connection_callback(BD_ADDR_T *addr, bool_t is_ble, bool_t connected)
{
    UNUSED(is_ble);
    UNUSED(addr);
    DBGLOG_ECONN_DBG("[econn] gatts_connection_callback connected:%d\n", connected);

    if (connected) {
        gatts_connected = true;
    } else {
        gatts_connected = false;
    }
}

static bool_t econn_send_data(uint8_t *data, uint16_t len)
{
    int ret = RET_FAIL;
    if (spps_connected) {
        ret = app_spp_send_data(&remote_addr, SPP_UUID16, data, len);
    } else if (gatts_connected) {
        ret = app_gatts_send_notify(&remote_addr, character_tx, data, len);
    }
    return ret == RET_OK;
}

void app_econn_init(void)
{
    gatts_service_t *service = NULL;

		// user add cuixu
		app_user_battery_init();
		app_user_read_data();
		app_user_spp_init();

    app_register_msg_handler(MSG_TYPE_ECONN, econn_msg_handler);

    econn_cfg_load();
    update_player_eq();
	
    //app_audio_listen_mode_force_normal(true);

    app_spp_register_service(SPP_UUID16, spp_connection_callback, spp_data_callback);

    app_gatts_register_connection_callback(gatts_connection_callback);

    service = app_gatts_register_service_ext(service_uuid128);
    assert(service);

    character_rx = app_gatts_register_character(
        service, UUID_CHARACTER_RX, GATT_PROP_READ | GATT_PROP_WRITE_WITHOUT_RSP | GATT_PROP_WRITE,
        NULL, write_callback, NULL);
    assert(character_rx);

    character_tx = app_gatts_register_character(service, UUID_CHARACTER_TX, GATT_PROP_NOTIFY, NULL,
                                                NULL, notify_enable_callback);
    assert(character_tx);
}

void app_econn_handle_sys_state(uint32_t sys_state)
{
    if (sys_state <= STATE_DISABLED) {
        gatts_connected = false;
        spps_connected = false;
    }

    if (sys_state < STATE_CONNECTED) {
        if (app_wws_is_master() && disconnect_for_spp) {
            DBGLOG_ECONN_DBG("recover ag pairing after spp disconnect\n");
            app_bt_set_discoverable_and_connectable(true, true);
        }
        disconnect_for_spp = false;

        econn_disable_adv();

        if (gatts_connected) {
            app_gatts_disconnect((BD_ADDR_T *)&remote_addr);
            gatts_connected = false;
            memset(&remote_addr, 0, 6);
        }
    } else if (app_wws_is_slave()) {   // slave connected
        econn_disable_adv();
    } else {   // master connected
        econn_enable_adv();
    }
}

void app_econn_deinit(void)
{
}

bool_t app_econn_exists(void)
{
    return true;
}

void app_econn_enter_ag_pairing(void)
{

}

void app_econn_handle_listen_mode_changed(uint8_t mode)
{
    UNUSED(mode);

    if ((!spps_connected) && (!gatts_connected)) {
        return;
    }

    if (app_listen_mode_changing) {
        DBGLOG_ECONN_DBG("app_econn_handle_listen_mode_changed ignored for app\n");
    } else {
        send_listen_mode();
    }
}

void app_econn_handle_in_ear_changed(bool_t in_ear)
{
    UNUSED(in_ear);
}

void app_econn_handle_peer_in_ear_changed(bool_t in_ear)
{
    UNUSED(in_ear);
}

void app_econn_handle_battery_level_changed(uint8_t level)
{
    UNUSED(level);

    send_battery_level();
}

void app_econn_handle_peer_battery_level_changed(uint8_t level)
{
    UNUSED(level);

    send_battery_level();
}

void app_econn_handle_tws_state_changed(bool_t connected)
{
    //if (connected) {
    //    app_audio_listen_mode_force_normal(false);
    //} else {
    //    last_battery_low = 0;
    //    app_audio_listen_mode_force_normal(true);
    //}

    app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_REPORT_TWS_STATE);
    app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_REPORT_TWS_STATE, NULL, 0, 2000);
}

void app_econn_handle_tws_role_changed(bool_t is_master)
{
    if (is_master) {
        if ((app_bt_get_sys_state() >= STATE_CONNECTED) && (!spps_connected)) {
            econn_enable_adv();
        } else {
            DBGLOG_ECONN_DBG("econn tws role changed state:0x%X spp:%d gatt:%d\n",
                             app_bt_get_sys_state(), spps_connected, gatts_connected);
        }
    } else {
        econn_disable_adv();
    }

    app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_REPORT_TWS_STATE);
    app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_REPORT_TWS_STATE, NULL, 0, 2000);
}

bool_t app_econn_handle_usr_evt(uint16_t event, bool_t from_peer)
{
    bool_t ret = false;

	DBGLOG_ECONN_DBG("[app_econn_handle_usr_evt]:event=%d, from_peer=%d\n", event, from_peer);
    switch (event) {
        case EVTUSR_FACTORY_RESET:
            DBGLOG_ECONN_DBG("customer EVTUSR_FACTORY_RESET\n");
            if (app_charger_is_charging()) {
                app_led_indicate_event(EVTUSR_FACTORY_RESET);
                app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_FACTORY_RESET, NULL, 0, 2000);
            } else {
                DBGLOG_ECONN_DBG("customer EVTUSR_FACTORY_RESET ignored\n");
            }
						app_send_msg(MSG_TYPE_PM, 2, NULL, 0);	//cuixu test
            ret = true;
            break;
        case EVTUSR_ENTER_PAIRING:
            DBGLOG_ECONN_DBG("customer EVTUSR_ENTER_PAIRING\n");
            if (app_wws_peer_is_charging() && app_wws_is_connected_master()
                && app_charger_is_charging()) {
                app_bt_enter_ag_pairing();
            } else {
                DBGLOG_ECONN_DBG("EVTUSR_ENTER_PAIRING ignored\n");
            }
            ret = true;
            break;
        default:
            break;
    }
	DBGLOG_ECONN_DBG("[app_econn_handle_usr_evt]:ret=%d\n", ret);
    return ret;
}

bool_t app_econn_handle_sys_evt(uint16_t event, void *param)
{
    bool_t ret = false;

    UNUSED(param);

	DBGLOG_ECONN_DBG("[app_econn_handle_sys_evt]:event=%d\n", event);
    switch (event) {
        case EVTSYS_BT_POWER_OFF:
            if (power_off_by_box || (!app_charger_is_charging())) {
                power_off_by_box = false;
                ret = false;
            } else {   // ignore power off led & tone in box
                ret = true;
            }
            break;
        case EVTSYS_BT_POWER_ON:
            if (app_pm_get_power_on_reason() == PM_POWER_ON_REASON_FACTORY_RESET) {
                app_led_indicate_event(EVTUSR_POWER_ON);
            }
            break;
        case EVTSYS_DISCONNECTED:
            if (app_bt_get_sys_state() == STATE_AG_PAIRING) {
                DBGLOG_ECONN_DBG("customer EVTSYS_DISCONNECTED ignored!\n");
                //ret = true;
            }
            break;

        case EVTSYS_CHARGE_CONNECTED:
            key_sensor_set_enabled(false);
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_CHARGE_CONNECTED\n");
            break;

        case EVTSYS_CHARGE_DISCONNECTED:
            key_sensor_set_enabled(true);
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_CHARGE_DISCONNECTED\n");
            break;
			
        case EVTSYS_CONNECTED:
            force_discoverable = false;
            app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_FORCE_DISCOVERABLE);
            app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_CHECK_SPP_STATE);
            app_bt_set_force_discoverable(false);
            app_led_set_custom_state(0);
            if (link_loss) {
                link_loss = false;
                DBGLOG_ECONN_DBG("customer EVTSYS_CONNECTED ignored\n");
                return true;
            }
            if (app_bt_get_sys_state() >= STATE_INCOMING_CALL) {
                DBGLOG_ECONN_DBG("customer EVTSYS_CONNECTED ignored for talking\n");
                app_led_indicate_event(event);
                return true;
            }
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_CONNECTED\n");
            break;

        case EVTSYS_BATTERY_LOW:
            if (app_wws_is_connected_slave()) {
                app_wws_send_usr_evt(EVTSYS_BATTERY_LOW);
                return true;
            }

            {
                uint32_t cur_battery_low = os_boot_time32();
                DBGLOG_ECONN_DBG(
                    "customer rcv EVTSYS_BATTERY_LOW last_battery_low:%d, cur_battery_low:%d\n",
                    last_battery_low, cur_battery_low);
                if ((last_battery_low == 0)
                    || (cur_battery_low - last_battery_low >= BATTERY_LOW_INTERVAL_MS)) {
                    last_battery_low = cur_battery_low;
                    return false;
                } else {
                    return true;
                }
            }
            break;

        case EVTSYS_WWS_ROLE_SWITCH:
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_WWS_ROLE_SWITCH\n");
            if (link_loss && force_discoverable) {
                if (app_wws_is_master()) {
                    app_bt_set_force_discoverable(true);
                    app_led_set_custom_state(STATE_AG_PAIRING);
                } else {
                    app_bt_set_force_discoverable(false);
                    app_led_set_custom_state(0);
                }
            } else {
                app_bt_set_force_discoverable(false);
                app_led_set_custom_state(0);
                force_discoverable = false;
            }
            break;

        case EVTSYS_STATE_CHANGED:
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_STATE_CHANGED\n");
            {
                BD_ADDR_T cur_connected_addr = {0};
                if (app_bt_get_sys_state() == STATE_CONNECTED) {
                    if (app_bt_get_connected_device(&cur_connected_addr)) {
                        DBGLOG_ECONN_DBG("customer EVTSYS_STATE_CHANGED get addr failed\n");
                        return false;
                    }

                    if (bdaddr_is_zero(&last_connected_addr)) {
                        memcpy(&last_connected_addr, &cur_connected_addr, sizeof(BD_ADDR_T));
                        return false;
                    }

                    if (!bdaddr_is_equal(&last_connected_addr, &cur_connected_addr)) {
                        memcpy(&last_connected_addr, &cur_connected_addr, sizeof(BD_ADDR_T));
                        link_loss = false;
                    }
                }
            }
            break;
        case EVTSYS_BOX_CMD_POWER_OFF:
            power_off_by_box = true;
            break;
        case EVTSYS_CMC_BOX_CLOSE:
            battery_charger_set_cur_limit(ntc_current_limit > 10 ? 10 : ntc_current_limit);
            break;
        case EVTSYS_CMC_BOX_OPEN:
            if (box_battery_low) {
                battery_charger_set_cur_limit(0);
            } else {
                battery_charger_set_cur_limit(ntc_current_limit);
            }
            break;

        default:
            break;
    }
	
	DBGLOG_ECONN_DBG("[app_econn_handle_sys_evt]:ret=%d\n", ret);
    return ret;
}

void econn_handle_box_battery_low(bool_t battery_low)
{
    DBGLOG_ECONN_DBG("econn_handle_box_battery_low:%d\n", battery_low);
    box_battery_low = battery_low;
    if (battery_low) {
        battery_charger_set_cur_limit(0);
    } else {
        battery_charger_set_cur_limit(ntc_current_limit);
    }
}

void app_econn_handle_ntc_value(int8_t value)
{
    uint16_t current_ma = 0;

    if (value < 0) {
        current_ma = 0;
    } else if (value < 15) {
        current_ma = 10;
    } else if (value < 46) {
        current_ma = 150;
    } else {
        current_ma = 0;
    }

    if (ntc_current_limit != current_ma) {
        DBGLOG_ECONN_DBG("charge current limited to %d according to ntc %d\n", current_ma, value);
        battery_charger_set_cur_limit(box_battery_low ? 0 : current_ma);
        ntc_current_limit = current_ma;
    }
}

void app_econn_handle_charging_changed(bool_t charging)
{
    if (charging) {
        app_led_set_enabled(true);
        app_pm_set_auto_power_off_enabled(false);
    } else {
        app_led_set_enabled(false);
		
        if (app_wws_is_master()) {
            app_send_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_CLAER_LAST_BATTERY_LOW, NULL, 0);
        } else if (app_wws_is_connected_slave()) {
            app_wws_send_remote_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_CLAER_LAST_BATTERY_LOW, 0, NULL);
        }
    }

    send_charging_state();
}

void app_econn_handle_peer_charging_changed(bool_t charging)
{
    UNUSED(charging);

    send_charging_state();
}

void app_econn_handle_box_state_changed(box_state_t state)
{
    UNUSED(state);
}

void app_econn_handle_peer_box_state_changed(box_state_t state)
{
    UNUSED(state);
}

#endif   //#if ECONN == ECONN_ZT210
