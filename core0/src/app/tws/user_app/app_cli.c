#include "types.h"
#include "string.h"
#include "os_mem.h"
#include "modules.h"
#include "cli.h"
#include "ro_cfg.h"
#include "app_evt.h"
#include "app_main.h"
#include "app_bt.h"
#include "app_cli.h"
#include "app_spp.h"
#include "app_gatts.h"
#include "app_adv.h"
#include "app_wws.h"
#include "app_econn.h"
#include "app_charger.h"
#include "app_bat.h"
#include "battery.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "key_sensor.h"
#include "led_manager.h"
#include "oem.h"
#include "app_tone.h"
#include "app_btn.h"
#include "app_inear.h"

#define APP_CLI_MSG_ID_START_ADV    1
#define APP_CLI_MSG_ID_SEND_BT_DATA 2

#ifndef APP_CLI_SPP_UUID
#define APP_CLI_SPP_UUID 0x7033
#endif

#ifndef APP_CLI_SERVICE_UUID
#define APP_CLI_SERVICE_UUID 0x7033
#endif

#ifndef APP_CLI_CHARACTER_RX_UUID
#define APP_CLI_CHARACTER_RX_UUID 0x1001
#endif

#ifndef APP_CLI_CHARACTER_TX_UUID
#define APP_CLI_CHARACTER_TX_UUID 0x1002
#endif

#ifndef APP_CLI_SPP_ENABLE
#define APP_CLI_SPP_ENABLE 1
#endif

#if APP_CLI_SPP_ENABLE
#ifdef APP_CLI_ENABLE_HEADER_CHECK
static const uint8_t cli_header[4] = {0xD0, 0xD2, 0xC5, 0xC2};
#endif
#endif

typedef struct {
    uint16_t evt;
} __attribute__((packed)) app_cli_user_event_msg_t;

typedef struct {
    uint16_t type;
    uint16_t id;
    uint8_t param[];
} __attribute__((packed)) app_cli_app_msg_t;

typedef struct {
    uint32_t cmd;
    uint8_t param[];
} __attribute__((packed)) app_cli_bt_rpc_cmd_msg_t;

typedef struct {
    uint16_t cmd;
    uint8_t param_len;
    uint8_t param[0];
} __attribute__((packed)) app_cli_hci_cmd_msg_t;

typedef struct {
    uint8_t evt;
    uint8_t param_len;
    uint8_t param[0];
} __attribute__((packed)) app_cli_hci_cmd_rsp_t;

typedef struct {
    uint8_t local_addr[6];
    uint8_t peer_addr[6];
} __attribute__((packed)) app_cli_get_bdaddr_rsp_t;

typedef struct {
    uint8_t magic_number;
} __attribute__((packed)) app_cli_tws_pair_msg_t;

typedef struct {
    uint8_t enable;
} __attribute__((packed)) app_cli_enable_hci_passthrough_msg_t;

typedef struct {
    uint8_t is_master;
    uint8_t is_left;
    uint8_t is_connected;
} __attribute__((packed)) app_cli_get_tws_state_rsp_t;

typedef struct {
    uint16_t volt;      // 0-5000 mV
    uint8_t charging;   // true or false
} __attribute__((packed)) app_cli_get_battery_state_rsp_t;

typedef struct {
    uint8_t key_id;
    uint8_t key_src;
} __attribute__((packed)) app_cli_get_key_state_msg_t;

typedef struct {
    uint8_t key_id;
    uint8_t key_src;
    bool_t pressed;
} __attribute__((packed)) app_cli_get_key_state_rsp_t;

typedef struct {
    uint8_t led_id;
    bool_t on;
} __attribute__((packed)) app_cli_control_led_msg_t;

typedef struct {
    uint8_t addr[6];
} __attribute__((packed)) app_cli_set_peer_addr_msg_t;

typedef struct {
    uint8_t addr[6];
} __attribute__((packed)) app_cli_get_oem_bdaddr_rsp_t;

typedef struct {
    uint8_t ppm;
} __attribute__((packed)) app_cli_get_oem_ppm_rsp_t;

typedef struct {
    bool_t pressed;
} __attribute__((packed)) app_cli_get_btn_pressed_rsp_t;

typedef struct {
    uint16_t ver;
    uint16_t peer_ver;
} __attribute__((packed)) app_cli_get_custom_ver_rsp_t;

typedef struct {
    uint8_t level;           // 0-100%
    uint8_t charging;        // 0 not charging ,1 charging
    uint8_t peer_level;      // 0-100%
    uint8_t peer_charging;   // 0 not charging ,1 charging
} __attribute__((packed)) app_cli_get_bat_rsp_t;

typedef struct {
    uint8_t inear;        // 0 out of ear, 1 in ear
    uint8_t peer_inear;   // 0 out of ear, 1 in ear
} __attribute__((packed)) app_cli_get_inear_rsp_t;

static gatts_character_t *character_rx = NULL;
static gatts_character_t *character_tx = NULL;

static bool_t gatts_connected = false;
static bool_t spp_connected = false;
static BD_ADDR_T remote_addr = {0};

static void user_event_msg_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cli_ret = RET_FAIL;
    app_cli_user_event_msg_t *msg = (app_cli_user_event_msg_t *)buffer;

    if (length != sizeof(app_cli_user_event_msg_t)) {
        DBGLOG_CLI_ERR("user_event_msg_handler invalid length:%d\n", length);
    } else {
        app_evt_send(msg->evt);
        cli_ret = RET_OK;
    }
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_USER_EVENT, NULL, 0, 0,
                               cli_ret);
}

static void app_msg_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cli_ret = RET_FAIL;
    app_cli_app_msg_t *msg = (app_cli_app_msg_t *)buffer;
    uint16_t param_len = 0;

    if (length < sizeof(app_cli_app_msg_t)) {
        DBGLOG_CLI_ERR("app_msg_handler invalid length:%d\n", length);
    } else if (length > sizeof(app_cli_app_msg_t)) {
        param_len = length - sizeof(app_cli_app_msg_t);
        app_send_msg((app_msg_type_t)msg->type, msg->id, msg->param, param_len);
        cli_ret = RET_OK;
    }

    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_APP_MSG, NULL, 0, 0,
                               cli_ret);
}

static void bt_rpc_cmd_msg_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cli_ret = RET_FAIL;
    app_cli_bt_rpc_cmd_msg_t *msg = (app_cli_bt_rpc_cmd_msg_t *)buffer;
    uint16_t param_len = 0;

    if (length < sizeof(app_cli_bt_rpc_cmd_msg_t)) {
        DBGLOG_CLI_ERR("bt_rpc_cmd_msg_handler invalid length:%d\n", length);
    } else if (length > sizeof(app_cli_bt_rpc_cmd_msg_t)) {
        param_len = length - sizeof(app_cli_bt_rpc_cmd_msg_t);
        app_bt_send_rpc_cmd(msg->cmd, msg->param, param_len);
        cli_ret = RET_OK;
    }
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_BT_RPC_CMD, NULL, 0, 0,
                               cli_ret);
}

static void hci_cmd_msg_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_hci_cmd_msg_t *msg = (app_cli_hci_cmd_msg_t *)buffer;
    bt_result_t ret;
    uint32_t cli_ret = RET_FAIL;
    bt_cmd_send_hci_command_t cmd_param;

    if (length < sizeof(app_cli_hci_cmd_msg_t)) {
        DBGLOG_CLI_ERR("hci_cmd_msg_handler invalid length:%d\n", length);
    } else if (length != sizeof(app_cli_hci_cmd_msg_t) + msg->param_len) {
        DBGLOG_CLI_ERR("app_cli hci_cmd_msg_handler length:%d error\n", length);
    } else {
        cmd_param.cmd = msg->cmd;
        cmd_param.param_len = msg->param_len;
        cmd_param.param = msg->param;
        ret = bt_handle_user_cmd(BT_CMD_SEND_HCI_COMMAND, &cmd_param, sizeof(cmd_param));
        if (ret == BT_RESULT_SUCCESS) {
            cli_ret = RET_OK;
        }
        DBGLOG_CLI_DBG("app_cli send hci cmd %04X ret=%d\n", cmd_param.cmd, ret);
    }
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_HCI_CMD, NULL, 0, 0,
                               cli_ret);
}

static void hci_data_msg_handler(uint8_t *buffer, uint32_t length)
{
    bt_result_t ret;
    uint32_t cli_ret = RET_FAIL;
    bt_cmd_send_hci_data_t cmd_param;

    cmd_param.data_len = length;
    cmd_param.data = buffer;

    ret = bt_handle_user_cmd(BT_CMD_SEND_HCI_DATA, &cmd_param, sizeof(cmd_param));
    if (ret == BT_RESULT_SUCCESS) {
        cli_ret = RET_OK;
    }
    DBGLOG_CLI_DBG("app_cli send hci data len=%04X ret=%d\n", length, cli_ret);
}

static void get_bdaddr_msg_handler(uint8_t *buffer, uint32_t length)
{
    const BD_ADDR_T *local_addr = app_bt_get_local_address();
    const BD_ADDR_T *peer_addr = app_wws_get_peer_addr();
    app_cli_get_bdaddr_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    memcpy(rsp.local_addr, local_addr, 6);
    memcpy(rsp.peer_addr, peer_addr, 6);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BDADDR, (uint8_t *)&rsp,
                               sizeof(rsp), 0, RET_OK);
}

static void tws_pair_msg_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cli_ret = RET_FAIL;
    app_cli_tws_pair_msg_t *msg;
    if (length != sizeof(app_cli_tws_pair_msg_t)) {
        DBGLOG_CLI_ERR("tws_pair_msg_handler invalid length:%d\n", length);
    } else {
        msg = (void *)buffer;
        app_wws_start_pair(app_econn_get_vid(), app_econn_get_pid(), msg->magic_number);
        cli_ret = RET_OK;
    }
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_TWS_PAIR, NULL, 0, 0,
                               cli_ret);
}

static void enable_hci_passthrough_handler(uint8_t *buffer, uint32_t length)
{
    bt_result_t ret;
    uint32_t cli_ret = RET_FAIL;
    app_cli_enable_hci_passthrough_msg_t *msg = (app_cli_enable_hci_passthrough_msg_t *)buffer;
    bt_cmd_set_hci_evt_report_enabled_t param;

    if (length != sizeof(app_cli_enable_hci_passthrough_msg_t)) {
        DBGLOG_CLI_ERR("enable_hci_passthrough_handler invalid length:%d\n", length);
    } else {
        param.enabled = (bool_t)(msg->enable);
        ret = bt_handle_user_cmd(BT_CMD_SET_HCI_EVT_REPORT_ENABLED, &param, sizeof(param));
        if (ret == BT_RESULT_SUCCESS) {
            cli_ret = RET_OK;
        }
        DBGLOG_CLI_DBG("enable_hci_passthrough_handler ret=%d\n", ret);
    }
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_ENABLE_HCI_PASSTHROUGH, NULL,
                               0, 0, cli_ret);
}

static void get_tws_state_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_tws_state_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    rsp.is_connected = app_wws_is_connected();
    rsp.is_left = app_wws_is_left();
    rsp.is_master = app_wws_is_master();

    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_TWS_STATE,
                               (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
}

static void get_battery_state_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_battery_state_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    rsp.volt = battery_get_voltage_mv();
    rsp.charging = app_charger_is_charging();

    if (rsp.volt < UVP_VOLT_MV) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BATTERY_STATE,
                                   (uint8_t *)&rsp, sizeof(rsp), 0, RET_FAIL);
    } else {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BATTERY_STATE,
                                   (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
    }
}

static void get_key_state_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_key_state_msg_t *param;
    app_cli_get_key_state_rsp_t rsp;

    if (length != sizeof(app_cli_get_key_state_msg_t)) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_GET_KEY_STATE length:%d error\n", length);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_KEY_STATE, NULL, 0,
                                   0, RET_FAIL);
    } else {
        param = (app_cli_get_key_state_msg_t *)buffer;
        rsp.key_id = param->key_id;
        rsp.key_src = param->key_src;
        rsp.pressed = key_sensor_is_key_pressed(param->key_id, param->key_src);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_KEY_STATE,
                                   (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
    }
}

static void control_led_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_control_led_msg_t *param;
    RESOURCE_GPIO_ID led_id;
    uint32_t ret;
    led_param_t led_param;

    if (length != sizeof(app_cli_control_led_msg_t)) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_CONTROL_LED length:%d error\n", length);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    param = (app_cli_control_led_msg_t *)buffer;
    led_id = param->led_id;

    if (led_id >= LED_MAX_NUM) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_CONTROL_LED leled_idngth:%d error\n", led_id);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    DBGLOG_CLI_ERR("APP_CLI_MSGID_CONTROL_LED led:%d on:%d\n", led_id, param->on);

    led_off(led_id);

    if (!param->on) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                                   RET_OK);
        return;
    }

    led_param.mode = LED_MODE_NORMAL_LIGHT;
    led_param.on_duty = 1000;
    led_param.off_duty = 0;
    led_param.blink_cnt = 1;
    led_param.dim_duty = 1000;
    led_param.interval = 100;
    led_param.loop = 0;

    ret = led_config(led_id, &led_param, NULL);
    if (RET_OK != ret) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_CONTROL_LED led_config error %d\n", ret);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    ret = led_start_action(led_id, true);
    if (RET_OK != ret) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_CONTROL_LED led_start_action ERROR %d\n", ret);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, NULL, 0, 0,
                               RET_OK);
}

static void set_peer_addr_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_set_peer_addr_msg_t *param;
    BD_ADDR_T peer_addr;

    if (length != sizeof(app_cli_set_peer_addr_msg_t)) {
        DBGLOG_CLI_ERR("APP_CLI_MSGID_SET_PEER_ADDR len:%d error\n", length);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SET_PEER_ADDR, NULL, 0,
                                   0, RET_FAIL);
        return;
    }

    param = (app_cli_set_peer_addr_msg_t *)buffer;
    memcpy(&peer_addr.addr, &param->addr, 6);

    if (app_bt_set_peer_addr(&peer_addr)) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SET_PEER_ADDR, NULL, 0,
                                   0, RET_FAIL);
    } else {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SET_PEER_ADDR, NULL, 0,
                                   0, RET_OK);
    }
}

static void get_oem_bdaddr_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_oem_bdaddr_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    if (oem_data_get_mac_addr(rsp.addr)) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_BDADDR, NULL, 0,
                                   0, RET_FAIL);
    } else {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_BDADDR,
                                   (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
    }
}

static void enable_battery_report_handler(uint8_t *buffer, uint32_t length)
{
    if (length < 1 || buffer == NULL) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_BATTERY_REPORT, NULL,
                                   0, 0, RET_FAIL);
    }
    app_bat_enable_report((bool_t)buffer[0]);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_BATTERY_REPORT, NULL, 0,
                               0, RET_OK);
}

static void enable_tone_handler(uint8_t *buffer, uint32_t length)
{
    if (length < 1 || buffer == NULL) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_TONE, NULL, 0, 0,
                                   RET_FAIL);
    }
    app_tone_enable((bool_t)buffer[0]);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_TONE, NULL, 0, 0, RET_OK);
}

static void get_oem_ppm_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_oem_ppm_rsp_t oem_ppm_rsp;

    UNUSED(buffer);
    UNUSED(length);

    if (oem_data_get_ppm(&oem_ppm_rsp.ppm)) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_PPM, NULL, 0, 0,
                                   RET_FAIL);
    } else {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_PPM,
                                   (uint8_t *)&oem_ppm_rsp, sizeof(oem_ppm_rsp), 0, RET_OK);
    }
}

static void get_btn_pressed_handler(uint8_t *buffer, uint32_t length)
{
    app_cli_get_btn_pressed_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    rsp.pressed = app_btn_all_released() ? false : true;
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BTN_PRESSED,
                               (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
}

static void get_bt_name_handler(uint8_t *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);

    const char *name = app_bt_get_local_name();
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BT_NAME, (uint8_t *)name,
                               strlen(name), 0, RET_OK);
}

static void get_custom_ver_handler(uint8_t *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);

    app_cli_get_custom_ver_rsp_t rsp;
#ifdef CUSTOM_VERSION
    rsp.ver = CUSTOM_VERSION;
#else
    rsp.ver = FIRMWARE_VERSION_BUILD;
#endif
    rsp.peer_ver = app_wws_peer_get_fw_ver_build();
    DBGLOG_LIB_CLI_INFO("cli get custom version %d peer_ver %d\n", rsp.ver, rsp.peer_ver);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_CUSTOM_VER,
                               (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
}

static void get_bat_handler(uint8_t *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);

    app_cli_get_bat_rsp_t rsp;
    rsp.level = app_bat_get_level();
    rsp.charging = app_charger_is_charging();
    rsp.peer_level = app_wws_peer_get_battery_level();
    rsp.peer_charging = app_wws_peer_is_charging();

    DBGLOG_LIB_CLI_INFO("cli get bat %d charging %d peer bat %d charging %d\n", rsp.level,
                        rsp.charging, rsp.peer_level, rsp.peer_charging);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BAT, (uint8_t *)&rsp,
                               sizeof(rsp), 0, RET_OK);
}

static void get_inear_handler(uint8_t *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);

    app_cli_get_inear_rsp_t rsp;
    rsp.inear = app_inear_get();
    rsp.peer_inear = app_wws_peer_is_inear();

    DBGLOG_LIB_CLI_INFO("cli get inear %d peer_inear %d\n", rsp.inear, rsp.peer_inear);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_INEAR, (uint8_t *)&rsp,
                               sizeof(rsp), 0, RET_OK);
}

static void get_anc_handler(uint8_t *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);
    uint8_t anc_mode = (uint8_t)app_audio_listen_mode_get();
    DBGLOG_LIB_CLI_INFO("cli get anc mode %d\n", anc_mode);
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_ANC,
                               (uint8_t *)&anc_mode, sizeof(anc_mode), 0, RET_OK);
}

static void do_send_bt_data(void *_ptr)
{
    uint8_t *ptr = *((void **)_ptr);
    uint16_t len = *((uint16_t *)ptr);
    uint8_t *data = ptr + 2;
    if (gatts_connected) {
        DBGLOG_CLI_DBG("cli send_notify len:%d\n", len);
        app_gatts_send_notify(&remote_addr, character_tx, data, len);
    } else if (spp_connected) {
        DBGLOG_CLI_DBG("cli send_spp len:%d\n", len);
        app_spp_send_data(&remote_addr, APP_CLI_SPP_UUID, data, len);
    }
    os_mem_free(ptr);
}

static int32_t send_cli_data(const uint8_t *data, uint32_t len)
{
    if ((len > 512) || (len == 0)) {
        DBGLOG_CLI_ERR("send_cli_data error, len:%d\n", len);
        return len;
    }

    if ((!gatts_connected) && (!spp_connected)) {
        //DBGLOG_CLI_ERR("send_cli_data error, not connected\n");
        return len;
    }

    uint8_t *ptr = os_mem_malloc(IOT_APP_MID, len + 2);
    ((uint16_t *)ptr)[0] = (uint16_t)len;
    memcpy(ptr + 2, data, len);

    if (app_send_msg(MSG_TYPE_CLI, APP_CLI_MSG_ID_SEND_BT_DATA, &ptr, sizeof(void *))) {
        os_mem_free(ptr);
    }

    return len;
}

static void handle_cli_data(uint8_t *data, uint16_t len)
{
    UNUSED(len);
    UNUSED(data);

    generic_transmission_io_recv(GENERIC_TRANSMISSION_IO_SPP, data, len, false);
}

static void handle_cli_connection_status(bool_t connected)
{
    if (connected) {
        //TODO: gtp_switch_module_to_bt()
    } else {
        //TODO: gtp_switch_module_to_bt()
    }
}
#if APP_CLI_SPP_ENABLE == 1
static void spp_connection_callback(BD_ADDR_T *addr, uint16_t uuid, bool_t connected)
{
    UNUSED(uuid);

    if (connected) {
        DBGLOG_CLI_DBG("cli spp connected\n");
        spp_connected = true;
        remote_addr = *addr;
        handle_cli_connection_status(true);
    } else {
        DBGLOG_CLI_DBG("cli spp disconnected\n");
        spp_connected = false;
        handle_cli_connection_status(false);
    }
}

static void spp_data_callback(BD_ADDR_T *addr, uint16_t uuid, uint8_t *data, uint16_t len)
{
    UNUSED(uuid);
    UNUSED(addr);

#ifdef APP_CLI_ENABLE_HEADER_CHECK
    if ((len > sizeof(cli_header) && (!memcmp(data, cli_header, sizeof(cli_header))))) {
#else
    if (1) {
#endif   //APP_CLI_ENABLE_HEADER_CHECK
        if (app_wws_is_master()) {
            DBGLOG_CLI_DBG("cli spp data len:%d\n", len);
            handle_cli_data(data, len);
        } else {
            DBGLOG_CLI_DBG("cli spp data len:%d, ignore for slave\n", len);
        }
    }
    os_mem_free(data);
}
#endif   //APP_CLI_SPP_ENABLE == 1

static void gatts_connection_callback(BD_ADDR_T *addr, bool_t is_ble, bool_t connected)
{
    UNUSED(is_ble);

    if ((!connected) && bdaddr_is_equal(&remote_addr, addr)) {
        DBGLOG_CLI_DBG("cli connection disconnected\n");
        gatts_connected = false;
        handle_cli_connection_status(false);
    }
}

static void write_callback(BD_ADDR_T *addr, gatts_character_t *character, uint8_t *data,
                           uint16_t len)
{
    UNUSED(character);

    if (!gatts_connected) {
        DBGLOG_CLI_DBG("cli gatt connected\n");
        gatts_connected = true;
        remote_addr = *addr;
        handle_cli_connection_status(true);
    }

    DBGLOG_CLI_DBG("cli write_callback len:%d\n", len);

    handle_cli_data(data, len);
    os_mem_free(data);
}

static void notify_enable_callback(BD_ADDR_T *addr, gatts_character_t *character, bool_t enabled)
{
    UNUSED(character);
    UNUSED(enabled);

    if (!gatts_connected) {
        DBGLOG_CLI_DBG("cli gatt connected\n");
        gatts_connected = true;
        remote_addr = *addr;
        handle_cli_connection_status(true);
    }

    DBGLOG_CLI_DBG("cli notify_enable_callback enabled:%d\n", enabled);
}

static void start_adv(void)
{
    const BD_ADDR_T *addr = app_bt_get_local_address();
    uint8_t random_addr[6];
    static uint8_t adv_data[] = {0x02, 0x01, 0x06,   //
                                 0x0F, 0xFF, 0x6E, 0x07, 0x01, 0x02, 0x03, 0x04,
                                 0x05, 0x06, 'w',  'q',  '-',  'c',  'l',  'i'};
    adv_data[7 + 0] = addr->addr[5];
    adv_data[7 + 1] = addr->addr[4];
    adv_data[7 + 2] = addr->addr[3];
    adv_data[7 + 3] = addr->addr[2];
    adv_data[7 + 4] = addr->addr[1];
    adv_data[7 + 5] = addr->addr[0];

    random_addr[0] = addr->addr[5] + 1;
    random_addr[1] = addr->addr[4];
    random_addr[2] = addr->addr[3];
    random_addr[3] = addr->addr[2];
    random_addr[4] = addr->addr[1];
    random_addr[5] = addr->addr[0] + 1;

    app_adv_set_random_address(random_addr);
    app_adv_set_adv_data(adv_data, sizeof(adv_data));
    app_adv_set_interval(0x400, 0x800);
    app_adv_set_enabled(true);

    DBGLOG_CLI_DBG("cli start adv\n");
}

static void app_cli_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);

    switch (msg_id) {
        case APP_CLI_MSG_ID_START_ADV: {
            if (app_wws_is_master()) {
                start_adv();
            }
            break;
        }
        case APP_CLI_MSG_ID_SEND_BT_DATA:
            do_send_bt_data(param);
            break;
        default:
            break;
    }
}

void app_cli_handle_hci_evt(uint8_t evt, uint8_t param_len, uint8_t *param)
{
    app_cli_hci_cmd_rsp_t *rsp;

    rsp = os_mem_malloc(IOT_APP_MID, sizeof(app_cli_hci_cmd_rsp_t) + param_len);
    assert(rsp);

    rsp->evt = evt;
    rsp->param_len = param_len;
    if (param_len) {
        memcpy(rsp->param, param, param_len);
    }

    cli_interface_msg_ind(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_HCI_CMD, (uint8_t *)rsp,
                          sizeof(app_cli_hci_cmd_rsp_t) + param_len, 0, RET_OK);

    os_mem_free(rsp);
}

void app_cli_handle_hci_data(uint16_t data_len, uint8_t *data)
{
    cli_interface_msg_ind(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_HCI_DATA, data, data_len, 0,
                          RET_OK);
}

void app_cli_init(void)
{
    static generic_transmission_io_method_t gtp_methods = {send_cli_data, NULL, NULL, NULL};

    generic_transmission_io_method_register(GENERIC_TRANSMISSION_IO_SPP, &gtp_methods);

    app_register_msg_handler(MSG_TYPE_CLI, app_cli_handle_msg);
#if APP_CLI_SPP_ENABLE == 1
    app_spp_register_service(APP_CLI_SPP_UUID, spp_connection_callback, spp_data_callback);
#endif
    app_gatts_register_connection_callback(gatts_connection_callback);

    gatts_service_t *service = app_gatts_register_service(APP_CLI_SERVICE_UUID);
    assert(service);

    character_rx =
        app_gatts_register_character(service, APP_CLI_CHARACTER_RX_UUID,
                                     GATT_PROP_WRITE_WITHOUT_RSP, NULL, write_callback, NULL);
    assert(character_rx);

    character_tx = app_gatts_register_character(
        service, APP_CLI_CHARACTER_TX_UUID, GATT_PROP_NOTIFY, NULL, NULL, notify_enable_callback);
    assert(character_tx);
    if (app_econn_exists() == false) {
        app_send_msg_delay(MSG_TYPE_CLI, APP_CLI_MSG_ID_START_ADV, NULL, 0, 8000);
    }
}

void app_cli_deinit(void)
{
}

void app_cli_handle_sys_state(uint32_t state)
{
    UNUSED(state);
}

CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_USER_EVENT, user_event_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_APP_MSG, app_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_BT_RPC_CMD, bt_rpc_cmd_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_HCI_CMD, hci_cmd_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BDADDR, get_bdaddr_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_TWS_PAIR, tws_pair_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SEND_HCI_DATA, hci_data_msg_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_ENABLE_HCI_PASSTHROUGH,
                enable_hci_passthrough_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_TWS_STATE, get_tws_state_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BATTERY_STATE,
                get_battery_state_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_KEY_STATE, get_key_state_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_CONTROL_LED, control_led_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_SET_PEER_ADDR, set_peer_addr_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_BDADDR, get_oem_bdaddr_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_BATTERY_REPORT,
                enable_battery_report_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_EN_TONE, enable_tone_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_OEM_PPM, get_oem_ppm_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BTN_PRESSED, get_btn_pressed_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BT_NAME, get_bt_name_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_CUSTOM_VER, get_custom_ver_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_BAT, get_bat_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_INEAR, get_inear_handler);
CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_ANC, get_anc_handler);
