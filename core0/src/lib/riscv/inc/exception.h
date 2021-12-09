/****************************************************************************
 *
 * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated into another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 *
 * ****************************************************************************/

#ifndef _LIB_RISCV_EXCEPTION_H
#define _LIB_RISCV_EXCEPTION_H

#include "riscv_cpu.h"

typedef struct trap_stack_registers {
    uint32_t mepc;
    uint32_t flr[32];
    uint32_t ra;
    uint32_t t0_2[3];
    uint32_t fp;
    uint32_t s1;
    uint32_t a0_7[8];
    uint32_t s2_11[10];
    uint32_t t3_6[4];
    uint32_t mstatus;
} trap_stack_registers_t;

typedef void (*exception_dump_callback)(const exception_info_t *info);

void exception(void);
void exception_register_dump_callback(exception_dump_callback cb);
void exception_default_register(void);

#endif
