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

#include "encoding.h"
#include "iot_trace.h"

#define TDATA1_TYPE_MATCH_CONTROL 2

enum {
    MATCHCTL_SELECT_ADDR,
    MATCHCTL_SELECT_DATA,
};

/*lint -esym(749, MATCHCTL_ACTION_DEBUGMODE) */
enum {
    MATCHCTL_ACTION_BREAKPOINT,
    MATCHCTL_ACTION_DEBUGMODE,
};

/*lint -esym(749, MATCHCTL_MATCH_GREAT_EQL, MATCHCTL_MATCH_LESS) */
enum {
    MATCHCTL_MATCH_EQUAL = 0,
    MATCHCTL_MATCH_GREAT_EQL = 2,
    MATCHCTL_MATCH_LESS = 3,
};

/*
 * trigger_data1 Base Type
 *
 * typedef union {
 *     uint32_t w;
 *     struct {
 *         uint32_t reserved:27;
 *         uint32_t dmode:1;
 *         uint32_t type:4;
 *     } b;
 * } trigger_data1_u;
 *
 */

typedef union {
    uint32_t w;
    /*lint -esym(754, match_control_bit::*) */
    struct match_control_bit {
        uint32_t load:1;
        uint32_t store:1;
        uint32_t execute:1;
        uint32_t u:1;
        uint32_t s:1;
        uint32_t h:1;
        uint32_t m:1;
        uint32_t match:4;
        uint32_t chain:1;
        uint32_t action:4;
        uint32_t sizelo:2;
        uint32_t timing:1;
        uint32_t select:1;
        uint32_t hit:1;
        uint32_t maskmax:6;
        uint32_t dmode:1;
        uint32_t type:4;
    } b;
} match_control_u;

// beetle only supports index = 0/1
static uint32_t index = 0;

static int trigger_config(unsigned long ctxt, bool_t is_chain,
                          bool_t is_data, uint32_t match)
{
    uint32_t ret;
    match_control_u match_control;

    write_csr(tselect, index);
    ret = read_csr(tselect);
    if (ret != index) {
        return RET_FAIL;
    }

    match_control.w = read_csr(tdata1);
    if (match_control.b.type != TDATA1_TYPE_MATCH_CONTROL) {
        return RET_FAIL;
    }

    match_control.b.select = is_data ? MATCHCTL_SELECT_DATA : MATCHCTL_SELECT_ADDR;
    match_control.b.timing = 0;
    match_control.b.sizelo = 0;
    match_control.b.action = MATCHCTL_ACTION_BREAKPOINT;
    match_control.b.chain = (uint32_t)is_chain;
    match_control.b.match = match;
    match_control.b.m = 1;
    match_control.b.h = 0;
    match_control.b.s = 0;
    match_control.b.u = 0;
    match_control.b.execute = 0;
    match_control.b.store = 1;
    match_control.b.load = 0;

    write_csr(tdata1, match_control.w);
    write_csr(tdata2, ctxt);

    index++;

    return RET_OK;
}

int iot_trigger_watch_detect(void *addr)
{
    return trigger_config((unsigned long)addr, false, false, MATCHCTL_MATCH_EQUAL);
}

int iot_trigger_watch_val_range(void *addr, uint32_t val, enum IOT_TRACE_MATCH_OPTION option)
{
    int ret;
    uint32_t match = MATCHCTL_MATCH_EQUAL;

    ret = trigger_config((unsigned long)addr, true, false, match);
    if (ret != RET_OK)
        return RET_FAIL;

    switch (option) {
        case IOT_TRACE_MATCH_EQUAL:
            match = MATCHCTL_MATCH_EQUAL;
            break;

#ifdef IOT_TRACE_NOTEQUAL
        case IOT_TRACE_MATCH_GREAT_EQUAL:
            match = MATCHCTL_MATCH_GREAT_EQL;
            break;

        case IOT_TRACE_MATCH_LESS:
            match = MATCHCTL_MATCH_LESS;
            break;
#endif

        default:
            break;
    }

    return trigger_config((unsigned long)val, false, true, match);
}
