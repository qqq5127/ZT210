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
#ifndef KEY_SENSOR__H_
#define KEY_SENSOR__H_

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
#include "dbglog.h"
#include "modules.h"

#define DBGLOG_KEY_SENSOR_RAW(fmt, arg...) \
    DBGLOG_LOG(IOT_SENSOR_HUB_MANAGER_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_KEY_SENSOR_INFO(fmt, arg...) \
    DBGLOG_STREAM_INFO(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)
#define DBGLOG_KEY_SENSOR_ERROR(fmt, arg...) \
    DBGLOG_STREAM_ERROR(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)

#define KEY_DRIVER_NONE        0
#define KEY_DRIVER_CUSTOMIZE   1
#define KEY_DRIVER_SIMPLE_IO   2
#define KEY_DRIVER_DEBOUNCE_IO 3
#define KEY_DRIVER_WQ_TOUCH    4
#define KEY_DRIVER_SC7A20      5
#define KEY_DRIVER_DF100       6
#define KEY_DRIVER_DF230       7
#define KEY_DRIVER_EXT_TOUCH   8
#define KEY_DRIVER_NDT_PT135   9
#define KEY_DRIVER_AW8686X     10

#ifndef KEY_DRIVER_SELECTION
#define KEY_DRIVER_SELECTION KEY_DRIVER_DEBOUNCE_IO
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_KEY_NUM 3

#define KEY_SRC_IO       0
#define KEY_SRC_TOUCH    1
#define KEY_SRC_EXTERNAL 2

/** @defgroup lib_key_sensor_enum Enum
 * @{
 */
typedef enum {
    BTN_TYPE_SINGLE = 0,
    BTN_TYPE_DOUBLE,
    BTN_TYPE_TRIPLE,
    BTN_TYPE_QUADRUPLE,
    BTN_TYPE_QUINTUPLE,
    BTN_TYPE_SEXTUPLE,  // 5
    BTN_TYPE_SHORT,
    BTN_TYPE_LONG,
    BTN_TYPE_LONG_RELEASE,
    BTN_TYPE_VLONG,
    BTN_TYPE_VLONG_RELEASE,  // 10
    BTN_TYPE_VVLONG,
    BTN_TYPE_VVLONG_RELEASE,
    BTN_TYPE_REPEAT,
    BTN_TYPE_PRESS,
    BTN_TYPE_RELEASE,
    BTN_LAST
} key_pressed_type_t;
/**
 * @}
 */

/** @defgroup lib_key_sensor_struct Struct
 * @{
 */
typedef struct {
    uint16_t long_time;   /* long press time */
    uint16_t vlong_time;  /* very long press time */
    uint16_t vvlong_time; /* very very long press time */
    uint16_t start_time;  /* Start time after that repeat counted */
    uint16_t repeat_time; /* repeat rate */
    /* maximum interval between continuous twice taps in one Multi-tap event. */
    uint16_t multi_tap_interval;
    uint8_t debounce_time; /* interval between debounce check */
} key_time_cfg_t;

typedef struct {
    uint8_t num;
    uint8_t src[MAX_KEY_NUM];
    uint8_t id[MAX_KEY_NUM];
} key_id_cfg_t;

typedef struct {
    uint32_t climb_thres;
    uint32_t fall_thres;
} key_thres_cfg_t;

typedef struct {
    key_id_cfg_t id;
    key_time_cfg_t time;
    key_thres_cfg_t thres;
} key_cfg_t;

typedef struct {
    uint8_t num; /* pressed key num */
    uint8_t src[MAX_KEY_NUM];
    uint8_t id[MAX_KEY_NUM];
    uint8_t type[MAX_KEY_NUM];
} key_pressed_info_t;
/**
 * @}
 */

/**
 * @brief callback to handle key press
 * @param info key pressed information
 */
typedef void (*key_callback_t)(const key_pressed_info_t *info);

/**
 * @brief This function is init key management module.
 *
 * @param key_id key id config
 * @param callback the callback to handle keys
 */
void key_sensor_init(const key_id_cfg_t *key_id, key_callback_t callback);

/**
 * @brief deinit the key management module.
 */
void key_sensor_deinit(bool_t wakeup_enable);

/**
 * @brief This function is used to open sensor.
 *
 * @param key_cfg key config
 * @param can_power_on power on function enalbed
 */
void key_sensor_open(const key_cfg_t *key_cfg, bool_t can_power_on);

/**
 * @brief This function is for checking if all keys released.
 *
 * @return bool_t true or false for checking all released.
 */
bool_t key_sensor_all_key_released(void);

/**
 * @brief
 * @param key_id id of the key
 * @param key_src src of the key
 *
 * @return true if the key is pressed, false if not
 */
bool_t key_sensor_is_key_pressed(uint8_t key_id, uint8_t key_src);

/**
 * @brief enable/disable the key sensor
 *
 * @param enable true if enable, false if not
 */
void key_sensor_set_enabled(bool_t enable);


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
#endif /* KEY_SENSOR__H_ */
