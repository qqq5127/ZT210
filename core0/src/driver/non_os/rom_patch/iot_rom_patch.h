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

#ifndef _DRIVER_NON_OS_ROM_PATCH_H
#define _DRIVER_NON_OS_ROM_PATCH_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup ROM_PATCH
 * @{
 * This section introduces the ROM_PATCH module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_EBREAK_TRIGER      0x9002
#define OS_LONG_EBREAK_TRIGER 0x100073

#define IOT_BT_ROM_BLK0_ADDR_MAX 0x10080000

/** @brief ROM_PATCH id. */
typedef enum {
    IOT_ROM_PATCH0,
    IOT_ROM_PATCH1,
    IOT_ROM_PATCH_NUM_MAX,
} IOT_ROM_PATCH_ID;

/** @brief ROM_PATCH group. */
typedef enum {
    IOT_DTOP_BOOT_ROM_PATCH,
    IOT_BT_ROM_PATCH,
    IOT_DSP_ROM_PATCH,
    IOT_ROM_PATCH_GROUP_MAX,
} IOT_ROM_PATCH_GROUP;

/** @brief ROM_PATCH state. */
typedef enum {
    NOUSED,
    USED,
} IOT_ROM_PATCH_STATE;

typedef struct rompatch_func {
    uint32_t rom_addr;    //hardware rom addr
    uint32_t func_addr;   //new function addr
    uint8_t patch_number;
    uint8_t used_flag;   //used flag
    uint8_t patch_group;
    uint8_t patch_id;
} rompatch_func_t;

typedef struct rom_patch_info {
    IOT_ROM_PATCH_GROUP rom_patch_group;
    IOT_ROM_PATCH_ID patch_id;
    uint8_t patch_number;
    bool valid;
} rom_patch_info_t;

/**
 * @brief This function is to enable rom patch controller.
 *
 * @param group is rom patch controller group.
 * @param patch_id is rom patch controller id.
 */
void iot_rom_patch_controller_enable(IOT_ROM_PATCH_GROUP group,
                                     IOT_ROM_PATCH_ID patch_id);

/**
 * @brief This function is to disable rom patch controller.
 *
 * @param group is rom patch controller group.
 * @param patch_id is rom patch controller id.
 */
void iot_rom_patch_controller_disable(IOT_ROM_PATCH_GROUP group,
                                      IOT_ROM_PATCH_ID patch_id);

/**
 * @brief This function is to disable single rom patch point.
 *
 * @param group is rom patch controller group.
 * @param member is rom patch number.
 */
void iot_rom_patch_points_disable(IOT_ROM_PATCH_GROUP group, uint8_t member);

/**
 * @brief This function is to install rom patch.
 *
 * @param rom_address is the address of the rom.
 * @param func_address is the address of the function.
 */
void iot_rom_patch_install(uint32_t rom_address, uint32_t func_address);

/**
 * @brief This function is to init rom patch.
 *
 */
void iot_rom_patch_init(void);

/**
 * @brief This function is to uninstall rom patch.
 *
 * @param rom_address is the address of the rom.
 */
void iot_rom_patch_unstall(uint32_t rom_address);

/**
 * @brief This function is to restore rom patch config when wakeup from deep sleep.
 *
 * @param data is the config before entry deep sleep.
 *
 * @return RET_OK.
 */
uint32_t iot_rom_patch_restore(uint32_t data);
#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup ROM_PATCH
 * @}
 * addtogroup HAL
 */
#endif /* _DRIVER_NON_OS_ROM_PATCH_H */
