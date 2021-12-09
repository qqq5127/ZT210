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
#ifndef KEY_WQ_TOUCH__H_
#define KEY_WQ_TOUCH__H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup KEY_SENSOR
 * @{
 * This section introduces the LIB KEY_SENSOR module's enum, structure, functions and how to use this module.
 */
/**
 * @addtogroup WQ_TOUCH_KEY
 * @{
 * This section introduces the LIB KEY_SENSOR WQ_TOUCH_KEY module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "key_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WQ_TOUCH_KEY_DIV_32K_FREQ,
    WQ_TOUCH_KEY_DIV_16K_FREQ,
    WQ_TOUCH_KEY_DIV_10K_FREQ,
    WQ_TOUCH_KEY_DIV_8K_FREQ,
} WQ_TOUCH_KEY_DIV_FREQ;

/**
 * @brief This function is init key wq touch module.
 *
 * @param id_cfg key id config
 */
void key_wq_touch_init(const key_id_cfg_t *id_cfg);

/**
 * @brief deinit the key wq touch module.
 */
void key_wq_touch_deinit(bool_t wakeup_enable);

/**
 * @brief This function is used to open wuqi touch key.
 * @param thres_cfg key threshold config
 * @param can_power_on power on function enabled
 */
void key_wq_touch_open(const key_thres_cfg_t *thres_cfg, bool_t can_power_on);

/**
 * @brief This function is used to enable wuqi touch key.
 * @param enable false -- disable, true -- enable
 * @return uint8_t RET_OK for success else for error
 */
uint8_t key_wq_touch_set_enable(bool_t enable);

/**
 * @brief This function is used to adjust touch threshold.
 * @param climb_inc climb threshold increment, 0 means revert to default value
 * @param fall_inc fall threshold increment, 0 means revert to default value
 * @return uint8_t RET_OK for success else for error
 */
uint8_t key_wq_touch_adj_thres(uint16_t climb_inc, uint16_t fall_inc);

/**
 * @brief This function is used to adjust touch frequency.
 * @param freq need to set the frequency
 */
void key_wq_touch_adj_freq(WQ_TOUCH_KEY_DIV_FREQ freq);

/**
 * @brief This function is used to adjust touch trig times.
 * @param climb_trig_times climb trig times, 0 means revert to default value
 * @param fall_trig_times fall trig times, 0 means revert to default value
 * @return uint8_t RET_OK for success else for error
 */
uint8_t key_wq_touch_adj_trig_times(uint8_t climb_trig_times, uint8_t fall_trig_times);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup WQ_TOUCH_KEY
 */

/**
 * @}
 * addtogroup KEY_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* KEY_WQ_TOUCH__H_ */
