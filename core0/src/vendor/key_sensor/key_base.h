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
#ifndef KEY_BASE_H__
#define KEY_BASE_H__

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup KEY_SENSOR
 * @{
 * This section introduces the LIB KEY_SENSOR module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "key_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_WORD16 0xFFFF
#define INVALID_UCHAR  0xFF

/** @defgroup lib_key_sensor_enum Enum
 * @{
 */
typedef enum {
    CHECK_PROGRESSING = 0,
    CHECK_DONE,
} key_check_result_t;
/**
 * @}
 */

/**
 * @brief This function is init key management module.
 * @param callback to handle key press
 */
void key_base_init(key_callback_t callback);

/**
 * @brief This function is deinit the key management module.
 */
void key_base_deinit(void);

/**
 * @brief This function is to set key parameters
 * @param time_cfg key time config
 */
void key_base_set_time_cfg(const key_time_cfg_t *time_cfg);

/**
 * @brief This function is or checking if the specific key pressed.
 *
 * @param key_id id of the key
 * @param key_src src of the key
 * @return true or false for checking key pressed.
 */
bool_t key_base_is_key_pressed(uint8_t key_id, uint8_t key_src);

/**
 * @brief This function is for checking if all keys released.
 * @return true or false for checking all released.
 */
bool_t key_base_all_key_released(void);

/**
 * @brief This function is to set key parameters
 *
 * @param key_id id of the key 
 * @param key_src src of the key
 * @param io_id io id of the key
 */
void key_base_register(uint8_t key_id, uint8_t key_src, uint16_t io_id);

/**
 * @brief This function is called when key pressed.
 * @param io_id is the io id of the pressed key.
 * @param time_offset is for compensating the time from waking-up to processing the IO changing.
 *
 */
void key_base_pressed(uint16_t io_id, uint32_t time_offset);

/**
 * @brief This function is called when key released.
 * @param io_id is the io id of the released key.
 *
 */
void key_base_released(uint16_t io_id);

/**
 * @brief This function is called when checking timer is out.
 * @return uint32_t CHECK_DONE for checking done else others .
 */
key_check_result_t key_base_check_type(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup KEY_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* KEY_BASE_H__ */
