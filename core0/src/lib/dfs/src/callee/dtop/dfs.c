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
#include "lib_dbglog.h"
#include "generic_list.h"
#include "modules.h"
#include "utils.h"
#include "riscv_cpu.h"
#include "critical_sec.h"

#include "os_lock.h"
#include "os_mem.h"
#include "os_timer.h"

#include "iot_rtc.h"
#include "iot_clock.h"
#include "iot_flash.h"
#include "iot_ipc.h"
#include "iot_soc.h"
#include "iot_dsp.h"
#include "iot_suspend_sched.h"
#include "iot_share_task.h"

#include "dfs.h"

#include "storage_controller.h"

#ifdef LOW_POWER_ENABLE
#include "iot_share_task.h"
#include "dev_pm.h"
#endif

#ifndef CONFIG_DFS_WAKEUP_ALL
#define CONFIG_DFS_WAKEUP_ALL       1
#endif

#ifndef CONFIG_DFS_DEBUG_SFC_BUSY
#define CONFIG_DFS_DEBUG_SFC_BUSY   1
#endif

#define DFS_OBJ_SHARED_FIXED_MASK   ((1<<(DFS_OBJ_SHARED_FIXED_END - DFS_OBJ_SHARED_FIXED_START)) - 1)


#define DFS_OBJ_SHARED_FIXED_NUM    (DFS_OBJ_SHARED_FIXED_END - DFS_OBJ_SHARED_FIXED_START)
#define DFS_OBJ_SHARED_ATLEAST_NUM  (DFS_OBJ_SHARED_ATLEAST_END - DFS_OBJ_SHARED_ATLEAST_START)
#define DFS_OBJ_DSP_NUM             (DFS_OBJ_DSP_END - DFS_OBJ_DSP_START)
#define DFS_OBJ_BT_NUM              (DFS_OBJ_BT_END - DFS_OBJ_BT_START)
#define DFS_OBJ_DTOP_NUM            (DFS_OBJ_DTOP_END - DFS_OBJ_DTOP_START)

#define DFS_OBJ_GRP_SHARED_FIXED_OFFSET(obj)    ((obj) - DFS_OBJ_SHARED_FIXED_START)
#define DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(obj)  ((obj) - DFS_OBJ_SHARED_ATLEAST_START)
#define DFS_OBJ_GRP_DSP_OFFSET(obj)             ((obj) - DFS_OBJ_DSP_START)
#define DFS_OBJ_GRP_BT_OFFSET(obj)              ((obj) - DFS_OBJ_BT_START)
#define DFS_OBJ_GRP_DTOP_OFFSET(obj)            ((obj) - DFS_OBJ_DTOP_START)

#define DFS_TIMEOUT_MS_MIN          5UL
#define DFS_TIMEOUT_MS_MARGIN       5UL

#define DFS_PMM_PD_ST_ON            0x4

typedef enum {
    DFS_KV_ID_CFG = LIB_DFS_BASE_ID,
} DFS_KV_ID;

typedef struct {
    uint8_t music_32m_disable;
    uint8_t reserved[3];
} dfs_kv_cfg_t;

typedef enum {
    DFS_SHARE_TASK_MSG_CMD_DFS_REQ = 0,
    DFS_SHARE_TASK_MSG_CMD_PM_RESTORE,
    DFS_SHARE_TASK_MSG_CMD_NUM,
} DFS_SHARE_TASK_MSG_CMD;

typedef struct {
    DFS_SHARE_TASK_MSG_CMD cmd;
    union {
        /* corresponding to DFS_SHARE_TASK_MSG_CMD_DFS_REQ */
        struct {
            uint8_t src_core;
            uint16_t obj;
            uint8_t act;
            union {
                struct {
                    uint32_t timeout_ms;
                } req;
                uint32_t val;
            } op_param_u;
        } dfs_req;
        /* DFS_SHARE_TASK_MSG_CMD_PM_RESTORE has no parameters, no param definition here */
    } param_u;
} dfs_share_task_msg_t;

typedef enum {
    DFS_OBJ_GRP_SHARED_FIXED_IDX = 0,
    DFS_OBJ_GRP_SHARED_ATLEAST_IDX,
    DFS_OBJ_GRP_DSP_IDX,
    DFS_OBJ_GRP_BT_IDX,
    DFS_OBJ_GRP_DTOP_IDX,
    DFS_OBJ_GRP_NUM,
} dfs_obj_grp_idx_t;

typedef struct {
    uint32_t obj_mask;
    const IOT_CLOCK_MODE *tab_p;
    uint32_t tab_len;
} dfs_obj_env_t;

typedef struct {
    struct list_head node;
    uint8_t obj;
    uint32_t end_ts;
} dfs_timeout_item_t;

typedef struct {
    os_mutex_h mutex;
    os_sem_h ipc_ack_sem_dsp;
    int32_t share_task_msg_id;
} dfs_persistent_env_t;

typedef enum {
    DFS_ST_CHAOS = 0,
    DFS_ST_NOT_READY,
    DFS_ST_READY,
    DFS_ST_NUM,
} DFS_ST;

typedef struct {
    DFS_ST  status;
    uint8_t curr_mode;
    uint8_t pending_mode;
    dfs_obj_env_t obj_env[DFS_OBJ_GRP_NUM];
#if CONFIG_DFS_DEBUG_SFC_BUSY
    uint32_t sfc_busy_count;
    uint32_t mode_miss_count;
#endif
    dfs_hook_t hooks[DFS_HOOK_TYPE_NUM];
#ifdef LOW_POWER_ENABLE
    struct pm_operation dfs_pm;
    dfs_share_task_msg_t pm_restore_msg;
#endif
    struct list_head timeout_list;
    timer_id_t timeout_timer;
    uint8_t timeout_obj;
} dfs_env_t;

static const IOT_CLOCK_MODE s_dfs_shared_fixed_table[DFS_OBJ_SHARED_FIXED_NUM] = {

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_16M)]       = IOT_CLOCK_MODE_0,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_16M)]       = IOT_CLOCK_MODE_1,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_48_48_16M)]       = IOT_CLOCK_MODE_2,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_64_64_16M)]       = IOT_CLOCK_MODE_3,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_80_80_16M)]       = IOT_CLOCK_MODE_4,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_32M)]       = IOT_CLOCK_MODE_5,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_32M)]       = IOT_CLOCK_MODE_6,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_64_64_32M)]       = IOT_CLOCK_MODE_7,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_48M)]       = IOT_CLOCK_MODE_8,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_48_48_48M)]       = IOT_CLOCK_MODE_9,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_64M)]       = IOT_CLOCK_MODE_10,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_64M)]       = IOT_CLOCK_MODE_11,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_64_64_64M)]       = IOT_CLOCK_MODE_12,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_80M)]       = IOT_CLOCK_MODE_13,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_80_80_80M)]       = IOT_CLOCK_MODE_14,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_96M)]       = IOT_CLOCK_MODE_15,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_96M)]       = IOT_CLOCK_MODE_16,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_48_48_96M)]       = IOT_CLOCK_MODE_17,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_128M)]      = IOT_CLOCK_MODE_18,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_128M)]      = IOT_CLOCK_MODE_19,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_64_64_128M)]      = IOT_CLOCK_MODE_20,

    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_16_16_160M)]      = IOT_CLOCK_MODE_21,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_32_32_160M)]      = IOT_CLOCK_MODE_22,
    [DFS_OBJ_GRP_SHARED_FIXED_OFFSET(DFS_OBJ_SHARED_FIXED_80_80_160M)]      = IOT_CLOCK_MODE_23,
};

static const IOT_CLOCK_MODE s_dfs_shared_atleast_table[DFS_OBJ_SHARED_ATLEAST_NUM] = {

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_16M)]   = IOT_CLOCK_MODE_1,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_48_48_16M)]   = IOT_CLOCK_MODE_2,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_64_64_16M)]   = IOT_CLOCK_MODE_3,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_80_80_16M)]   = IOT_CLOCK_MODE_4,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_32M)]   = IOT_CLOCK_MODE_5,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_32M)]   = IOT_CLOCK_MODE_6,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_64_64_32M)]   = IOT_CLOCK_MODE_7,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_48M)]   = IOT_CLOCK_MODE_8,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_48_48_48M)]   = IOT_CLOCK_MODE_9,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_64M)]   = IOT_CLOCK_MODE_10,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_64M)]   = IOT_CLOCK_MODE_11,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_64_64_64M)]   = IOT_CLOCK_MODE_12,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_80M)]   = IOT_CLOCK_MODE_13,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_80_80_80M)]   = IOT_CLOCK_MODE_14,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_96M)]   = IOT_CLOCK_MODE_15,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_96M)]   = IOT_CLOCK_MODE_16,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_48_48_96M)]   = IOT_CLOCK_MODE_17,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_128M)]  = IOT_CLOCK_MODE_18,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_128M)]  = IOT_CLOCK_MODE_19,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_64_64_128M)]  = IOT_CLOCK_MODE_20,

    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_16_16_160M)]  = IOT_CLOCK_MODE_21,
    [DFS_OBJ_GRP_SHARED_ATLEAST_OFFSET(DFS_OBJ_SHARED_ATLEAST_32_32_160M)]  = IOT_CLOCK_MODE_22,
};

/* =====================  DFS table for enable 32M music (default) start ====================== */
static const IOT_CLOCK_MODE s_dfs_dsp_table[DFS_OBJ_DSP_NUM] = {
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_MIC_STREAM)]                        = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_MUSIC_STREAM)]                      = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_VOICE_STREAM)]                      = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_TONE_STREAM)]                       = IOT_CLOCK_MODE_6,
};

static const IOT_CLOCK_MODE s_dfs_bt_table[DFS_OBJ_BT_NUM] = {
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_TDS)]                                 = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_SCO_LINK)]                            = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_MUSIC_STREAM)]                        = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_BLE_CON)]                             = IOT_CLOCK_MODE_6,
};

static const IOT_CLOCK_MODE s_dfs_dtop_table[DFS_OBJ_DTOP_NUM] = {
    /* CLOCK_CORE_64M, CLOCK_CORE_64M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_OTA)]                             = IOT_CLOCK_MODE_12,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_TONE_STREAM)]                     = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_ANC)]                             = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_VAD)]                             = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_PLAYER_SPK)]                      = IOT_CLOCK_MODE_6,
};
/* =====================  DFS table for enable 32M music (default) end ====================== */

/* =======================  DFS table for disable 32M music start =========================== */
static const IOT_CLOCK_MODE s_dfs_dsp_table_music_32m_disable[DFS_OBJ_DSP_NUM] = {
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_MIC_STREAM)]                        = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_16M, CLOCK_CORE_16M, CLOCK_CORE_16M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_MUSIC_STREAM)]                      = IOT_CLOCK_MODE_0,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_VOICE_STREAM)]                      = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DSP_OFFSET(DFS_OBJ_DSP_TONE_STREAM)]                       = IOT_CLOCK_MODE_6,
};

static const IOT_CLOCK_MODE s_dfs_bt_table_music_32m_disable[DFS_OBJ_BT_NUM] = {
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_TDS)]                                 = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_SCO_LINK)]                            = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_16M, CLOCK_CORE_16M, CLOCK_CORE_16M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_MUSIC_STREAM)]                        = IOT_CLOCK_MODE_0,
    /* CLOCK_CORE_16M, CLOCK_CORE_16M, CLOCK_CORE_16M */
    [DFS_OBJ_GRP_BT_OFFSET(DFS_OBJ_BT_BLE_CON)]                             = IOT_CLOCK_MODE_0,
};

static const IOT_CLOCK_MODE s_dfs_dtop_table_music_32m_disable[DFS_OBJ_DTOP_NUM] = {
    /* CLOCK_CORE_64M, CLOCK_CORE_64M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_OTA)]                             = IOT_CLOCK_MODE_12,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_TONE_STREAM)]                     = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_32M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_ANC)]                             = IOT_CLOCK_MODE_6,
    /* CLOCK_CORE_32M, CLOCK_CORE_32M, CLOCK_CORE_64M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_VAD)]                             = IOT_CLOCK_MODE_11,
    /* CLOCK_CORE_16M, CLOCK_CORE_16M, CLOCK_CORE_16M */
    [DFS_OBJ_GRP_DTOP_OFFSET(DFS_OBJ_DTOP_PLAYER_SPK)]                      = IOT_CLOCK_MODE_0,
};
/* =======================  DFS table for disable 32M music end =========================== */

static dfs_persistent_env_t s_dfs_persist_env = {
    .mutex = NULL,
    .ipc_ack_sem_dsp = NULL,
    .share_task_msg_id = 0,
};

static dfs_env_t s_dfs_env = {
    .status = DFS_ST_CHAOS,
    .curr_mode = IOT_CLOCK_MODE_0,
    .pending_mode = IOT_CLOCK_MODE_MAX,
    .obj_env = {
        [DFS_OBJ_GRP_SHARED_FIXED_IDX] = {
            .obj_mask = 0x0,
            .tab_p = &s_dfs_shared_fixed_table[0],
            .tab_len = DFS_OBJ_SHARED_FIXED_NUM,
        },
        [DFS_OBJ_GRP_SHARED_ATLEAST_IDX] = {
            .obj_mask = 0x0,
            .tab_p = &s_dfs_shared_atleast_table[0],
            .tab_len = DFS_OBJ_SHARED_ATLEAST_NUM,
        },
        [DFS_OBJ_GRP_DSP_IDX] = {
            .obj_mask = 0x0,
            .tab_p = &s_dfs_dsp_table[0],
            .tab_len = DFS_OBJ_DSP_NUM,
        },
        [DFS_OBJ_GRP_BT_IDX] = {
            .obj_mask = 0x0,
            .tab_p = &s_dfs_bt_table[0],
            .tab_len = DFS_OBJ_BT_NUM,
        },
        [DFS_OBJ_GRP_DTOP_IDX] = {
            .obj_mask = 0x0,
            .tab_p = &s_dfs_dtop_table[0],
            .tab_len = DFS_OBJ_DTOP_NUM,
        },
    },
#if CONFIG_DFS_DEBUG_SFC_BUSY
    .sfc_busy_count = 0,
    .mode_miss_count = 0,
#endif
    .hooks = {NULL},
    .timeout_obj = DFS_OBJ_MAX,
};

static inline bool dfs_is_shared_fixed_obj(DFS_OBJ obj)
{
    return (obj >= DFS_OBJ_SHARED_FIXED_START && obj < DFS_OBJ_SHARED_FIXED_END);
}

static bool dfs_obj_grp_info_get(DFS_OBJ obj,
                                    uint8_t *obj_grp_idx_p, uint8_t *obj_grp_start_p)
{
    assert(obj_grp_idx_p);
    assert(obj_grp_start_p);

    if (obj >= DFS_OBJ_SHARED_FIXED_START && obj < DFS_OBJ_SHARED_FIXED_END) {
        *obj_grp_idx_p = DFS_OBJ_GRP_SHARED_FIXED_IDX;
        *obj_grp_start_p = DFS_OBJ_SHARED_FIXED_START;
    } else if (obj >= DFS_OBJ_SHARED_ATLEAST_START && obj < DFS_OBJ_SHARED_ATLEAST_END) {
        *obj_grp_idx_p = DFS_OBJ_GRP_SHARED_ATLEAST_IDX;
        *obj_grp_start_p = DFS_OBJ_SHARED_ATLEAST_START;
    } else if (obj >= DFS_OBJ_DSP_START && obj < DFS_OBJ_DSP_END) {
        *obj_grp_idx_p = DFS_OBJ_GRP_DSP_IDX;
        *obj_grp_start_p = DFS_OBJ_DSP_START;
    } else if (obj >= DFS_OBJ_BT_START && obj < DFS_OBJ_BT_END) {
        *obj_grp_idx_p = DFS_OBJ_GRP_BT_IDX;
        *obj_grp_start_p = DFS_OBJ_BT_START;
    } else if (obj >= DFS_OBJ_DTOP_START && obj < DFS_OBJ_DTOP_END) {
        *obj_grp_idx_p = DFS_OBJ_GRP_DTOP_IDX;
        *obj_grp_start_p = DFS_OBJ_DTOP_START;
    } else {
        assert(0);
        return false;
    }

    return true;
}

static bool dfs_obj_mask_check_and_set(DFS_OBJ obj)
{
    uint8_t obj_grp_idx = 0;
    uint8_t obj_grp_start = 0;

    if (dfs_obj_grp_info_get(obj, &obj_grp_idx, &obj_grp_start) == false) {
        return false;
    }

    if ((s_dfs_env.obj_env[obj_grp_idx].obj_mask & (1<<(obj - obj_grp_start))) != 0) {
        return false;
    }

    s_dfs_env.obj_env[obj_grp_idx].obj_mask |= (1<<(obj - obj_grp_start));

    return true;
}

static bool dfs_obj_mask_check_and_clear(DFS_OBJ obj)
{
    uint8_t obj_grp_idx;
    uint8_t obj_grp_start;

    if (dfs_obj_grp_info_get(obj, &obj_grp_idx, &obj_grp_start) == false) {
        return false;
    }

    if ((s_dfs_env.obj_env[obj_grp_idx].obj_mask & (1<<(obj - obj_grp_start))) == 0) {
        return false;
    }

    s_dfs_env.obj_env[obj_grp_idx].obj_mask &= ~(1<<(obj - obj_grp_start));

    return true;
}

static void dfs_pm_lock_ipc_ack_cb(IPC_CORES src_cpu) IRAM_TEXT(dfs_pm_lock_ipc_ack_cb);
static void dfs_pm_lock_ipc_ack_cb(IPC_CORES src_cpu)
{
    if (src_cpu == AUD_CORE) {
        os_post_semaphore_from_isr(s_dfs_persist_env.ipc_ack_sem_dsp);
    } else {
        assert(0);
    }
}

static void dfs_set_clock_mode(IOT_CLOCK_MODE src_mode, IOT_CLOCK_MODE dst_mode, uint32_t wakeup_core_mask)
{
    uint8_t dummy_data;
    dfs_hook_t pre_hook = s_dfs_env.hooks[DFS_HOOK_TYPE_PRE];
    dfs_hook_t post_hook = s_dfs_env.hooks[DFS_HOOK_TYPE_POST];

    if (wakeup_core_mask & (1<<BT_CORE)) {
        iot_suspend_sched_wait_core_suspend(BT_CORE);
    }

    if (wakeup_core_mask & (1<<AUD_CORE)) {
        while (iot_ipc_send_message(AUD_CORE, IPC_TYPE_PM_LOCK,
                                    &dummy_data, sizeof(dummy_data), true) != RET_OK) {
            /* nothing, just try until ipc send ok */
        }
        os_pend_semaphore(s_dfs_persist_env.ipc_ack_sem_dsp, 0xFFFFFFFF);
    }

    if (pre_hook != NULL) {
        pre_hook(src_mode, dst_mode);
    }

    iot_clock_set_mode(dst_mode);

    if (post_hook != NULL) {
        post_hook(src_mode, dst_mode);
    }

    if (wakeup_core_mask & (1<<BT_CORE)) {
        iot_suspend_sched_notify_core_resume(BT_CORE);
    }

    if (wakeup_core_mask & (1<<AUD_CORE)) {
        iot_ipc_finsh_keep_wakeup(AUD_CORE);
    }
}

static void dfs_pending_list_handler(void)
{
    uint8_t mode;

    os_acquire_mutex(s_dfs_persist_env.mutex);

    if (s_dfs_env.status != DFS_ST_READY) {
        os_release_mutex(s_dfs_persist_env.mutex);
        return;
    }

    mode = s_dfs_env.pending_mode;

    iot_flash_unregister_pe_callback();

    if (s_dfs_env.pending_mode != IOT_CLOCK_MODE_MAX) {
        dfs_set_clock_mode(s_dfs_env.curr_mode, s_dfs_env.pending_mode,
                           ((1<<BT_CORE)|(1<<AUD_CORE)));

        s_dfs_env.curr_mode = s_dfs_env.pending_mode;
        s_dfs_env.pending_mode = IOT_CLOCK_MODE_MAX;
    }

    os_release_mutex(s_dfs_persist_env.mutex);

    DBGLOG_LIB_INFO("[DFS] pending update clock to mode %d!!\n", mode);
}

static inline dfs_timeout_item_t *dfs_timeout_first_item(void)
{
    if (list_empty(&s_dfs_env.timeout_list)) {
        return NULL;
    }

    return list_entry(s_dfs_env.timeout_list.next, dfs_timeout_item_t, node);
}

static void dfs_timeout_restart_timer(bool force)
{
    uint32_t period;

    cpu_critical_enter();

    if (list_empty(&s_dfs_env.timeout_list)) {
        s_dfs_env.timeout_obj = DFS_OBJ_MAX;
        goto _stop_timer;
    } else {
        dfs_timeout_item_t *first_item = dfs_timeout_first_item();

        if (force || s_dfs_env.timeout_obj != first_item->obj) {
            s_dfs_env.timeout_obj = first_item->obj;

            /* always make timer trigger to let it process in timer handler */
            period = first_item->end_ts - iot_rtc_get_global_time_ms();
            if (period < DFS_TIMEOUT_MS_MIN || period > DFS_TIMEOUT_MS_MAX) {
                period = DFS_TIMEOUT_MS_MIN;
            }

            goto _restart_timer;
        } else {
            //do nothing
        }
    }

    cpu_critical_exit();

    return;

_stop_timer:
    cpu_critical_exit();

    os_stop_timer(s_dfs_env.timeout_timer);

    return;

_restart_timer:
    cpu_critical_exit();

    os_stop_timer(s_dfs_env.timeout_timer);
    os_start_timer(s_dfs_env.timeout_timer, period);

    return;
}

static void dfs_timeout_handler(timer_id_t timer_id, void * arg)
{
    dfs_timeout_item_t *first_item = dfs_timeout_first_item();
    uint8_t obj = DFS_OBJ_MAX;

    UNUSED(timer_id);
    UNUSED(arg);

    if (s_dfs_env.status != DFS_ST_READY) {
        return;
    }

    cpu_critical_enter();

    /* current node is valid and the end ts is past, then remove it */
    if (s_dfs_env.timeout_obj < DFS_OBJ_MAX && first_item != NULL) {

        /* do not remove the check, because if the dfs_timeout_handler is already happen, and
         * preempt current task when it do dfs_timeout_add/modifiy.
         */
        if (s_dfs_env.timeout_obj == first_item->obj &&
                (first_item->end_ts - iot_rtc_get_global_time_ms() <= DFS_TIMEOUT_MS_MARGIN ||
                 iot_rtc_get_global_time_ms() - first_item->end_ts <= DFS_TIMEOUT_MS_MAX)) {
            obj = s_dfs_env.timeout_obj;
            s_dfs_env.timeout_obj = DFS_OBJ_MAX;
        }
    }

    cpu_critical_exit();

    if (obj < DFS_OBJ_MAX) {
        DBGLOG_LIB_INFO("[DFS] TO stop obj %d !!\n", obj);

        while (dfs_request_impl(cpu_get_mhartid(), obj, DFS_ACT_STOP, 0) != RET_OK);
    } else {
        /* Do nothing. As desgin, do not restart timer in dfs_timeout_handler.
         * Enter here means the timeout handler is suddenly preempt before restart timer
         * in dfs_timeout_add, then dfs_timeout_add will restart timer.
         */
        DBGLOG_LIB_INFO("[DFS] TO ignore obj %d/%d ts %d/%d !!\n",
                        s_dfs_env.timeout_obj, (first_item ? first_item->obj : DFS_OBJ_MAX),
                        iot_rtc_get_global_time_ms(), (first_item ? first_item->end_ts : 0));
    }
}

static void dfs_timeout_add(uint32_t obj, uint32_t timeout)
{
    struct list_head *saved;
    dfs_timeout_item_t *new_item;
    dfs_timeout_item_t *item;
    bool found = false;

    if (timeout == 0) {
        return;
    }

    new_item = (dfs_timeout_item_t *)os_mem_malloc(UNKNOWN_MID, sizeof(dfs_timeout_item_t));
    assert(new_item != NULL);

    new_item->obj = obj;
    new_item->end_ts = (uint32_t)(iot_rtc_get_global_time_ms() + timeout);

    cpu_critical_enter();

    list_for_each_entry_safe(item, &s_dfs_env.timeout_list, node, saved) {
        //look for the item which end ts is just after new item end ts
        if (item->end_ts - new_item->end_ts <= DFS_TIMEOUT_MS_MAX) {
            found = true;
            //insert before current head
            list_add_tail(&new_item->node, &item->node);
            break;
        }
    }

    if (!found) {
        //append to list tail
        list_add_tail(&new_item->node, &s_dfs_env.timeout_list);
    }

    cpu_critical_exit();

    dfs_timeout_restart_timer(false);
}

static void dfs_timeout_del(uint32_t obj)
{
    struct list_head *saved;
    dfs_timeout_item_t *item;
    dfs_timeout_item_t *free_item = NULL;

    cpu_critical_enter();

    list_for_each_entry_safe(item, &s_dfs_env.timeout_list, node, saved) {
        if (item->obj == obj) {
            list_del(&item->node);
            free_item = item;

            if (s_dfs_env.timeout_obj == obj) {
                s_dfs_env.timeout_obj = DFS_OBJ_MAX;
            }

            break;
        }
    }

    cpu_critical_exit();

    if (free_item != NULL) {
        os_mem_free(free_item);
    }

    dfs_timeout_restart_timer(false);
}

static void dfs_timeout_modify(uint32_t obj, uint32_t timeout)
{
    struct list_head *saved;
    dfs_timeout_item_t *item;
    dfs_timeout_item_t *free_item = NULL;
    uint32_t new_ts;

    /* always use the longest ts end, timeout 0 is treat forever as large enough.
     * 1. previous end_ts is before new ts ---> new
     * 2. previous end_ts is after new ts --->  previous
     */

    new_ts = (uint32_t)(iot_rtc_get_global_time_ms() + timeout);

    cpu_critical_enter();

    list_for_each_entry_safe(item, &s_dfs_env.timeout_list, node, saved) {
        if (item->obj == obj) {
            if (timeout == 0) {
                list_del(&item->node);
                free_item = item;
            } else if (new_ts - item->end_ts <= DFS_TIMEOUT_MS_MAX) {
                item->end_ts = new_ts;
            } else {
                //do nothing
            }
            break;
        }
    }

    cpu_critical_exit();

    if (free_item) {
        os_mem_free(item);
    }

    dfs_timeout_restart_timer(true);
}

static void dfs_timeout_clear_all(void)
{
    struct list_head tmp_list;
    struct list_head *saved;
    dfs_timeout_item_t *item;

    os_stop_timer(s_dfs_env.timeout_timer);

    list_init(&tmp_list);

    /* critical protect for list walk, then mem free from tmp list */
    cpu_critical_enter();
    list_for_each_entry_safe(item, &s_dfs_env.timeout_list, node, saved) {
        list_del(&item->node);
        list_add_tail(&item->node, &tmp_list);
    }
    cpu_critical_exit();

    list_for_each_entry_safe(item, &tmp_list, node, saved) {
        list_del(&item->node);
        os_mem_free(item);
    }
}

static uint32_t dfs_request_process(uint32_t obj,
                                    uint32_t act,
                                    uint32_t param,
                                    uint32_t wakeup_core_mask)
{
    IOT_CLOCK_MODE new_mode;
    IOT_CLOCK_CORE new_dtop_clk = IOT_CLOCK_CORE_16M;
    IOT_CLOCK_CORE new_bt_clk = IOT_CLOCK_CORE_16M;
    IOT_CLOCK_CORE new_dsp_clk = IOT_CLOCK_CORE_16M;
    uint8_t obj_grp_idx_start;
    uint8_t obj_grp_idx_num;

    if (obj >= DFS_OBJ_MAX) {
        return RET_INVAL;
    }

    if (act >= DFS_ACT_NUM) {
        return RET_INVAL;
    }

    os_acquire_mutex(s_dfs_persist_env.mutex);

    if (s_dfs_env.status != DFS_ST_READY) {
        os_release_mutex(s_dfs_persist_env.mutex);
        return RET_NOT_READY;
    }

    if (act == DFS_ACT_START) {
        if (dfs_obj_mask_check_and_set(obj) == false) {
            dfs_timeout_modify(obj, param);
            os_release_mutex(s_dfs_persist_env.mutex);
            return RET_EXIST;
        } else {
            dfs_timeout_add(obj, param);
        }
    } else {
        if (dfs_obj_mask_check_and_clear(obj) == false) {
            os_release_mutex(s_dfs_persist_env.mutex);
            return RET_EXIST;
        } else {
            dfs_timeout_del(obj);
        }
    }

    if (dfs_is_shared_fixed_obj(obj) &&
            (s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask &
                DFS_OBJ_SHARED_FIXED_MASK) != 0) {
        obj_grp_idx_start = DFS_OBJ_GRP_SHARED_FIXED_IDX;
        obj_grp_idx_num = 1;
    } else {
        if ((s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask &
                DFS_OBJ_SHARED_FIXED_MASK) != 0) {
            os_release_mutex(s_dfs_persist_env.mutex);
            return RET_PENDING;
        } else {
            obj_grp_idx_start = DFS_OBJ_GRP_SHARED_ATLEAST_IDX;
            obj_grp_idx_num = DFS_OBJ_GRP_NUM;
        }
    }

    for (uint8_t obj_grp_idx = obj_grp_idx_start; obj_grp_idx < obj_grp_idx_num; obj_grp_idx++) {

        if (s_dfs_env.obj_env[obj_grp_idx].obj_mask == 0) {
            continue;
        }

        for (uint8_t idx = 0; idx < s_dfs_env.obj_env[obj_grp_idx].tab_len; idx++) {
            if (s_dfs_env.obj_env[obj_grp_idx].obj_mask & (1<<idx)) {
                const IOT_CLOCK_MODE *dfs_tab_p = s_dfs_env.obj_env[obj_grp_idx].tab_p;

                IOT_CLOCK_CORE dtop_clk = iot_clock_get_core_clock_by_mode(dfs_tab_p[idx], 0);
                IOT_CLOCK_CORE bt_clk = iot_clock_get_core_clock_by_mode(dfs_tab_p[idx], 1);
                IOT_CLOCK_CORE dsp_clk = iot_clock_get_core_clock_by_mode(dfs_tab_p[idx], 2);

                new_dtop_clk = (new_dtop_clk > dtop_clk ? new_dtop_clk : dtop_clk);
                new_bt_clk = (new_bt_clk > bt_clk ? new_bt_clk : bt_clk);
                new_dsp_clk = (new_dsp_clk > dsp_clk ? new_dsp_clk : dsp_clk);
            }
        }
    }

    assert(new_dtop_clk == new_bt_clk);

    new_mode = iot_clock_get_mode_by_core_clock(new_dtop_clk,
                                            new_bt_clk,
                                            new_dsp_clk);

    if (new_mode >= IOT_CLOCK_MODE_MAX) {
        os_release_mutex(s_dfs_persist_env.mutex);

        DBGLOG_LIB_ERROR("[auto][DFS] update clock fail, obj %u, act %u, mask %x %x %x %x %x!!\n",
                         obj, act,
                         s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask,
                         s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_ATLEAST_IDX].obj_mask,
                         s_dfs_env.obj_env[DFS_OBJ_GRP_DSP_IDX].obj_mask,
                         s_dfs_env.obj_env[DFS_OBJ_GRP_BT_IDX].obj_mask,
                         s_dfs_env.obj_env[DFS_OBJ_GRP_DTOP_IDX].obj_mask);
        return RET_FAIL;
    }

    if (new_mode != s_dfs_env.curr_mode) {
#if CONFIG_DFS_DEBUG_SFC_BUSY
        if (s_dfs_env.pending_mode != IOT_CLOCK_MODE_MAX) {
            s_dfs_env.mode_miss_count++;
        }
#endif
        /* 1. whatever the pending_mode is, always use the new mode overwrite it.
         *    This is dut to that the middle value is no meaningful when pe callback
         *    is calling.
         * 2. It's more safe that set pending mode before pe check and clear it if pe check fail.
         */
        s_dfs_env.pending_mode = new_mode;
        iot_flash_register_pe_callback(dfs_pending_list_handler);

        if (iot_flash_is_pe_in_progress()) {
            os_release_mutex(s_dfs_persist_env.mutex);
#if CONFIG_DFS_DEBUG_SFC_BUSY
            s_dfs_env.sfc_busy_count++;
            DBGLOG_LIB_INFO("[DFS] update clock in flash busy c %d/%d, obj %u, act %u, mask %x %x %x %x %x!!\n",
                            s_dfs_env.sfc_busy_count, s_dfs_env.mode_miss_count,
                            obj, act,
                            s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask,
                            s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_ATLEAST_IDX].obj_mask,
                            s_dfs_env.obj_env[DFS_OBJ_GRP_DSP_IDX].obj_mask,
                            s_dfs_env.obj_env[DFS_OBJ_GRP_BT_IDX].obj_mask,
                            s_dfs_env.obj_env[DFS_OBJ_GRP_DTOP_IDX].obj_mask);
#endif
            return RET_BUSY;
        }

        iot_flash_unregister_pe_callback();
        s_dfs_env.pending_mode = IOT_CLOCK_MODE_MAX;

        dfs_set_clock_mode(s_dfs_env.curr_mode, new_mode, wakeup_core_mask);

        s_dfs_env.curr_mode = new_mode;

        DBGLOG_LIB_INFO("[auto][DFS] update clock to mode %d, obj %u, act %u, mask %x %x %x %x %x!!\n",
                        new_mode, obj, act,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_ATLEAST_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_DSP_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_BT_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_DTOP_IDX].obj_mask);
    } else {
        DBGLOG_LIB_INFO("[auto][DFS] update clock ignore %d, obj %u, act %u, mask %x %x %x %x %x!!\n",
                        new_mode, obj, act,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_FIXED_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_SHARED_ATLEAST_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_DSP_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_BT_IDX].obj_mask,
                        s_dfs_env.obj_env[DFS_OBJ_GRP_DTOP_IDX].obj_mask);
    }

    os_release_mutex(s_dfs_persist_env.mutex);

    return RET_OK;
}

static uint32_t dfs_request_in_share_task(uint32_t src_core, uint32_t obj, uint32_t act, uint32_t param)
{
    uint32_t wakeup_core_mask;
#if CONFIG_DFS_WAKEUP_ALL
    UNUSED(src_core);
    wakeup_core_mask = ((1<<BT_CORE) | (1<<AUD_CORE));
#else
    wakeup_core_mask = (src_core == DTOP_CORE ? ((1<<BT_CORE) | (1<<AUD_CORE)) :
            (src_core == BT_CORE ? (1<<AUD_CORE) : (1<<BT_CORE)));
#endif
    return dfs_request_process(obj, act, param, wakeup_core_mask);
}

static void dfs_share_task_handler(void *arg)
{
    dfs_share_task_msg_t *msg = (void *)arg;

    switch (msg->cmd) {
    case DFS_SHARE_TASK_MSG_CMD_DFS_REQ:
        dfs_request_in_share_task(msg->param_u.dfs_req.src_core,
                                    msg->param_u.dfs_req.obj,
                                    msg->param_u.dfs_req.act,
                                    msg->param_u.dfs_req.op_param_u.val);
        os_mem_free(msg);
        break;
#ifdef LOW_POWER_ENABLE
    case DFS_SHARE_TASK_MSG_CMD_PM_RESTORE:
        if (s_dfs_env.curr_mode != IOT_CLOCK_MODE_0) {
            uint32_t wakeup_core_mask = ((1<<BT_CORE) | (1<<AUD_CORE));

            os_acquire_mutex(s_dfs_persist_env.mutex);

            if (s_dfs_env.status != DFS_ST_READY) {
                os_release_mutex(s_dfs_persist_env.mutex);
                return;
            }

            dfs_set_clock_mode(IOT_CLOCK_MODE_0, s_dfs_env.curr_mode, wakeup_core_mask);
            os_release_mutex(s_dfs_persist_env.mutex);
        }

        break;
#endif
    default:
        assert(0);
    }
}

uint32_t dfs_request_impl(uint32_t src_core, uint32_t obj, uint32_t act, uint32_t param)
{
    bool ret;
    dfs_share_task_msg_t *msg;

    if (act == DFS_ACT_START && param > DFS_TIMEOUT_MS_MAX) {
        return RET_INVAL;
    }

    msg = (dfs_share_task_msg_t *)os_mem_malloc(UNKNOWN_MID, sizeof(dfs_share_task_msg_t));

    if (msg == NULL) {
        return RET_NOMEM;
    }

    msg->cmd = DFS_SHARE_TASK_MSG_CMD_DFS_REQ;
    msg->param_u.dfs_req.src_core = (uint8_t)src_core;
    msg->param_u.dfs_req.obj = (uint16_t)obj;
    msg->param_u.dfs_req.act = (uint8_t)act;
    msg->param_u.dfs_req.op_param_u.val = param;

    ret = iot_share_task_post_msg(IOT_SHARE_TASK_QUEUE_HP, s_dfs_persist_env.share_task_msg_id, msg);

    if (!ret) {
        os_mem_free(msg);
    }

    return (ret ? RET_OK : RET_FAIL);
}

uint8_t dfs_hook_register(DFS_HOOK_TYPE type, dfs_hook_t hook)
{
    if (type >= DFS_HOOK_TYPE_NUM) {
        return RET_INVAL;
    }

    s_dfs_env.hooks[type] = hook;

    return RET_OK;
}

#ifdef LOW_POWER_ENABLE
static uint32_t dfs_pm_restore(uint32_t data)
{
    bool ret;
    UNUSED(data);

    iot_clock_reset_state();

    s_dfs_env.pm_restore_msg.cmd = DFS_SHARE_TASK_MSG_CMD_PM_RESTORE;

    ret = iot_share_task_post_msg_from_isr(IOT_SHARE_TASK_QUEUE_HP,
                                           s_dfs_persist_env.share_task_msg_id,
                                           &s_dfs_env.pm_restore_msg);

    return (ret ? RET_OK : RET_FAIL);
}
#endif

static void dfs_load_config(void)
{
    uint32_t ret;
    dfs_kv_cfg_t kv_cfg;
    uint32_t kv_cfg_len = sizeof(dfs_kv_cfg_t);

    ret = storage_read(LIB_DFS_BASE_ID, DFS_KV_ID_CFG,
                 (void *)&kv_cfg,  &kv_cfg_len);

    if (ret != RET_OK || kv_cfg_len != sizeof(dfs_kv_cfg_t)) {
        DBGLOG_LIB_ERROR("[DFS] Read KV err: %d, len %d\n", ret, kv_cfg_len);
        return;
    }

    DBGLOG_LIB_INFO("[DFS] Load config, MUSIC_32M_DISABLE: val 0x%x\n", kv_cfg.music_32m_disable);

    if (kv_cfg.music_32m_disable) {
        s_dfs_env.obj_env[DFS_OBJ_GRP_DSP_IDX].tab_p = &s_dfs_dsp_table_music_32m_disable[0];
        s_dfs_env.obj_env[DFS_OBJ_GRP_BT_IDX].tab_p = &s_dfs_bt_table_music_32m_disable[0];
        s_dfs_env.obj_env[DFS_OBJ_GRP_DTOP_IDX].tab_p = &s_dfs_dtop_table_music_32m_disable[0];
    } else {
        //do nothing, use music 32M enable as default
    }
}

/* persistent env init, never deinit nor free */
static void dfs_pers_init(void)
{
    if (s_dfs_env.status == DFS_ST_CHAOS) {
        s_dfs_env.status = DFS_ST_NOT_READY;
        s_dfs_persist_env.mutex = os_create_mutex(UNKNOWN_MID);
        s_dfs_persist_env.ipc_ack_sem_dsp = os_create_semaphore(UNKNOWN_MID, 1, 0);
        s_dfs_persist_env.share_task_msg_id = iot_share_task_msg_register(dfs_share_task_handler);

        assert(s_dfs_persist_env.mutex != NULL);
        assert(s_dfs_persist_env.ipc_ack_sem_dsp != NULL);
        assert(s_dfs_persist_env.share_task_msg_id > 0);
    }
}

uint8_t dfs_init(dfs_init_cfg_t *cfg)
{
    dfs_pers_init();

    os_acquire_mutex(s_dfs_persist_env.mutex);

    assert(s_dfs_env.status != DFS_ST_CHAOS);

    if (s_dfs_env.status != DFS_ST_NOT_READY) {
        os_release_mutex(s_dfs_persist_env.mutex);
        return RET_EXIST;
    }

    dfs_load_config();

    iot_ipc_register_ack_cb(IPC_TYPE_PM_LOCK, dfs_pm_lock_ipc_ack_cb);

    s_dfs_env.timeout_timer = os_create_timer(UNKNOWN_MID, false, dfs_timeout_handler, NULL);
    assert(s_dfs_env.timeout_timer != (timer_id_t)NULL);

    list_init(&s_dfs_env.timeout_list);

#ifdef LOW_POWER_ENABLE
    list_init(&s_dfs_env.dfs_pm.node);
    s_dfs_env.dfs_pm.restore = dfs_pm_restore;
    iot_dev_pm_register(&s_dfs_env.dfs_pm);
#endif

    s_dfs_env.status = DFS_ST_READY;

    os_release_mutex(s_dfs_persist_env.mutex);

    if (cfg != NULL && cfg->init_timeout_ms <= DFS_TIMEOUT_MS_MAX) {
        iot_soc_bt_power_up();
        iot_dsp_power_on();
        dfs_request_process(cfg->init_obj, DFS_ACT_START, cfg->init_timeout_ms, 0x0);
    }

    DBGLOG_LIB_INFO("[DFS] init!\n");

    return RET_OK;
}

void dfs_deinit(void)
{
    assert(s_dfs_persist_env.mutex != NULL);

    os_acquire_mutex(s_dfs_persist_env.mutex);

    if (s_dfs_env.status != DFS_ST_READY) {
        os_release_mutex(s_dfs_persist_env.mutex);
        return;
    }

    s_dfs_env.status = DFS_ST_NOT_READY;

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_unregister(&s_dfs_env.dfs_pm);
#endif
    iot_ipc_unregister_ack_cb(IPC_TYPE_PM_LOCK);

    dfs_timeout_clear_all();
    os_delete_timer(s_dfs_env.timeout_timer);
    s_dfs_env.timeout_timer = (timer_id_t)NULL;

    os_release_mutex(s_dfs_persist_env.mutex);

    DBGLOG_LIB_INFO("[DFS] deinit!\n");
}
