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

#ifndef LIB_POWER_MGNT_H
#define LIB_POWER_MGNT_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#else
#include "stdio.h"
#endif

#ifdef LIB_DBGLOG_ENABLE
#define DBGLOG_LIB_PM_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(LIB_POWER_MGNT_MID, fmt, ##arg)
#define DBGLOG_LIB_PM_ERROR(fmt, arg...)     DBGLOG_STREAM_ERROR(LIB_POWER_MGNT_MID, fmt, ##arg)
#else
#define DBGLOG_LIB_PM_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_LIB_PM_ERROR(fmt, arg...)     printf(fmt, ##arg)
#endif

typedef enum {
#ifdef BUILD_CORE_CORE0
    POWER_SLEEP_UART,
    POWER_SLEEP_CHARGER,
    POWER_SLEEP_AUDIO_INTF,
#elif BUILD_CORE_CORE1
    POWER_SLEEP_BT,
#endif
    /* common */
    POWER_SLEEP_RTC_MON,
    POWER_SLEEP_DMA,
    POWER_SLEEP_IPC,
    POWER_SLEEP_RPC,
    POWER_SLEEP_LED,
    POWER_SLEEP_DEB,
    POWER_SLEEP_VOTE_MAX,
} POWER_SLEEP_VOTE;

typedef enum {
    PM_STATUS_ACTIVE,
    PM_STATUS_SHUTDOWN,
} POWER_MGNT_STATUS;

typedef enum {
    PM_WKS_RTC,
    PM_WKS_GPIO,
} PM_WAKEUP_SRC;

union pm_wakeup_arg {
    struct rtc_arg_struct {
        uint32_t ms;
    } rtc_arg;

    struct gpio_arg_struct {
        uint16_t gpio;
    } gpio_arg;
};

typedef enum {
    SLP_ST_VOTE =       0,
    SLP_ST_SHUTDOWN =   1,
    SLP_ST_ABORT =      2,
    SLP_ST_TIME =       3,
    SLP_ST_AGAIN =      4,
    SLP_ST_BUSY =       5,
    SLP_ST_DEEP =       6,
    SLP_ST_MAX
} slp_st_t;

extern uint8_t power_sleep_vote_count[POWER_SLEEP_VOTE_MAX];

typedef uint8_t (*bt_awake_or_awaking_cb_t)(void);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 * @brief This function is to init power managment.
 * for threshold - when larger then this threshold, the corresponding
 * sleep mode would be enter. the check priority
 * is first deep sleep then light sleep.
 * if 0 is used, then this means always sleep if chance
 *
 * @param deep_sleep_thr is deep sleep thr.
 * @param light_sleep_thr is light sleep thr.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_init(uint32_t deep_sleep_thr, uint32_t light_sleep_thr);

/**
 * @brief Set the threshold of min sleep time.
 *
 * @param deep deep sleep threshold.
 * @param light light sleep threshold.
 */
uint8_t power_mgnt_set_sleep_thr(uint32_t deep, uint32_t light);

/**
 * @brief Set the threshold of min sleep time.
 *
 * @param[out] deep deep sleep threshold pointer.
 * @param[out] light light sleep threshold pointer.
 */
uint8_t power_mgnt_get_sleep_thr(uint32_t *deep, uint32_t *light);

/**
 * @brief This function is to let power managment bt awake cbk register.
 *
 * @param cb is bt awake or awaking callback.
 */
void power_mgnt_bt_awake_cbk_reg(bt_awake_or_awaking_cb_t cb);

/**
 * @brief This function is to set sleep vote.
 *
 * @param vote is power sleep vote.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_set_sleep_vote(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to clear sleep vote.
 *
 * @param vote is power sleep vote.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_clear_sleep_vote(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to get sleep vote.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t power_mgnt_get_sleep_vote(void);

/**
 * @brief This function is to decide module refcnt.
 *
 * @param vote is power sleep vote.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_dec_module_refcnt(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to increase module refcnt.
 *
 * @param vote is power sleep vote.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_inc_module_refcnt(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to get module refcnt.
 *
 * @param vote is power sleep vote.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t power_mgnt_get_module_refcnt(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to clr module refcnt.
 *
 * @param vote is power sleep vote.
 * @return none
 */
void power_mgnt_clr_module_refcnt(POWER_SLEEP_VOTE vote);

/**
 * @brief This function is to enter deep sleep mode.
 *
 * @param expected_idle_time is the expected idle time.
 * @return uint32_t !RET_OK if deep sleep fail, no return if true
 */
uint32_t power_mgnt_deep_sleep(uint32_t expected_idle_time);

/**
 * @brief This function is to enter light bsleep mode.
 *
 * @param expected_idle_time is the expected idle time.
 */
void power_mgnt_light_sleep(uint32_t expected_idle_time);

/**
 * @brief This function is to enter process sleep mode.
 *
 * @param expected_idle_time is the expected idle time.
 */
void power_mgnt_process_sleep(uint32_t expected_idle_time);


/**
 * @brief This function is to dump before assert.
 */
void power_mgnt_failed_dump(void);

/**
 * @brief This function is to get wakeup_ts_us.
 *
 * @return uint32_t is the value of wakeup_ts_us.
 */
uint32_t power_mgnt_get_wakeup_ts_us(void);

/**
 * @brief This function is to get wakeup_ts2_us.
 *
 * @return uint32_t is the value of wakeup_ts2_us.
 */
uint32_t power_mgnt_get_wakeup_ts2_us(void);

/**
 * @brief This function is to get performance parameter.
 *
 * @param hw_deep_ms_in_chip is hardware's deep sleep rtc.
 * @param hw_light_ms_in_chip is hardware's light sleep rtc.
 * @param sw_deep_ms is software's deep sleep rtc.
 * @param sw_light_ms is software's light sleep rtc.
 * @param deep_cnt is deep sleep count.
 * @param light_cnt is light sleep count.
 */
void power_mgnt_perf_get(uint32_t *hw_deep_ms_in_chip, uint32_t *hw_light_ms_in_chip,
                         uint32_t *sw_deep_ms, uint32_t *sw_light_ms, uint32_t *deep_cnt,
                         uint32_t *light_cnt);
/**
 * @brief When this exported function called, it means allowing to shutdown the local domain.
 *
 */
void power_mgnt_shut_down(void);

/**
 * @brief Do not call it directly by user-land.
 *
 */
void power_mgnt_shutdown_process(void);

/**
 * @brief set wakeup source for low-power sleep
 *
 * @param wakeup_src figure out wakeup source
 * @param arg accompany source with argument
 * @return int return status
 */
int power_mgnt_wakeup_src(PM_WAKEUP_SRC wakeup_src, const union pm_wakeup_arg *arg);

/**
 * @brief get power mgnt status
 *
 * @return int active or shutdown in progress
 */
uint32_t power_mgnt_get_status(void);

/**
 * @brief get power mgnt stat for last sleep sussessful or not
 *
 * @param slp_st
 * @param slp_ts_rtc
 * @return int active or shutdown in progress
 */
uint8_t power_mgnt_get_last_slp_st(uint32_t *slp_st[], uint32_t *slp_ts_rtc);

#ifdef __cplusplus
}
#endif

#endif /* LIB_POWER_MGNT_H */
