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
#ifndef _DRIVER_HW_LEDC_H_
#define _DRIVER_HW_LEDC_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEDC_MAX_LED_NUM     8
#define LEDC_MAX_LED_TMR_NUM 4

typedef enum {
    LEDC_DONE_INT = 0x0,
    LEDC_TMR_RELOAD_INT,
    LEDC_TMR_OVF_INT,
    LEDC_INT_TYPE_MAX,
} LEDC_INT_TYPE;

typedef enum {
    LEDC_DUTY_MODE_NORMAL = 0x0,
    /*light -> off -> off ->light*/
    LEDC_DUTY_MODE_CHANGE_LOOP1 = 0x1,
    /* light -> off -> light ->off*/
    LEDC_DUTY_MODE_CHANGE_LOOP2 = 0x2,

} LEDC_DUTY_MODE;

typedef enum {
    LEDC_CTRL_PMM,
    LEDC_CTRL_DTOP,
    LEDC_CTRL_MAX_NUM,
} LEDC_CTRL;
typedef enum {
    LEDC_OUTPUT_LVL_L = 0,
    LEDC_OUTPUT_LVL_H = 1,
} LEDC_OUTPUT_LVL;

int32_t ledc_out_pin_set(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint16_t pin);
void ledc_init(LEDC_CTRL ledc_ctrl_id);
void ledc_disable_all_int(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint8_t tmr_id);
void ledc_clr_all_int(LEDC_CTRL ledc_ctrl_id, uint8_t id);
void ledc_set_hl_val(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint16_t h2l_value, uint16_t l2h_value);
void ledc_tmr_close(LEDC_CTRL ledc_ctrl_id, uint8_t tmr_id);
void ledc_tmr_cfg(LEDC_CTRL ledc_ctrl_id, uint8_t tmr_id, uint16_t div);
void ledc_tmr_sel(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint8_t tmr_id);
void ledc_set_duty_num(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint16_t num);
void ledc_set_duty_mode(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint8_t mode);
void ledc_start(LEDC_CTRL ledc_ctrl_id, uint8_t id);
void ledc_stop(LEDC_CTRL ledc_ctrl_id, uint8_t id);
void ledc_set_duty_scale(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint8_t val);
void ledc_set_duty_thrs(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint16_t val);
void ledc_set_duty_load(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint16_t val, bool_t reload);
void ledc_deinit(LEDC_CTRL ledc_ctrl_id);
void ledc_set_outinv(LEDC_CTRL ledc_ctrl_id, uint8_t id, uint8_t val);
void ledc_set_idle_output(LEDC_CTRL ledc_ctrl_id, uint8_t id, LEDC_OUTPUT_LVL level);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_LEDC_H_ */
