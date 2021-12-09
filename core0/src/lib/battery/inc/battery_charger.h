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

#ifndef LIB_BATTERY_CHARGER_H
#define LIB_BATTERY_CHARGER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BAT_CHARGER_FULL_VOL_MIN     3734
#define BAT_CHARGER_FULL_VOL_DEFAULT 4192
#define BAT_CHARGER_FULL_VOL_MAX     4700

#define BAT_CAPACITY_MIN     20
#define BAT_CAPACITY_DEFAULT 30
#define BAT_CAPACITY_MAX     50

#define BAT_CHG_TOTAL_TIMEOUT_MIN     5400
#define BAT_CHG_TOTAL_TIMEOUT_DEFAULT 7200
#define BAT_CHG_TOTAL_TIMEOUT_MAX     14400

#define BAT_FULL_TIMEOUT_MIN     5
#define BAT_FULL_TIMEOUT_DEFAULT 420
#define BAT_FULL_TIMEOUT_MAX     600

#define BAT_FAST_CHARGER_CUR_MIN     18
#define BAT_FAST_CHARGER_CUR_DEFAULT 27
#define BAT_FAST_CHARGER_CUR_MAX     45

#define BAT_DP_CHARGER_CUR_MIN     0
#define BAT_DP_CHARGER_CUR_DEFAULT 10
#define BAT_DP_CHARGER_CUR_MAX     15

#define BAT_CUT_OFF_CUR_MIN     0
#define BAT_CUT_OFF_CUR_DEFAULT 10
#define BAT_CUT_OFF_CUR_MAX     15

#define BAT_RESERVED_16_MIN         0
#define BATTERY_RESERVED_16_DEFAULT 0
#define BAT_RESERVED_16_MAX         0xffff

#define BAT_RESERVED_32_MIN         0
#define BATTERY_RESERVED_32_DEFAULT 0
#define BAT_RESERVED_32_MAX         0xffffffff

typedef enum {
    BATTERY_STATE_UNKNOWN,      /*!< something wrong */
    BATTERY_STATE_FULL,         /*!< battery voltage larger than target when charger timeout */
    BATTERY_STATE_BAD,          /*!< battery voltage smaller than target when charger timeout */
    BATTERY_STATE_CHARGE_START, /*!< charge shart */
    BATTERY_STATE_CHARGE_STOP,  /*!< charge stop */
} BATTERY_STATE;

//software charger mode
typedef enum {
    BAT_CHARGER_MODE_STOP,          // software not in charger mode
    BAT_CHARGER_MODE_FAST,          // lager current mode
    BAT_CHARGER_MODE_SET_MONITOR,   // set monitor cur
    BAT_CHARGER_MODE_CV,            // constant voltage mode
    BAT_CHARGER_MODE_FULL,          // battery full contiue charger mode
    BAT_CHARGER_MODE_CMC,           // box msg communicating
    BAT_CHARGER_MODE_MAX,
} BAT_CHARGER_MODE;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint16_t bat_chg_full_vol;        //unit:mV
    uint16_t bat_capacity;            //unit:mAh
    uint16_t bat_chg_total_timeout;   //unit:s
    uint16_t bat_full_timeout;        //unit:s

    uint16_t fast_charger_cur;   //unit:mA
    uint16_t dp_charger_cur;     //unit:mA
    uint16_t cut_off_cur;        //unit:mA
    uint16_t reserved_16;
    uint32_t reserved_32;
} battery_charger_cfg_t;
#pragma pack(pop)

typedef void (*battery_charger_change_cb)(uint8_t flag, uint32_t peried);
typedef void (*battery_state_cb)(BATTERY_STATE state);

/**
 * @brief This function is to start battery charger.
 *
 */
void battery_charger_start(void);

/**
 * @brief This function is to stop battery charger.
 *
 */
void battery_charger_stop(void);

/**
 * @brief This function is to init battery charger.
 *
 */
void battery_charger_init(void);

/**
 * @brief This function is to set current battery charger.
 *
 * @param cur is to set current value.
 */
void battery_charger_set_current(uint16_t cur);

/**
 * @brief Battery charger flag change callback register
 *
 * @param cb callback
 */
void battery_charger_change_register_callback(battery_charger_change_cb cb);

/**
 * @brief Battery charger get flag
 *
 * @return 0 or 1
 */
uint8_t battery_charger_get_flag(void);

/**
 * @brief register callback to notify battery charger info.
 *
 * @param cb callback.
 */
void battery_state_register_callback(battery_state_cb cb);

/**
 * @brief This function is to disable the charge hard reset.
 *
 */
void battery_charger_disable_hard_reset(void);

/**
 * @brief This function is to reatime set charger current.
 *
 * @param cur_ma is to set current value.
 */
void battery_charger_set_cur_limit(uint16_t cur_ma);

/**
 * @brief get charger current limitation
 *
 * @return current limitation
 */
uint16_t battery_charger_get_cur_limit(void);

/**
 * @brief Get the percentage of power charged this time
 *
 * @param base_percent base percent when start charge
 *
 * @return uint8_t percentage of power charged this time
 */
uint8_t battery_charger_get_charged_capacity(uint8_t base_percent);

/**
 * @brief get current charger mode
 *
 * @return current charger mode
 */
BAT_CHARGER_MODE battery_charger_get_mode(void);
#ifdef __cplusplus
}
#endif

#endif /* LIB_BATTERY_CHARGER_H */
