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

#ifndef _DRIVER_NON_OS_LPM_H
#define _DRIVER_NON_OS_LPM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup HAL
 * @{
 * @addtogroup LPM
 * @{
 * This section introduces the LPM module's functions and how to use this driver.
 */

#include "types.h"

#define IOT_LPM_PD_SHUTDOWN             (8)
#define IOT_LPM_CORE_IS_SHUTDOWN(x) ((x) == IOT_LPM_PD_SHUTDOWN)

typedef enum {
    IOT_LPM_DS_WAKEUP_GPIO,
    IOT_LPM_DS_WAKEUP_RTC1,
    IOT_LPM_DS_WAKEUP_RTC0,
    IOT_LPM_DS_WAKEUP_BAT_RESUME,
    IOT_LPM_DS_WAKEUP_CHARGER_ON,
    IOT_LPM_DS_WAKEUP_CHARGER_2P5,
    IOT_LPM_DS_WAKEUP_TOUCH_KEY,
    IOT_LPM_DS_WAKEUP_GPI_DEB,
    IOT_LPM_DS_WAKEUP_WIC0,
    IOT_LPM_DS_WAKEUP_WIC1,
    IOT_LPM_DS_WAKEUP_BT,
} IOT_LPM_DS_WAKEUP_SRC;

/**
 * @brief This function is to init the function of pmm waking up rtc.
 */
void iot_lpm_pmm_wakeup_rtc_init(void);

/**
 * @brief This function is to let lpm enter lightsleep module.
 */
void iot_lpm_enter_lightsleep(void);

/**
 * @brief This function is to enable or disable the xtal for lowpower.
 *
 */
void iot_lpm_xtal_lowpower_enable(bool_t enable);

/**
 * @brief This function is to disable or enable the xtal.
 *
 * @param enable is the signal.
 */
void iot_lpm_xtal_enable(bool_t enable);

/**
 * @brief This function is to let lpm enter deepsleep module.
 */
void iot_lpm_enter_deepsleep(void);

/**
 * @brief Restore sleep state when exit from deepsleep.
 */
void iot_lpm_exit_deepsleep(void);

/**
 * @brief This function is to set dig rtc to wake up source,can only wakeup in light sleep.
 *
 * @param time is the time to set for timer.
 */
void iot_lpm_set_dig_rtc_wake_src(uint32_t time);

/**
 * @brief This function is to set pmm rtc to wake up source.
 *
 * @param slp_st_rtc The rtc time sleep start
 * @param time_rtc Total time allow to sleep
 */
uint8_t iot_lpm_set_pmm_rtc_wakeup_src(uint32_t slp_st_rtc, uint32_t time_rtc);

/**
 * @brief This function is to set pmm gpio to wake up source.
 *
 * @param gpio is the gpio id.
 */
void iot_lpm_set_pmm_gpio_wakeup_src(uint16_t gpio);

/**
 * @brief This function is to init lpm module.
 *
 */
void iot_lpm_init(void);

/**
 * @brief This function is to get the count of the light sleep.
 *
 * @return uint32_t is the count of the light sleep.
 */
uint32_t iot_lpm_get_light_slp_cnt(void);

/**
 * @brief This function is to get the count of the deep sleep.
 *
 * @return uint32_t is the count of the deep sleep.
 */
uint32_t iot_lpm_get_deep_slp_cnt(void);

/**
 * @brief This function is to get the count of the performance monitor.
 *
 * @param value select the monitor.
 * @return uint32_t is the count.
 */
uint32_t iot_lpm_get_perf_mon_cnt(uint8_t value);

/**
 * @brief This function is to enter the shutdown.
 *
 */
void iot_lpm_enter_shutdown(void);

/**
 * @brief This fucntion is to check source whether wakeup from charger.
 *
 * @return bool_t true wakeup from charge and other false.
 */
bool_t iot_lpm_wakeup_from_charger(void);

/**
 * @brief This function is to get all power domain status.
 *
 * @param dcore_status is pointer to store DCORE power domain status.
 * @param bt_status is pointer to store BT power domain status.
 * @param dsp_status is pointer to store AUDIO power domain status.
 * @param audif_status is pointer to store AUDIO_INF power domain status.
 */
void iot_lpm_get_power_domain_status(uint8_t *dcore_status,
                uint8_t *bt_status, uint8_t *dsp_status, uint8_t *audif_status);

/**
 * @brief This function is to get all power domain wakeup request source.
 *
 * @param dcore_src is pointer to store DCORE power domain wakeup request source.
 * @param bt_src is pointer to store BT power domain wakeup request source.
 * @param dsp_src is pointer to store AUDIO power domain wakeup request source.
 */
void iot_lpm_get_wakeup_req_src(uint16_t *dcore_src,
                        uint16_t *bt_src, uint16_t *dsp_src);

/**
 * @brief assign wakeup source
 *
 * @param src the wakeup source assigned
 * @param enable enable or disable
 */
void iot_lpm_set_pd_wakeup_src_req(IOT_LPM_DS_WAKEUP_SRC src, bool_t enable);

/**
 * @brief This function would disable all wakeup source
 *
 */
void iot_lpm_clear_all_pd_wakeup_src_req(void);

/**
 * @brief This function would restore doze sleep config
 */
void iot_lpm_enter_dozesleep(void);

/**
 * @brief This function would return if current have pending rtc int for lpm rtc
 *
 * @return true if have rtc INT pending
 */
uint32_t iot_lpm_pmm_rtc_get_sts(void);

uint32_t iot_lpm_get_wakeup_count(void);

/**
 * @brief Set all dcdc08, dcdc12, dcdc18 pulse width
 *
 * @param pulse_width The pulse width to set, 0 - 15.
 */
void iot_lpm_set_dcdc_pulse_width(uint8_t pulse_width);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* _DRIVER_NON_OS_LPM_H */
