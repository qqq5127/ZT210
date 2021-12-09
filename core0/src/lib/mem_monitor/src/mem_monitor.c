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
#include "string.h"
#include "mem_monitor.h"
#include "critical_sec.h"

typedef struct mem_monitor_node {
    uint8_t node_used;
    void*   arg;
    uint8_t isr_corrupt;
    node_check_callback cb;
} mem_monitor_node_t;

static mem_monitor_node_t g_mon_node[MAX_MONITOR_NODE];

bool_t mem_monitor_register_node(uint8_t *out_node, void* arg, node_check_callback cb)
{
    cpu_critical_enter();
    uint8_t node_id;
    for (node_id = 0; node_id < MAX_MONITOR_NODE; node_id++) {
        if (!g_mon_node[node_id].node_used) {
            g_mon_node[node_id].node_used = 0x01;
            g_mon_node[node_id].arg = arg;
            g_mon_node[node_id].cb = cb;
            *out_node = node_id;
            cpu_critical_exit();
            return true;
        }
    }
    // all nodes used
    *out_node = 0xFF;
    cpu_critical_exit();
    return false;
}

bool_t mem_monitor_clear_node(uint8_t node)
{
    cpu_critical_enter();
    g_mon_node[node].node_used = 0x00;
    g_mon_node[node].cb = NULL;
    cpu_critical_exit();
    return true;
}

void mem_monitor_clear_all(void)
{
    cpu_critical_enter();
    memset(&g_mon_node[0], 0, sizeof(g_mon_node));
    cpu_critical_exit();
}

bool_t mem_monitor_process(uint8_t isr_corrupt) IRAM_TEXT(mem_monitor_process);
bool_t mem_monitor_process(uint8_t isr_corrupt) /*lint -esym(714, mem_monitor_process) symbol 'mem_monitor_process' called in mtvec */
{
    uint8_t node_id;
    for (node_id = 0; node_id < MAX_MONITOR_NODE; node_id++) {
        g_mon_node[node_id].isr_corrupt = isr_corrupt?0:1;
        if (g_mon_node[node_id].node_used && g_mon_node[node_id].cb) {
            if (!g_mon_node[node_id].cb(g_mon_node[node_id].arg)) {
                //memory corrupt detected here
                return false;
            }
        }
    }
    return true;
}
