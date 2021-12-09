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
#include "riscv_cpu.h"
#include "exception.h"
#include "encoding.h"

#ifdef CHECK_ISR_FLASH
#include "iot_soc.h"
#endif

exception_dump_callback exception_callback = NULL;

static uint32_t ebreak_handler(exception_info_t *info)
{
    UNUSED(info);
    return RET_FAIL;
}

void exception(void) IRAM_TEXT(exception);
void exception(void)
{
    volatile uint32_t forever = 1;
    uint32_t ret = RET_FAIL;
#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
    exception_info_t *info = cpu_get_exception_data();

    /* memory trace hit */
    if ((info->mcause & MCAUSE_CAUSE) == CAUSE_BREAKPOINT) {
        ret = ebreak_handler(info);
        if (ret == RET_OK) {
            return;
        }
    }

    /* ret != RET_OK */
    if (exception_callback != NULL) {
        exception_callback(info);
    }

    while (forever){
    }
}

void exception_register_dump_callback(exception_dump_callback cb)
{
    exception_callback = cb;
}

void exception_default_register(void)
{
    cpu_register_exception_handler(exception);
}
