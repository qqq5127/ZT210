#include "iot_soc.h"
#include "os_task.h"
#include "os_timer.h"
#include "riscv_cpu.h"
#include "iot_suspend_sched.h"
#include "iot_timer.h"
#include "apb.h"

// only available in caller side
static volatile uint32_t max_interval = 0;
static volatile uint32_t suspend_ts = 0;
static volatile uint32_t resume_ts = 0;
static volatile uint32_t ref_count = 0;

static os_task_h suspend_sched_task;

static int32_t ipc_task_suspend_handler(IPC_CORES src_cpu, const void *payload) IRAM_TEXT(ipc_task_suspend_handler);
static int32_t ipc_task_suspend_handler(IPC_CORES src_cpu, const void *payload)
{
    UNUSED(src_cpu);
    UNUSED(payload);

    iot_soc_inc_task_suspend_vote();
    os_set_task_event(suspend_sched_task);
    return 0;
}

void iot_suspend_sched_std_handler(void) IRAM_TEXT(iot_suspend_sched_std_handler);
void iot_suspend_sched_std_handler(void)
{
#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, false);
#endif

    iot_soc_set_task_suspend_vote();

    while (iot_soc_get_task_suspend_vote() != 0) {
    }

#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
}

static void iot_suspend_sched(void *arg) IRAM_TEXT(iot_suspend_sched);
static void iot_suspend_sched(void *arg)
{
    UNUSED(arg);

    while (1) {     //lint !e716 task main loop
        os_wait_task_event();

        os_task_suspend_all();

        apb_btb_enable((uint8_t)cpu_get_mhartid(), false);

        iot_suspend_sched_std_handler();

        apb_btb_enable((uint8_t)cpu_get_mhartid(), true);

        os_task_resume_all();
    }
}

int32_t iot_suspend_sched_init(void)
{
    iot_ipc_register_handler(IPC_TYPE_TASK_SUSPEND, ipc_task_suspend_handler);
    suspend_sched_task = os_create_task_ext(iot_suspend_sched, NULL, OS_TASK_PRIO_HIGHEST, 128, "suspend_sched_task");

    return RET_OK;
}

static void iot_suspend_sched_request_core_suspend(IPC_CORES core) IRAM_TEXT(iot_suspend_sched_request_core_suspend);
static void iot_suspend_sched_request_core_suspend(IPC_CORES core)
{
    int32_t ret = RET_INVAL;
    for (;;) {
        ret = iot_ipc_send_message(core, IPC_TYPE_TASK_SUSPEND, NULL, 0, false);
        if(ret == RET_OK)
            break;
    }
}

void iot_suspend_sched_wait_core_suspend(IPC_CORES core) IRAM_TEXT(iot_suspend_sched_wait_core_suspend);
void iot_suspend_sched_wait_core_suspend(IPC_CORES core)
{
    iot_suspend_sched_request_core_suspend(core);

    if (suspend_ts == 0) {
        suspend_ts = iot_timer_get_time();
    }

    for (;;) {
        if (iot_soc_get_task_suspend_vote()) {
            break;
        }
    }
}

void iot_suspend_sched_notify_core_resume(IPC_CORES core) IRAM_TEXT(iot_suspend_sched_notify_core_resume);
void iot_suspend_sched_notify_core_resume(IPC_CORES core)
{
    UNUSED(core);

    ref_count = iot_soc_clear_task_suspend_vote();

    if (ref_count == 0) {
        resume_ts = iot_timer_get_time();

        uint32_t interval = resume_ts - suspend_ts;
        // Record MAX interval of suspend task
        if (interval > max_interval) {
            max_interval = interval;
        }

        suspend_ts = 0;
    }
}
