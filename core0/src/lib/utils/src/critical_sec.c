#include "critical_sec.h"
#include "types.h"
#include "riscv_cpu.h"

#if defined(BUILD_OS_NON_OS) || defined(BUILD_OS_FREERTOS)

uint32_t critical_mask = 0;
uint32_t critical_nest = 0;

void cpu_critical_enter(void) IRAM_TEXT(cpu_critical_enter);
void cpu_critical_enter(void)
{
    uint32_t mask = cpu_disable_irq();

    if (!critical_nest) {
        critical_mask = mask;
    }

    critical_nest++;
}

void cpu_critical_exit(void) IRAM_TEXT(cpu_critical_exit);
void cpu_critical_exit(void)
{
    assert(critical_nest);

    critical_nest--;

    if (!critical_nest) {
        cpu_restore_irq(critical_mask);
    }
}

#else

#include "os_lock.h"

void cpu_critical_enter(void)
{
    os_critical_enter();
}

void cpu_critical_exit(void)
{
    os_critical_exit();
}

#endif
