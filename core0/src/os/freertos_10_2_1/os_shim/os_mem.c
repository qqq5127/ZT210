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

/* Free RTOS includes */
#include "FreeRTOS.h"

#include "types.h"
#include "string.h"
#include "modules.h"

#include "lib_dbglog.h"

/* os shim includes */
#include "os_mem.h"

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
#include "riscv_cpu.h"
#include "critical_sec.h"
#endif

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1

/* OS_MALLOC_DEBUG configerations */

#ifndef OS_MALLOC_DEBUG_CFG_SUMMARY_NUM
#if OS_MALLOC_DEBUG_LEVEL >= 2

#ifdef BUILD_CORE_CORE0
#define OS_MALLOC_DEBUG_CFG_SUMMARY_NUM 108
#else
#define OS_MALLOC_DEBUG_CFG_SUMMARY_NUM 72
#endif /* BUILD_CORE_CORE */

#else /* OS_MALLOC_DEBUG_LEVEL < 2 */

#ifdef BUILD_CORE_CORE0
#define OS_MALLOC_DEBUG_CFG_SUMMARY_NUM 60
#else
#define OS_MALLOC_DEBUG_CFG_SUMMARY_NUM 48
#endif /* BUILD_CORE_CORE */

#endif /* OS_MALLOC_DEBUG_LEVEL */
#endif /* OS_MALLOC_DEBUG_CFG_SUMMARY_NUM */

#ifndef OS_MALLOC_DEBUG_CFG_MID_MASKS_BASIC
#define OS_MALLOC_DEBUG_CFG_MID_MASKS_BASIC     0xFFFFFFFF
#endif

#ifndef OS_MALLOC_DEBUG_CFG_MID_MASKS_LIB
#define OS_MALLOC_DEBUG_CFG_MID_MASKS_LIB       0xFFFFFFFF
#endif

#ifndef OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_STACK
#define OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_STACK  0xFFFFFFFF
#endif

#ifndef OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_PHY
#define OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_PHY    0xFFFFFFFF
#endif

#ifndef OS_MALLOC_DEBUG_CFG_MID_MASKS_APP
#define OS_MALLOC_DEBUG_CFG_MID_MASKS_APP       0xFFFFFFFF
#endif

#ifndef OS_MALLOC_DEBUG_CFG_SIZE_LOW_THR
#define OS_MALLOC_DEBUG_CFG_SIZE_LOW_THR        0
#endif

#ifndef OS_MALLOC_DEBUG_CFG_SIZE_HIGH_THR
#define OS_MALLOC_DEBUG_CFG_SIZE_HIGH_THR       65535
#endif

#define RESERVED_MEM_SIZE sizeof(heap_dbg_block_trace_t)

typedef struct {
    uint32_t ra;
    uint8_t  module_id;
    uint8_t  cnt;                   //max 255 Times
    uint16_t max_single_size;       //max 64KB
    uint16_t max_watermark;         //max 64KB
    uint16_t cur_watermark;         //max 64KB
} heap_dbg_summary_t;

typedef struct {
    uint32_t ra;
    uint16_t size;
    uint16_t reserved:15;
    uint16_t in_summary:1;
} heap_dbg_block_trace_t;

typedef struct {
    struct {
        uint32_t basic_mask;
        uint32_t lib_mask;
        uint32_t bt_stack_mask;
        uint32_t bt_phy_mask;
        uint32_t app_mask;
    } mid_masks;
    size_t size_low_thr;  //if less then the LOW THR bytes, do not record in dbg summary
    size_t size_high_thr; //if larger then the LOW THR bytes, do not record in dbg summary
} heap_dbg_cfg_t;
#endif

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
static heap_dbg_summary_t s_heap_dbg_summary[OS_MALLOC_DEBUG_CFG_SUMMARY_NUM] = {0};

static heap_dbg_cfg_t s_heap_dbg_cfg = {
    //default value
    .mid_masks = {
        .basic_mask = OS_MALLOC_DEBUG_CFG_MID_MASKS_BASIC,
        .lib_mask = OS_MALLOC_DEBUG_CFG_MID_MASKS_LIB,
        .bt_stack_mask = OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_STACK,
        .bt_phy_mask = OS_MALLOC_DEBUG_CFG_MID_MASKS_BT_PHY,
        .app_mask = OS_MALLOC_DEBUG_CFG_MID_MASKS_APP,
    },
    .size_low_thr = OS_MALLOC_DEBUG_CFG_SIZE_LOW_THR,
    .size_high_thr = OS_MALLOC_DEBUG_CFG_SIZE_HIGH_THR,
};

static uint32_t s_heap_dbg_summary_dump_cnt = 0;
#endif

bool_t g_malloc_need_init = false;
uint32_t heap_tatol_size = 0;

/* both define macro here to wrap all debug functions and define macro
 * in the body of each debug functions.
 * 1. wrap all debug function is normal correct operation for rom1.1
 * 2. wrap body in each debug function is to let romrespin compile successflly,
 *    because respin ignore all the macro which is out of function body.
 */
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
static bool os_mem_heap_dbg_condition_check(uint8_t module_id, size_t size)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    if (size < s_heap_dbg_cfg.size_low_thr ||
            size > s_heap_dbg_cfg.size_high_thr) {
        return false;
    }

    if (module_id >= IOT_APP_MID_START) {
        if ((1<<(module_id - IOT_APP_MID_START)) & s_heap_dbg_cfg.mid_masks.app_mask) {
            return true;
        }
    } else if (module_id >= BT_PHY_MID_START) {
        if ((1<<(module_id - BT_PHY_MID_START)) & s_heap_dbg_cfg.mid_masks.bt_phy_mask) {
            return true;
        }
    } else if (module_id >= BT_STACK_MID_START) {
        if ((1<<(module_id - BT_STACK_MID_START)) & s_heap_dbg_cfg.mid_masks.bt_stack_mask) {
            return true;
        }
    } else if (module_id >= LIB_MID_START) {
        if ((1<<(module_id - LIB_MID_START)) & s_heap_dbg_cfg.mid_masks.lib_mask) {
            return true;
        }
    } else {
        //module_id >= IOT_BASIC_MID_START
        if ((1<<(module_id - IOT_BASIC_MID_START)) & s_heap_dbg_cfg.mid_masks.basic_mask) {
            return true;
        }
    }

    return false;
#else
    UNUSED(module_id);
    UNUSED(size);

    return false;
#endif
}

static int os_mem_heap_dbg_search_idx_by_ra(uint32_t ra, bool is_alloc)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    int idx;

    for (idx = 0; idx < OS_MALLOC_DEBUG_CFG_SUMMARY_NUM; idx++) {
        if (s_heap_dbg_summary[idx].ra == 0) {
            //new caller address
            assert(is_alloc);
            break;
        } else if (s_heap_dbg_summary[idx].ra == ra) {
            //the caller address already exist
            break;
        }
    }

    if (idx == OS_MALLOC_DEBUG_CFG_SUMMARY_NUM) {
        idx = -1;
    }

    // -1: full, [0:OS_MEM_DBG_INFO_NUM-1]:success
    return idx;
#else
    UNUSED(ra);
    UNUSED(is_alloc);

    return -1;
#endif
}

static void os_mem_Heap_dbg_summary_dump(void)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    uint8_t core_id = cpu_get_mhartid();

    DBGLOG_OS_INFO("Heap Debug Summary Dump Start: Core %d, Cnt %d\n",
                   core_id, s_heap_dbg_summary_dump_cnt);

    for (int idx = 0; idx < OS_MALLOC_DEBUG_CFG_SUMMARY_NUM; idx++)
    {
        if (s_heap_dbg_summary[idx].ra == 0)
        {
            //already foreach end
            break;
        }

        DBGLOG_OS_RAW("MA: I %d M %d A %x C %u MS %u MW %u CW %u\n",
                      cpu_get_mhartid(),
                      s_heap_dbg_summary[idx].module_id,
                      s_heap_dbg_summary[idx].ra,
                      s_heap_dbg_summary[idx].cnt,
                      s_heap_dbg_summary[idx].max_single_size,
                      s_heap_dbg_summary[idx].max_watermark,
                      s_heap_dbg_summary[idx].cur_watermark);
    }

    DBGLOG_OS_INFO("Heap Debug Summary Dump End: Core %d, Cnt %d\n",
                   core_id, s_heap_dbg_summary_dump_cnt);

    s_heap_dbg_summary_dump_cnt++;
#endif
}

void *os_mem_malloc_dbg(module_id_t module_id, size_t size, uint32_t ra) IRAM_TEXT(os_mem_malloc_dbg);
void *os_mem_malloc_dbg(module_id_t module_id, size_t size, uint32_t ra)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    heap_dbg_block_trace_t *dbg_block;
    size_t orig_size;
    uint32_t ret_addr;

    if (ra == 0) {
        ret_addr = CPU_GET_RET_ADDRESS();
    } else {
        ret_addr = ra;
    }

    orig_size = size;
    size += RESERVED_MEM_SIZE;

    void *buf = pvPortMalloc(size);
    if (buf) {
        if (g_malloc_need_init) {
            memset(buf, 0, size);
        }

        dbg_block = (void *)buf;
        dbg_block->ra = ret_addr;
        dbg_block->size = orig_size;

        buf = (uint8_t *)buf + RESERVED_MEM_SIZE;

        cpu_critical_enter();

        if (os_mem_heap_dbg_condition_check(module_id, orig_size)) {
            int idx = os_mem_heap_dbg_search_idx_by_ra(ret_addr, true);

            if (idx >= 0 && idx < OS_MALLOC_DEBUG_CFG_SUMMARY_NUM) {
                s_heap_dbg_summary[idx].ra = ret_addr;

                if (s_heap_dbg_summary[idx].max_single_size < orig_size) {
                    s_heap_dbg_summary[idx].max_single_size = orig_size;
                }

                s_heap_dbg_summary[idx].cur_watermark += orig_size;

                if (s_heap_dbg_summary[idx].max_watermark < s_heap_dbg_summary[idx].cur_watermark) {
                    s_heap_dbg_summary[idx].max_watermark = s_heap_dbg_summary[idx].cur_watermark;
                }

                s_heap_dbg_summary[idx].cnt++;

                dbg_block->in_summary = 1;
            } else {
                dbg_block->in_summary = 0;
            }
        } else {
            dbg_block->in_summary = 0;
        }

        cpu_critical_exit();
    } else {
        assert(0);
    }

    return buf;
#else
    UNUSED(module_id);
    UNUSED(size);
    UNUSED(ra);

    return NULL;
#endif
}

void os_mem_free_dbg(void *ptr) IRAM_TEXT(os_mem_free_dbg);
void os_mem_free_dbg(void *ptr)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    heap_dbg_block_trace_t *dbg_block = (void *)((uint8_t *)ptr - RESERVED_MEM_SIZE);

    ptr = (void *)dbg_block;

    cpu_critical_enter();

    if (dbg_block->in_summary != 0) {
        int idx = os_mem_heap_dbg_search_idx_by_ra(dbg_block->ra, false);

        if (idx >= 0 && idx < OS_MALLOC_DEBUG_CFG_SUMMARY_NUM) {
            s_heap_dbg_summary[idx].cur_watermark -= dbg_block->size;
            s_heap_dbg_summary[idx].cnt--;
        }
    }

    cpu_critical_exit();

    vPortFree(ptr);
#else
    UNUSED(ptr);
#endif
}
#endif

void os_heap_init(const os_heap_region_t *region)
{
    const os_heap_region_t *heap_region = region;
    while (heap_region->start && heap_region->length) {
        heap_tatol_size += heap_region->length;
        heap_region++;
    }

    vPortDefineHeapRegions((const HeapRegion_t *)region); /*lint !e740 Unusual pointer cast.*/
}

uint32_t os_get_heap_size(void)
{
    return heap_tatol_size;
}

void os_set_stack(uint32_t stack_top)
{
    extern uint32_t xISRStackTop;
    xISRStackTop = stack_top;
}

void os_mem_malloc_need_init(bool_t value)
{
    g_malloc_need_init = value;
}

void *os_mem_malloc(module_id_t module_id, size_t size) IRAM_TEXT(os_mem_malloc);
void *os_mem_malloc(module_id_t module_id, size_t size)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    return os_mem_malloc_dbg(module_id, size, ret_addr);
#else
    UNUSED(module_id);   // avoid warning. to be fixed.

    void *buf = pvPortMalloc(size);
    if (buf) {
        if (g_malloc_need_init) {
            memset(buf, 0, size);
        }
    }

    return buf;
#endif
}

void os_mem_free(void *ptr) IRAM_TEXT(os_mem_free);
void os_mem_free(void *ptr)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    os_mem_free_dbg(ptr);
#else
    vPortFree(ptr);
#endif
}

void *os_mem_malloc_panic(module_id_t module_id, size_t size) IRAM_TEXT(os_mem_malloc_panic);
void *os_mem_malloc_panic(module_id_t module_id, size_t size)
{
#ifdef OS_MALLOC_DEBUG
    cpu_critical_enter();
    os_mem_total[module_id] += size;
    cpu_critical_exit();
    size += RESERVED_MEM_SIZE;
#else
    UNUSED(module_id);   // avoid warning. to be fixed.
#endif

    void *buf = pvPortMallocInPanic(size);
    if (buf) {
        if (g_malloc_need_init) {
            memset(buf, 0, size);
        }
#ifdef OS_MALLOC_DEBUG
        *(uint32_t *)buf = (size - RESERVED_MEM_SIZE) | (module_id << 24);
        buf = (uint8_t *)buf + RESERVED_MEM_SIZE;
#endif
    } else {
#ifdef OS_MALLOC_DEBUG
        assert(0);
#endif
    }
    return buf;
}

void os_mem_free_panic(void *ptr) IRAM_TEXT(os_mem_free_panic);
void os_mem_free_panic(void *ptr)
{
#ifdef OS_MALLOC_DEBUG
    ptr = (uint8_t *)ptr - RESERVED_MEM_SIZE;
    module_id_t mid = *(uint32_t *)ptr >> 24;
    uint32_t size = *(uint32_t *)ptr & 0xffffff;
    cpu_critical_enter();
    os_mem_total[mid] -= size;
    cpu_critical_exit();
#endif
    vPortFreeInPanic(ptr);
}

void *os_mem_aligned_malloc(module_id_t module_id, size_t bytes, size_t alignment)
    IRAM_TEXT(os_mem_aligned_malloc);
void *os_mem_aligned_malloc(module_id_t module_id, size_t bytes, size_t alignment)
{
    void *ptr;
    long diff;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    ptr = os_mem_malloc_dbg(module_id, bytes + alignment, ret_addr);
#else
    ptr = os_mem_malloc(module_id, bytes + alignment);
#endif
    if (ptr) {
        diff = (long)(((~(unsigned long)ptr) & (alignment - 1)) + 1);
        ptr = (char *)ptr + diff;
        ((char *)ptr)[-1] = (char)diff;
    }
    return ptr;
}

void os_mem_aligned_free(void *p) IRAM_TEXT(os_mem_aligned_free);
void os_mem_aligned_free(void *p)
{
    if (p) {
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
        os_mem_free_dbg((char *)p - ((char *)p)[-1]);
#else
        os_mem_free((char *)p - ((char *)p)[-1]);
#endif
    }
}

uint32_t os_mem_get_heap_free(void)
{
    return (xPortGetFreeHeapSize());
}

uint32_t os_mem_get_heap_lowest_free(void)
{
    return (xPortGetMinimumEverFreeHeapSize());
}

void os_mem_display_heap_malloc_info(void)
{
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 1
    os_mem_Heap_dbg_summary_dump();
#endif
}
