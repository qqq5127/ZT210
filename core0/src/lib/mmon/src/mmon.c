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
#include "pmp.h"

#include "mmon.h"

bool_t mmon_disable_address_access(uint32_t start, uint32_t size)
{
    int rc;
    struct pmp *pmp;

    /* PMP addresses are 4-byte aligned, drop the bottom two bits */
    size_t protected_addr = ((size_t)start) >> 2;

    /* Clear the bit corresponding with alignment */
    protected_addr &= ~(size >> 3);

    /* Set the bits up to the alignment bit */
    protected_addr |= ((size >> 3) - 1);

    /* Initialize PMPs */
    pmp = pmp_get_device();

    if (!pmp) {
        return false;
    }

    /* pmp init */
    pmp_init(pmp);

    /* Configure PMP 0 to only allow reads to protected_global. The
    * PMP region is locked so that the configuration applies to M-mode
    * accesses. */
    struct pmp_config config = {
        .L = PMP_LOCKED,
        .A = PMP_NAPOT, /* Naturally-aligned power of two */
        .X = 0,
        .W = 0,
        .R = 0,
    };

    rc = pmp_set_region(pmp, 0, config, protected_addr);
    if (rc != 0) {
        return false;
    }

    return true;
}

bool_t mmon_enable_address_access(uint32_t start, uint32_t size)
{
    UNUSED(start);
    UNUSED(size);
    return true;
}
