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

#ifndef OS_SHIM_MEM_H
#define OS_SHIM_MEM_H
/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_MEM
 * @{
 * This section introduces OS memory reference api.
 */
#include "modules.h"
#ifndef LIB_DBGLOG_ENABLE
#include "stdio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIB_DBGLOG_ENABLE
#define DBGLOG_OS_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_OS_SHIM_MID, fmt, ##arg)
#define DBGLOG_OS_RAW(fmt, arg...)      DBGLOG_LOG_RAW(IOT_OS_SHIM_MID, fmt, ##arg)
#else
#define DBGLOG_OS_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_OS_RAW(fmt, arg...)      printf(fmt, ##arg)
#endif
typedef struct heap_region
{
    uint8_t *start;
    size_t length;
} os_heap_region_t;

/**
 * @brief
 *
 * @param region
 */
void os_heap_init(const os_heap_region_t *region);

/**
 * @brief
 *
 * @return uint32_t
 */
uint32_t os_get_heap_size(void);

/**
 * @brief
 *
 * @param stack_top
 */
void os_set_stack(uint32_t stack_top);

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
/**
 * @brief This function is used to allocate memory for debug. Dynamicallly allocate
 *        the number of bytes of memory. The allocated memory must be reset to 0.
 *
 * @param module_id is the module mallocing memory. For the debugging purpose.
 * @param size is number of bytes of memory to be allocated.
 * @param rar return address of caller.
 * @return void* null(failure), otherwise(pointer of allocated memory).
 */
void *os_mem_malloc_dbg(module_id_t module_id, size_t size, uint32_t ra);

/**
 * @brief This function is used to free memory.
 *
 * @param ptr is pointer to the memory to be free'd.
 */
void os_mem_free_dbg(void *ptr);
#endif

/**
 * @brief This function is used to allocate memory. Dynamicallly allocate
 *        the number of bytes of memory. The allocated memory must be reset to 0.
 * @note  This functiong can ONLY be used in 'PANIC CONTEXT',
 *        Don't use it unless you know what you are doing
 *
 * @param module_id is the module mallocing memory. For the debugging purpose.
 * @param size is number of bytes of memory to be allocated.
 * @return void* null(failure), otherwise(pointer of allocated memory).
 */
void *os_mem_malloc(module_id_t module_id, size_t size);

/**
 * @brief This function is used to allocate memory in panic context. Dynamicallly allocate
 *        the number of bytes of memory. The allocated memory must be reset to 0.
 * @note  This functiong can ONLY be used in 'PANIC CONTEXT',
 *        Don't use it unless you know what you are doing
 *
 * @param module_id is the module mallocing memory. For the debugging purpose.
 * @param size is number of bytes of memory to be allocated.
 * @return void* null(failure), otherwise(pointer of allocated memory).
 */
void *os_mem_malloc_panic(module_id_t module_id, size_t size);

/**
 * @brief This function is used to free memory.
 *
 * @param ptr is pointer to the memory to be free'd.
 */
void os_mem_free(void *ptr);

/**
 * @brief This function is used to free memory in panic context.
 *
 * @param ptr is pointer to the memory to be free'd.
 */
void os_mem_free_panic(void *ptr);

/**
 * @brief This function is used toallocate tagged memory. Dynamicallly allocate
 *        the number of bytes of memory. The allocated memory must be reset to 0.
 *
 * @param module_id is the module mallocing memory. For the debugging purpose.
 * @param bytes is number of bytes of memory to be allocated.
 * @param alignment is alignment number.
 * @return void* null(failure), otherwise(pointer of allocated memory).
 */
void *os_mem_aligned_malloc(module_id_t module_id, size_t bytes,
                            size_t alignment);

/**
 * @brief This function is used to free memory.
 *
 * @param p is alignement pointer to the memory to be free'd.
 */
void os_mem_aligned_free(void *p);

/**
 * @brief This function is used to whether buffer data set to zero.
 *
 * @param value is 0(not set to zero), 1(set to zero).
 */
void os_mem_malloc_need_init(bool_t value);

/**
 * @brief This function is used to Get heap's free memory size.
 *
 * @return uint32_t free memory size.
 */
uint32_t os_mem_get_heap_free(void);

/**
 * @brief This function is used to Get heap's lowest free memory size.
 *
 * @return uint32_t lowest free memory size.
 */
uint32_t os_mem_get_heap_lowest_free(void);

/**
 * @brief This function is used to dump heap's malloc size with module id.
 */
void os_mem_display_heap_malloc_info(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OS
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS_MEM
 */

#endif /* OS_SHIM_MEM_H */
