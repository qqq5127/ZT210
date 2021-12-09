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
#include "iot_irq.h"
#include "iot_wdt.h"

#include "riscv_cpu.h"
#include "exception.h"
#include "wdt.h"
#include "pmm.h"
#include "apb.h"

#include "driver_dbglog.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

/*lint -esym(754, iot_wdt_info_t::reserved) */
/*lint -esym(754, iot_wdt_info_t::reserved2) */
#define IOT_WDT_MAGIC_NUM 0xdeadbeef
typedef struct iot_wdt_info_t {
    iot_irq_t feed_irq;
    iot_irq_t timeout_irq;
    uint32_t feed_cnt;
    volatile uint8_t need_feed;
    uint8_t wdt_id;
    uint8_t timeout:1,
        reserved:7;
    uint8_t reserved2;
} iot_wdt_info;

static iot_wdt_info g_wdt_info[WATCHDOG_MAX_NUM];
static uint8_t wdt_get_used_id(void);

#ifdef LOW_POWER_ENABLE
static uint32_t iot_wdt_pm_save(uint32_t data)
{
    UNUSED(data);

    return RET_OK;
}

static uint32_t iot_wdt_pm_restore(uint32_t data)
{
    UNUSED(data);

    uint32_t cpu = cpu_get_mhartid();

    iot_wdt_init();

    if(cpu == 1 || cpu ==2 ){
        /* Now bt sys only use WDG1 */
        wdt_deinit(WATCHDOG2);
    }

    return RET_OK;
}

static struct pm_operation wdt_pm = {
    .node = LIST_HEAD_INIT(wdt_pm.node),
    .save = iot_wdt_pm_save,
    .restore= iot_wdt_pm_restore,
};
#endif

static uint8_t wdt_get_used_id(void)
{
    uint32_t cpu = cpu_get_mhartid();
    if (cpu == 0) {
        return WATCHDOG0;
    } else if (cpu == 1 || cpu == 2) {
        return WATCHDOG1;
    } else {
        return WATCHDOG4;
    }
}

static void wdt_global_disable_feed(void) IRAM_TEXT(wdt_global_disable_feed);
static void wdt_global_disable_feed(void)
{
    /* set magic num and disable global feeddog */
    apb_set_scratch1_register(IOT_WDT_MAGIC_NUM);
}

static uint32_t wdt_feed_isr_handler(uint32_t vector, iot_addrword_t data) IRAM_TEXT(wdt_feed_isr_handler);
static uint32_t wdt_feed_isr_handler(uint32_t vector, iot_addrword_t data)
{
    UNUSED(vector);

    iot_wdt_info *wdt_info = (iot_wdt_info *)data;
    wdt_info->need_feed = 1;

    iot_irq_mask(wdt_info->feed_irq);

    if (wdt_info->wdt_id == wdt_get_global_id()) {
        if (apb_get_scratch1_register() != IOT_WDT_MAGIC_NUM) {
            iot_wdt_global_do_feed();
        }
    }

    return 0;
}

static uint32_t wdt_timeout_isr_handler(uint32_t vector, iot_addrword_t data) IRAM_TEXT(wdt_timeout_isr_handler);
static uint32_t wdt_timeout_isr_handler(uint32_t vector, iot_addrword_t data)
{
    UNUSED(vector);
    iot_wdt_info *wdt_info = (iot_wdt_info *)data;

    iot_irq_mask(wdt_info->timeout_irq);

    wdt_disable_cpu_reset(wdt_info->wdt_id);

#if defined(BUILD_CORE_CORE0)
    if (wdt_info->wdt_id == WATCHDOG1 ||
        wdt_info->wdt_id == WATCHDOG2 ||
        wdt_info->wdt_id == WATCHDOG3) {
            //DBGLOG_DRIVER_WARNING("Watchdog %d timeout, and disable global watchdog.\n", wdt_info->wdt_id);
            /* disable global watchdog feed */
            wdt_global_disable_feed();

            wdt_info->timeout = 1;

            return 0;
    }
#endif
    //DBGLOG_DRIVER_WARNING("Watchdog %d timeout.\n", wdt_info->wdt_id);

    extern void exception(void);
    exception();

    return 0;
}

static void wdt_feed_irq_init(uint8_t id)
{
    uint32_t feed_irq_vector = wdt_get_feed_irq_vector(id);

    /* register feed and timeout irq */
    g_wdt_info[id].feed_irq = iot_irq_create(
        feed_irq_vector, (uint32_t)&g_wdt_info[id], wdt_feed_isr_handler);

    /* turn on wdt's irq */
    iot_irq_unmask(g_wdt_info[id].feed_irq);

    /* mark the wdt id*/
    g_wdt_info[id].wdt_id = id;
}

static void wdt_timeout_irq_init(uint8_t id)
{
    uint32_t timeout_irq_vector = wdt_get_timeout_irq_vector(id);

    g_wdt_info[id].timeout_irq = iot_irq_create(
        timeout_irq_vector, (uint32_t)&g_wdt_info[id], wdt_timeout_isr_handler);

    /* turn on wdt's irq */
    iot_irq_unmask(g_wdt_info[id].timeout_irq);

    /* mark the wdt id*/
    g_wdt_info[id].wdt_id = id;
}

static void wdt_irq_deinit(uint8_t id)
{
    /* turn off wdt's irq */
    iot_irq_mask(g_wdt_info[id].feed_irq);
    iot_irq_mask(g_wdt_info[id].timeout_irq);

    /* release irq resource */
    iot_irq_delete(g_wdt_info[id].feed_irq);
    iot_irq_delete(g_wdt_info[id].timeout_irq);

    g_wdt_info[id].feed_irq = 0;
    g_wdt_info[id].timeout_irq = 0;
}

void iot_wdt_init(void)
{
    uint8_t id = wdt_get_used_id();

    /* wdt hw init */
    wdt_init(id);

    /* feeddog irq attach */
    wdt_feed_irq_init(id);

#if defined(BUILD_CORE_CORE0)
    /* ALL DTOP watchdog timeout IRQ attach to CORE0 */
    wdt_timeout_irq_init(WATCHDOG0);
    wdt_timeout_irq_init(WATCHDOG1);
    wdt_timeout_irq_init(WATCHDOG2);
    wdt_timeout_irq_init(WATCHDOG3);
#else
    wdt_timeout_irq_init(id);
#endif

#if defined(LOW_POWER_ENABLE)
    iot_dev_pm_register(&wdt_pm);
#endif
}

void iot_wdt_deinit(void)
{
    uint8_t id = wdt_get_used_id();

    /* wdt hw deinit */
    wdt_deinit(id);

    /* IRQ deattach */
    wdt_irq_deinit(id);
}

void iot_wdt_set_feed_period(uint32_t period)
{
    uint8_t id = wdt_get_used_id();
    wdt_set_feed_period(id, period);
}

bool_t iot_wdt_need_feed(void)
{
    uint8_t id = wdt_get_used_id();

    return !!g_wdt_info[id].need_feed;
}

void iot_wdt_do_feed(void)
{
    uint8_t id = wdt_get_used_id();

    g_wdt_info[id].need_feed = 0;
    g_wdt_info[id].feed_cnt++;
    wdt_feed(id);

    iot_irq_unmask(g_wdt_info[id].feed_irq);
}

void iot_wdt_do_reset(void)
{
    uint8_t id = wdt_get_used_id();

    wdt_init(id);

    wdt_reset(id);
}

void iot_wdt_global_init(void)
{
    uint8_t id = wdt_get_global_id();

    /* global wdt hw init*/
    wdt_init(id);

    /* global watchdog is chip reset by default */
    pmm_set_wdg_reset_mode(true);

    /* IRQ attach*/
    wdt_feed_irq_init(id);

    wdt_timeout_irq_init(id);
}

void iot_wdt_global_deinit(void)
{
    uint8_t id = wdt_get_global_id();

    /* global wdt hw init*/
    wdt_deinit(id);

    /* IRQ deattach*/
    wdt_irq_deinit(id);
}

void iot_wdt_global_set_feed_period(uint32_t period)
{
    uint8_t id = wdt_get_global_id();

    wdt_set_feed_period(id, period);
}

bool_t iot_wdt_global_need_feed(void)
{
    uint8_t id = wdt_get_global_id();

    return !!g_wdt_info[id].need_feed;
}

void iot_wdt_global_do_feed(void) IRAM_TEXT(iot_wdt_global_do_feed);
void iot_wdt_global_do_feed(void)
{
    uint8_t id = wdt_get_global_id();

    g_wdt_info[id].need_feed = 0;
    g_wdt_info[id].feed_cnt++;

    wdt_feed(id);

    iot_irq_unmask(g_wdt_info[id].feed_irq);
}

void iot_wdt_global_do_reset(bool_t chip_reset)
{
    uint8_t id = wdt_get_global_id();

    /* global wdt hw init*/
    wdt_init(id);

    pmm_set_wdg_reset_mode(chip_reset);

    /* disable global feeddog */
    wdt_global_disable_feed();

    wdt_reset(id);
}

uint32_t iot_wdt_global_get_feed_cnt(void)
{
    uint8_t id = wdt_get_global_id();

    return g_wdt_info[id].feed_cnt;
}

void iot_wdt_enable(void)
{
    iot_wdt_init();
}

void iot_wdt_disable(void)
{
    iot_wdt_deinit();
}

void iot_wdt_global_enable(void)
{
    iot_wdt_global_init();
}

void iot_wdt_global_disable(void)
{
    iot_wdt_global_deinit();
}

void iot_wdt_disable_all(void)
{
    for(uint8_t id = 0; id < WATCHDOG_MAX_NUM; id++){
        wdt_deinit(id);
    }
}

uint32_t iot_wdt_get_feed_cnt(void)
{
    uint8_t id = wdt_get_used_id();

    return g_wdt_info[id].feed_cnt;
}

bool_t iot_wdt_is_timeout(uint8_t *id)
{
    for(uint8_t i = 0; i < WATCHDOG_MAX_NUM; i++){
        if (g_wdt_info[i].timeout) {
            *id = i;
            return true;
        }
    }

    return false;
}

void iot_wdt_dump_reg(void)
{
    uint32_t cnt, cmp;
    uint32_t timeout, cpu_reset, full_reset;
    uint32_t feed_cnt;

    for (uint8_t i = 0; i < WATCHDOG_MAX_NUM; i++) {
        cmp = wdt_get_cmp(i);
        cnt = wdt_get_cnt(i);
        wdg_get_int_reset_cmp(i, &timeout, &cpu_reset, &full_reset);

        DBGLOG_DRIVER_INFO("WDT%d: cmp:%d cnt:%d reset timeout:%d, cpu_reset:%d, full_reset:%d\n",
                         i, cmp, cnt, timeout, cpu_reset, full_reset);
    }

    feed_cnt = iot_wdt_get_feed_cnt();
    DBGLOG_DRIVER_INFO("WDT feed cnt:%d\n", feed_cnt);

    if (wdt_get_used_id() == WATCHDOG0) {
        feed_cnt = iot_wdt_global_get_feed_cnt();
        DBGLOG_DRIVER_INFO("PMM WDT feed cnt:%d\n", feed_cnt);
    }

}
