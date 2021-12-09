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
#include "types.h"
#include "os_task.h"
#include "os_lock.h"
#include "os_timer.h"
#include "os_mem.h"
#include "lib_dbglog.h"
#include "riscv_cpu.h"
#include "iot_share_task.h"
#ifdef LOW_POWER_ENABLE
#include "power_mgnt_display.h"
#endif

#include "iot_cache.h"
#include "cpu_usage.h"

#ifdef CHECK_ISR_USAGE
#include "iot_irq.h"
#include "iot_rtc.h"
#endif
#include "iot_lpm.h"

#include "power_mgnt.h"
#include "iot_wdt.h"
#include "iot_soc.h"
#include "iot_audio.h"
#include "iot_rpc.h"
#include "iot_ipc.h"
#include <string.h>
#if defined(BUILD_CORE_CORE0)
#include "iot_sdm_dac.h"
#endif

// #define THREAD_STACK_DISPLAY
#define RPC_FREQUENCE_DETECT 1

//#define CHECK_WDT_INFO

typedef struct task_info_list_t {
    task_info_t *p_tasks;
    uint32_t num;
    uint32_t ts;
} task_info_list_t;

/*lint -esym(754, cpu_usage_cxt_t::ratio) */
typedef struct cpu_usage_cxt_t {
    task_info_list_t start;
    task_info_list_t end;
    uint32_t interval;
    uint32_t ratio;
    timer_id_t timer;
    void *lock;
} cpu_usage_ctxt;

static cpu_usage_ctxt g_cpu_usage_ctxt = {0};
#ifdef BUILD_CORE_CORE1
uint32_t g_phy_save[16] = {0};
uint32_t g_phy_save_cnt = 0;
#endif

static void os_cpu_usage_get_task_list(task_info_list_t *tasks)
{
    uint32_t task_num;
    uint32_t task_time;
    task_info_t *p_tasks;

    p_tasks = os_get_task_info(&task_num, &task_time);
    assert(p_tasks);

    tasks->p_tasks = p_tasks;
    tasks->num = task_num;
    tasks->ts = task_time;
}

static void os_cpu_usage_display_handler(const cpu_usage_ctxt *ctxt)
{
    task_info_t *p_tasks;
    task_info_t *p_tasks_prev;
    uint32_t i, j;
    uint32_t ts_span;
    uint32_t total_time;
    uint32_t ratio;
    uint32_t cpu = cpu_get_mhartid();
    uint32_t task_duty = 100;
#ifdef CHECK_ISR_USAGE
    task_duty = 100 - iot_isr_usage.isr_duty;
#endif

    total_time = ctxt->end.ts - ctxt->start.ts;

#ifdef BUILD_CORE_CORE0
    DBGLOG_LIB_CPU_USAGE_DEBUG("[auto]Core%d: Mem usage total:%dB free:%dB lowest:%dB cache missed:%d\n",
                    cpu, os_get_heap_size(), os_mem_get_heap_free(),
                    os_mem_get_heap_lowest_free(), iot_cache_get_miss_cnt(IOT_CACHE_SFC_ID));
#else
    DBGLOG_LIB_CPU_USAGE_DEBUG("[auto]Core%d: Mem usage total:%dB free:%dB lowest:%dB\n",
                    cpu, os_get_heap_size(), os_mem_get_heap_free(),
                    os_mem_get_heap_lowest_free());
    DBGLOG_LIB_CPU_USAGE_DEBUG("%d phy[0-7] %x %x %x %x %x %x %x %x\r\n",
                    g_phy_save_cnt,
                    g_phy_save[0], g_phy_save[1], g_phy_save[2], g_phy_save[3],
                    g_phy_save[4], g_phy_save[5], g_phy_save[6], g_phy_save[7]);

    DBGLOG_LIB_CPU_USAGE_DEBUG("%d phy[8-15] %x %x %x %x %x %x %x %x\r\n",
                    g_phy_save_cnt,
                    g_phy_save[8], g_phy_save[9], g_phy_save[10], g_phy_save[11],
                    g_phy_save[12], g_phy_save[13], g_phy_save[14], g_phy_save[15]);
#endif

    DBGLOG_LIB_CPU_USAGE_DEBUG("Total time: %u, feeddog: %d, slp_vote: 0x%x \n", total_time,
            iot_wdt_get_feed_cnt(), power_mgnt_get_sleep_vote());
#ifdef LOW_POWER_ENABLE
    power_mgnt_performance_display(cpu, ctxt->interval);
#endif
    p_tasks = ctxt->end.p_tasks;
    p_tasks_prev = ctxt->start.p_tasks;

    DBGLOG_LIB_CPU_USAGE_DEBUG("Index\tCycle/K\tRatio\tStack lowest\tThread name\tPri\n");
    DBGLOG_LIB_CPU_USAGE_DEBUG("----------------------------------------\n");

    for (i = 0; i < ctxt->start.num; i++) {

        for (j = 0; j < ctxt->end.num; j++) {
            if (p_tasks[j].id == p_tasks_prev[i].id)
                break;
        }

        if (j == ctxt->end.num)
            continue;

        ts_span = p_tasks[j].cpu_ts - p_tasks_prev[i].cpu_ts;
        ratio = ts_span * task_duty / total_time;
        DBGLOG_LIB_CPU_USAGE_DEBUG("%d\t%u\t%2d%%\t%d\t%c%c%c%c\t%d\n", i + 1, ts_span, ratio,
                        p_tasks[j].stack_size, p_tasks[j].name[0], p_tasks[j].name[1], p_tasks[j].name[2], p_tasks[j].name[3], p_tasks[j].priority);
    }

#ifdef CHECK_ISR_USAGE
    DBGLOG_LIB_CPU_USAGE_DEBUG("\t\t%3d%%\t\t        ISR duty\n" , iot_isr_usage.isr_duty);
#endif

#ifdef CHECK_WDT_INFO
    iot_wdt_dump_reg();
#endif
}

void os_cpu_usage_display_isr(void)
{
#ifdef CHECK_ISR_USAGE
    DBGLOG_LIB_CPU_USAGE_DEBUG("ISR/Critical time top%d:\n", MAX_SAVE_ISR_CNT);
    DBGLOG_LIB_CPU_USAGE_DEBUG("Top\tTime/us\tVector\tCritical-Time/us\tra\r\n");
    DBGLOG_LIB_CPU_USAGE_DEBUG("----------------------------------------------\n");
    for (int i = 0; i < MAX_SAVE_ISR_CNT; i++) {
        DBGLOG_LIB_CPU_USAGE_DEBUG("%d\t%d\t%d\t\t%d\t0x%x\n", i, iot_isr_usage.isr[i].dur_us,
                            iot_isr_usage.isr[i].vector_ra,
                            iot_isr_usage.critical[i].dur_us,
                            iot_isr_usage.critical[i].vector_ra);
    }

    uint32_t mask = cpu_disable_irq();
    memset(&iot_isr_usage.isr, 0 ,sizeof(iot_isr_usage.isr));
    memset(&iot_isr_usage.critical, 0 ,sizeof(iot_isr_usage.critical));
    cpu_restore_irq(mask);

    //display ISR duty
    uint32_t now = iot_rtc_get_global_time_ms();
    uint32_t display_duration = (now - iot_isr_usage.last_display_time) * 1000;
    iot_isr_usage.isr_duty = 100 * iot_isr_usage.isr_time_total_us / display_duration;
    DBGLOG_LIB_CPU_USAGE_DEBUG("%d/%d=%d%%\n" , iot_isr_usage.isr_time_total_us, display_duration, iot_isr_usage.isr_duty);
    //clear for next
    iot_isr_usage.last_display_time = now;
    iot_isr_usage.isr_time_total_us = 0;
#endif
}

static void lpm_info_display(void)
{
    uint8_t dtop_ps, bt_ps, dsp_ps, audif_ps;
    uint16_t dtop_src, bt_src, dsp_src;

    iot_lpm_get_power_domain_status(&dtop_ps, &bt_ps, &dsp_ps, &audif_ps);
    DBGLOG_LIB_CPU_USAGE_DEBUG("Power Domain Status : DTOP %d  BT %d  DSP %d  AUDIF %d\n",
                        dtop_ps, bt_ps, dsp_ps, audif_ps);

    iot_lpm_get_wakeup_req_src(&dtop_src, &bt_src, &dsp_src);
    DBGLOG_LIB_CPU_USAGE_DEBUG("Power Domain Wakeup Req Src : DTOP %x  BT %x  DSP %x\n",
                        dtop_src, bt_src, dsp_src);
#if defined(BUILD_CORE_CORE0)
    uint32_t power_mask = iot_soc_audio_intf_power_mask();
    uint32_t clk_status = iot_audio_get_global_clk_status();
    DBGLOG_LIB_CPU_USAGE_DEBUG("AUDIF power mask : 0x%x, AUDIF clock status: 0x%x cur_gain:%d\n",
            power_mask, clk_status, iot_sdm_dac_get_dig_gain());
#endif
}

static void os_status_display_handler(void *arg)
{
    cpu_usage_ctxt *ctxt = arg;

#ifdef CHECK_ISR_USAGE
    os_cpu_usage_display_isr();
#endif

#if RPC_FREQUENCE_DETECT
    iot_rpc_freq_detect();
    iot_ipc_freq_detect();
#endif

    os_cpu_usage_get_task_list(&ctxt->end);
    os_cpu_usage_display_handler(ctxt);

#ifdef THREAD_STACK_DISPLAY
    os_thread_stack_status(ctxt->end.p_tasks, ctxt->end.num);
#endif

#ifdef OS_MALLOC_DEBUG_DISPLAY
    os_mem_display_heap_malloc_info();
#endif

    os_acquire_mutex(ctxt->lock);

    os_mem_free(ctxt->start.p_tasks);
    ctxt->start = ctxt->end;

    os_release_mutex(ctxt->lock);

    lpm_info_display();
}

static void os_cpu_usage_timer_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    /* post event to share task to show CPU usage */
    iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_LP,
                              IOT_SHARE_EVENT_CPU_USAGE_EVENT);
    return;
}

void os_cpu_usage_util_start(uint32_t interval)
{
    cpu_usage_ctxt *ctxt = &g_cpu_usage_ctxt;

    os_acquire_mutex(ctxt->lock);

    os_cpu_usage_get_task_list(&ctxt->start);

    ctxt->interval = interval;

    /* start timer to collect CPU usage info */
    os_start_timer(ctxt->timer, interval);

    os_release_mutex(ctxt->lock);
}

void os_cpu_usage_util_stop(void)
{
    cpu_usage_ctxt *ctxt = &g_cpu_usage_ctxt;

    assert(ctxt->lock);

    os_acquire_mutex(ctxt->lock);

    ctxt->interval = 0;

    os_stop_timer(ctxt->timer);

    os_mem_free(ctxt->start.p_tasks);

    os_release_mutex(ctxt->lock);
}

void os_cpu_usage_util_init(void)
{
    cpu_usage_ctxt *ctxt = &g_cpu_usage_ctxt;

    ctxt->lock = os_create_mutex(OS_UTILS_MID);
    assert(ctxt->lock);

    timer_id_t timer =
        os_create_timer(OS_UTILS_MID, true, os_cpu_usage_timer_cb, ctxt);

    if (!timer) {
        DBGLOG_LIB_CPU_USAGE_ERROR("cpu usage timer create failed.\n");
        return;
    }

#ifdef CHECK_ISR_USAGE
    iot_isr_usage.last_display_time = iot_rtc_get_global_time_ms();
#endif

    iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP,
                                  IOT_SHARE_EVENT_CPU_USAGE_EVENT,
                                  os_status_display_handler, ctxt);

    ctxt->timer = timer;
}
