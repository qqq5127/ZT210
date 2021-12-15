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
#include "types.h"
#include "assert.h"
#include "string.h"
#include "app_bt.h"
#include "usr_cfg.h"
#include "app_audio.h"
#include "ro_cfg.h"
#include "app_main.h"

#define USR_CFG_VERSION (8)

#define USR_CFG_MSG_ID_SAVE_CFG 1

#ifndef USR_CFG_SAVE_DELAY_MS
#define USR_CFG_SAVE_DELAY_MS 1000
#endif

typedef struct {
    BD_ADDR_T addr;
    uint8_t music_vol;
    uint8_t call_vol;
    bt_a2dp_codec_t a2dp_codec : 8;
    bt_phone_type_t phone_type : 8;
} pdl_item_t;

typedef struct {
    /** cfg version, should restore to default if version not match */
    uint16_t version;
    /** set if cfg modified after load from flash */
    bool_t dirty;
    char local_name[MAX_NAME_LEN];
    /** circle buffer to save paired list */
    uint8_t language_id;
    uint8_t listen_mode;
    uint8_t listen_mode_cfg;
    uint8_t anc_level;
    uint8_t transparency_level;
    uint8_t adv_cnt;
    bool_t inear_enabled;
    uint16_t battery_low_prompt_interval;
} usr_cfg_t;

typedef struct {
    bool_t dirty;
    /** the head of the paired list*/
    uint8_t head;
    pdl_item_t items[USR_CFG_MAX_PAIR_LIST];
} usr_pdl_t;

static usr_cfg_t the_cfg;
static usr_cfg_t *cfg = &the_cfg;
static usr_pdl_t the_pdl;
static usr_pdl_t *pdl = &the_pdl;

static void usr_cfg_default(void)
{
    uint8_t ro_listen_mode = ro_gen_cfg()->default_listen_mode;
    uint8_t ro_listen_mode_config = ro_gen_cfg()->default_listen_mode_toggle_config;

    memset(cfg, 0, sizeof(usr_cfg_t));
    memset(pdl, 0, sizeof(usr_pdl_t));

    assert(ro_vol_cfg()->default_music_volume <= 15);
    assert(ro_vol_cfg()->default_call_volume <= 15);

    for (int i = 0; i < USR_CFG_MAX_PAIR_LIST; i++) {
        pdl->items[i].music_vol = (ro_vol_cfg()->default_music_volume + 1) * 8 - 1;
        pdl->items[i].call_vol = ro_vol_cfg()->default_call_volume;
    }

    cfg->version = USR_CFG_VERSION;
    pdl->head = 0;

    switch (ro_listen_mode) {
        case RO_LISTEN_MODE_NORMAL:
            cfg->listen_mode = LISTEN_MODE_NORMAL;
            break;
        case RO_LISTEN_MODE_ANC:
            cfg->listen_mode = LISTEN_MODE_ANC;
            break;
        case RO_LISTEN_MODE_TRANSPARENCY:
            cfg->listen_mode = LISTEN_MODE_TRANSPARENCY;
            break;
        default:
            cfg->listen_mode = LISTEN_MODE_NORMAL;
            break;
    }
    cfg->anc_level = DEFAULT_ANC_LEVEL;
    cfg->transparency_level = DEFAULT_TRANSPARENCY_LEVEL;
    cfg->listen_mode_cfg = ro_listen_mode_config;
    cfg->inear_enabled = true;
    cfg->battery_low_prompt_interval = ro_bat_cfg()->battery_low_prompt_interval
        + (ro_bat_cfg()->battery_low_prompt_interval_high << 8);
}

static void pdl_save(void)
{
    pdl->dirty = true;
    DBGLOG_CFG_DBG("pdl_save delayed\n");
    app_cancel_msg(MSG_TYPE_USR_CFG, USR_CFG_MSG_ID_SAVE_CFG);
    app_send_msg_delay(MSG_TYPE_USR_CFG, USR_CFG_MSG_ID_SAVE_CFG, NULL, 0, USR_CFG_SAVE_DELAY_MS);
}

static void usr_cfg_save(void)
{
    cfg->dirty = true;
    DBGLOG_CFG_DBG("usr_cfg_save delayed\n");
    app_cancel_msg(MSG_TYPE_USR_CFG, USR_CFG_MSG_ID_SAVE_CFG);
    app_send_msg_delay(MSG_TYPE_USR_CFG, USR_CFG_MSG_ID_SAVE_CFG, NULL, 0, USR_CFG_SAVE_DELAY_MS);
}

static void usr_cfg_load(void)
{
    uint32_t len;
    uint32_t ret;

    len = sizeof(usr_cfg_t);
    ret = storage_read(APP_BASE_ID, APP_USR_CFG_KEY_ID, cfg, &len);
    if ((len != sizeof(usr_cfg_t)) || (ret != RET_OK)) {
        DBGLOG_CFG_ERR("usr_cfg_load error len:%d ret:%d\n", len, ret);
        memset(cfg, 0, sizeof(usr_cfg_t));
    } else {
        DBGLOG_CFG_DBG("usr_cfg_load succeed\n");
    }

    len = sizeof(usr_pdl_t);
    ret = storage_read(APP_BASE_ID, APP_PDL_KEY_ID, pdl, &len);
    if ((len != sizeof(usr_pdl_t)) || (ret != RET_OK)) {
        DBGLOG_CFG_ERR("usr_cfg_load pdl error len:%d ret:%d\n", len, ret);
        memset(pdl, 0, sizeof(usr_pdl_t));
    } else {
        DBGLOG_CFG_DBG("usr_cfg_load pdl succeed\n");
    }
}

static void usr_cfg_write_flash(void)
{
    uint32_t ret;

    if (cfg->dirty) {
        cfg->dirty = false;
        ret = storage_write(APP_BASE_ID, APP_USR_CFG_KEY_ID, cfg, sizeof(usr_cfg_t));
        DBGLOG_CFG_DBG("usr_cfg_write_flash ret:%d\n", ret);
    }

    if (pdl->dirty) {
        pdl->dirty = false;
        ret = storage_write(APP_BASE_ID, APP_PDL_KEY_ID, pdl, sizeof(usr_pdl_t));
        DBGLOG_CFG_DBG("usr_cfg_write_flash pdl ret:%d\n", ret);
    }
}

static void usr_cfg_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);

    switch (msg_id) {
        case USR_CFG_MSG_ID_SAVE_CFG:
            if (app_bt_get_sys_state() > STATE_CONNECTED) {
                //delay when music or call is running
                app_send_msg_delay(MSG_TYPE_USR_CFG, USR_CFG_MSG_ID_SAVE_CFG, NULL, 0,
                                   USR_CFG_SAVE_DELAY_MS);
            } else {
                usr_cfg_write_flash();
            }
            break;
        default:
            break;
    }
}

uint8_t usr_cfg_get_music_vol(void)
{
    return pdl->items[pdl->head].music_vol;
}

uint8_t usr_cfg_get_call_vol(void)
{

    return pdl->items[pdl->head].call_vol;
}

void usr_cfg_set_music_vol(uint8_t vol)
{
    assert(vol <= MUSIC_VOLUME_LEVEL_MAX);
    if (pdl->items[pdl->head].music_vol != vol) {
        pdl->items[pdl->head].music_vol = vol;
        pdl_save();
    }
}

void usr_cfg_set_call_vol(uint8_t vol)
{
    assert(vol <= CALL_VOLUME_LEVEL_MAX);
    if (pdl->items[pdl->head].call_vol != vol) {
        pdl->items[pdl->head].call_vol = vol;
        pdl_save();
    }
}

uint8_t usr_cfg_get_cur_language(void)
{
    return cfg->language_id;
}

void usr_cfg_set_cur_language(uint8_t language_id)
{
    if (cfg->language_id != language_id) {
        cfg->language_id = language_id;
        usr_cfg_save();
    }
}

void usr_cfg_set_local_name(const char *name)
{
    assert(name);
    if (strcmp(name, cfg->local_name)) {
        memset(cfg->local_name, 0, sizeof(cfg->local_name));
        strlcpy(cfg->local_name, name, sizeof(cfg->local_name));
        usr_cfg_save();
    }
}

uint8_t usr_cfg_get_listen_mode(void)
{
    return cfg->listen_mode;
}

uint8_t usr_cfg_get_anc_level(void)
{
    return cfg->anc_level;
}

void usr_cfg_set_anc_level(uint8_t level)
{
    if (cfg->anc_level != level) {
        cfg->anc_level = level;
        usr_cfg_save();
    }
}

uint8_t usr_cfg_get_transparency_level(void)
{
    return cfg->transparency_level;
}

void usr_cfg_set_transparency_level(uint8_t level)
{
    if (cfg->transparency_level != level) {
        cfg->transparency_level = level;
        usr_cfg_save();
    }
}

void usr_cfg_set_listen_mode(uint8_t mode)
{
    if (cfg->listen_mode != mode) {
        cfg->listen_mode = mode;
        usr_cfg_save();
    }
}

uint8_t usr_cfg_get_listen_mode_cfg(void)
{
    return cfg->listen_mode_cfg;
}

void usr_cfg_set_listen_mode_cfg(uint8_t mode_cfg)
{
    if (cfg->listen_mode_cfg != mode_cfg) {
        cfg->listen_mode_cfg = mode_cfg;
        usr_cfg_save();
    }
}

uint8_t usr_cfg_get_adv_cnt(void)
{
    return cfg->adv_cnt;
}

void usr_cfg_set_adv_cnt(uint8_t adv_cnt)
{
    if (cfg->adv_cnt != adv_cnt) {
        cfg->adv_cnt = adv_cnt;
        usr_cfg_save();
    }
}
bool_t usr_cfg_get_inear_enabled(void)
{
    return cfg->inear_enabled;
}

void usr_cfg_set_battery_low_prompt_interval(uint16_t interval_s)
{
    if (cfg->battery_low_prompt_interval != interval_s) {
        cfg->battery_low_prompt_interval = interval_s;
        usr_cfg_save();
    }
}

uint16_t usr_cfg_get_battery_low_prompt_interval(void)
{
    return cfg->battery_low_prompt_interval;
}

void usr_cfg_set_inear_enabled(bool_t inear_enabled)
{
    if (cfg->inear_enabled != inear_enabled) {
        cfg->inear_enabled = inear_enabled;
        usr_cfg_save();
    }
}

void usr_cfg_get_local_name(char *name, int max_name_len)
{
    assert(name);
    assert(max_name_len);

    strlcpy(name, cfg->local_name, max_name_len);
}

void usr_cfg_pdl_add(BD_ADDR_T *addr)
{
    uint8_t index;
    pdl_item_t item;

    if (!addr) {
        DBGLOG_CFG_ERR("usr_cfg_pdl_add addr==NULL\n");
        return;
    }

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
        DBGLOG_CFG_DBG("usr_cfg_pdl_add already first\n");
        return;
    }

    memset(&item, 0, sizeof(pdl_item_t));
    memcpy(&item.addr, addr, sizeof(BD_ADDR_T));
    item.music_vol = (ro_vol_cfg()->default_music_volume + 1) * 8 - 1;
    item.call_vol = ro_vol_cfg()->default_call_volume;

    if (bdaddr_is_zero(&pdl->items[index].addr)) {
        memcpy(&pdl->items[index], &item, sizeof(pdl_item_t));
        pdl_save();
        DBGLOG_CFG_DBG("usr_cfg_pdl_add fisrt succeed\n");
        return;
    }

    do {
        if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
            memcpy(&item, &pdl->items[index], sizeof(pdl_item_t));
            usr_cfg_pdl_remove(addr);
            break;
        }
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    } while (index != pdl->head);

    index = (pdl->head + USR_CFG_MAX_PAIR_LIST - 1) % USR_CFG_MAX_PAIR_LIST;
    pdl->head = index;
    memcpy(&pdl->items[index], &item, sizeof(pdl_item_t));
    pdl_save();
    DBGLOG_CFG_DBG("usr_cfg_pdl_add succeed\n");
}

void usr_cfg_pdl_remove(BD_ADDR_T *addr)
{
    uint8_t index, next;

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
        memset(&pdl->items[index], 0, sizeof(pdl_item_t));
        index = (pdl->head + 1) % USR_CFG_MAX_PAIR_LIST;
        pdl->head = index;
        pdl_save();
        DBGLOG_CFG_DBG("usr_cfg_pdl_remove first succeed\n");
        return;
    }

    do {
        if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
            break;
        }
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    } while (index != pdl->head);

    if (index == pdl->head) {
        DBGLOG_CFG_ERR("usr_cfg_pdl_remove not found\n");
        return;
    }

    memset(&pdl->items[index], 0, sizeof(pdl_item_t));
    next = (index + 1) % USR_CFG_MAX_PAIR_LIST;

    while (next != pdl->head) {
        memcpy(&pdl->items[index], &pdl->items[next], sizeof(pdl_item_t));
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
        next = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    }
    memset(&pdl->items[index], 0, sizeof(pdl_item_t));
    pdl_save();

    DBGLOG_CFG_DBG("usr_cfg_pdl_remove succeed\n");
}

void usr_cfg_pdl_get_next(BD_ADDR_T *addr)
{
    uint8_t index;

    if (!addr) {
        DBGLOG_CFG_ERR("usr_cfg_pdl_get_next addr==NULL\n");
        return;
    }

    if (bdaddr_is_zero(addr)) {
        index = pdl->head;
        assert(index < USR_CFG_MAX_PAIR_LIST);
        memcpy(addr, &pdl->items[index].addr, sizeof(BD_ADDR_T));
        return;
    }

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    do {
        if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
            index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
            if (index == pdl->head) {
                memset(addr, 0, sizeof(BD_ADDR_T));
                DBGLOG_CFG_DBG("usr_cfg_pdl_get_next reach end\n");
            } else {
                memcpy(addr, &pdl->items[index].addr, sizeof(BD_ADDR_T));
            }
            return;
        }
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    } while (index != pdl->head);

    // not found
    index = pdl->head;
    memcpy(addr, &pdl->items[index].addr, sizeof(BD_ADDR_T));
}

void usr_cfg_pdl_get_first(BD_ADDR_T *addr)
{
    uint8_t index;

    if (!addr) {
        DBGLOG_CFG_ERR("usr_cfg_pdl_get_first addr==NULL\n");
        return;
    }

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    memcpy(addr, &pdl->items[index].addr, sizeof(BD_ADDR_T));
}

bool_t usr_cfg_pdl_is_empty(void)
{
    BD_ADDR_T addr = {0};

    usr_cfg_pdl_get_first(&addr);

    return bdaddr_is_zero(&addr);
}

void usr_cfg_set_a2dp_codec(const BD_ADDR_T *addr, bt_a2dp_codec_t codec)
{
    uint8_t index;

    assert(addr);

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    do {
        if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
            if (pdl->items[index].a2dp_codec != codec) {
                pdl->items[index].a2dp_codec = codec;
                pdl_save();
                DBGLOG_CFG_DBG("usr_cfg_set_a2dp_codec %d\n", codec);
            }
            break;
        }
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    } while (index != pdl->head);
}

bt_a2dp_codec_t usr_cfg_get_a2dp_codec(const BD_ADDR_T *addr)
{
    uint8_t index;

    assert(addr);

    index = pdl->head;
    assert(index < USR_CFG_MAX_PAIR_LIST);

    do {
        if (bdaddr_is_equal(&pdl->items[index].addr, addr)) {
            return pdl->items[index].a2dp_codec;
        }
        index = (index + 1) % USR_CFG_MAX_PAIR_LIST;
    } while (index != pdl->head);

    return A2DP_CODEC_UNKNOWN;
}

bt_phone_type_t usr_cfg_get_phone_type(void)
{
    return pdl->items[pdl->head].phone_type;
}

void usr_cfg_set_phone_type(bt_phone_type_t type)
{
    if (pdl->items[pdl->head].phone_type != type) {
        pdl->items[pdl->head].phone_type = type;
        pdl_save();
    }
}

void usr_cfg_init(void)
{
    app_register_msg_handler(MSG_TYPE_USR_CFG, usr_cfg_handle_msg);
    usr_cfg_load();
    if (cfg->version != USR_CFG_VERSION) {
        DBGLOG_CFG_DBG("usr_cfg version error %d:%d, load default\n", cfg->version,
                       USR_CFG_VERSION);
        usr_cfg_default();
        pdl->dirty = true;
        usr_cfg_save();
        return;
    }

    DBGLOG_CFG_DBG("usr_cfg_init succeed\n");
}

void usr_cfg_reset_pdl(void)
{
    memset(pdl->items, 0, sizeof(pdl->items));
    pdl->head = 0;
    pdl_save();
    DBGLOG_CFG_DBG("usr_cfg_reset_pdl done\n");
}

void usr_cfg_reset(void)
{
    usr_cfg_default();
    pdl->dirty = true;
    usr_cfg_save();
    DBGLOG_CFG_DBG("usr_cfg_reset done\n");
}

void usr_cfg_deinit(void)
{
    usr_cfg_write_flash();
}
