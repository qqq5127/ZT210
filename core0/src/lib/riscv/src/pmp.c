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

#include "types.h"
#include "riscv_cpu.h"

#include "pmp.h"

#define CONFIG_TO_INT(_config) (*((char *)&(_config)))
#define INT_TO_CONFIG(_int)    (*((struct pmp_config *)(char *)&(_int)))

struct pmp __dt_pmp;

struct pmp *pmp_get_device(void)
{
    return &__dt_pmp;
}

/* This function calculates the minimum granularity from the address
 * that pmpaddr takes on after writing all ones to pmpaddr when pmpcfg = 0.
 *
 * Detect the address granularity based on the position of the
 * least-significant 1 set in the address.
 *
 * For example, if the value read from pmpaddr is 0x3ffffc00, the
 * least-significant set bit is in bit 10 (counting from 0), resulting
 * in a detected granularity of 2^(10 + 2) = 4096.
 */
static uintptr_t _get_detected_granularity(uintptr_t address)
{
    if (address == 0) {
        return (uintptr_t)-1;
    }

    /* Get the index of the least significant set bit */
    int32_t index = 0;
    while (((address >> index) & 0x1) == 0) {
        index += 1;
    }

    /* The granularity is equal to 2^(index + 2) bytes */
    return BIT(index + 2);
}

/* This function calculates the granularity requested by the user's provided
 * value for pmpaddr.
 *
 * Calculate the requested granularity based on the position of the
 * least-significant unset bit.
 *
 * For example, if the requested address is 0x20009ff, the least-significant
 * unset bit is at index 9 (counting from 0), resulting in a requested
 * granularity of 2^(9 + 3) = 4096.
 */
static uintptr_t _get_pmpaddr_granularity(uintptr_t address)
{
    /* Get the index of the least significant unset bit */
    int32_t index = 0;
    while (((address >> index) & 0x1) == 1) {
        index += 1;
    }

    /* The granularity is equal to 2^(index + 3) bytes */
    return BIT(index + 3);
}

/* Get the number of pmp regions for the given hart */
uint32_t pmp_num_regions(int32_t hartid)
{
    UNUSED(hartid);

    return 4;
}

/* Get the number of pmp regions for the current hart */
static uint32_t _pmp_regions(void)
{
    return pmp_num_regions((int32_t)cpu_get_mhartid());
}

void pmp_init(struct pmp *pmp)
{
    static uint8_t init = 0;

    if (init) {
       return;
    }

    init = 1;

    if (!pmp) {
        return;
    }

    struct pmp_config init_config = {
        .L = PMP_UNLOCKED,
        .A = PMP_OFF,
        .X = 0,
        .W = 0,
        .R = 0,
    };

    for (uint32_t i = 0; i < _pmp_regions(); i++) {
        pmp_set_region(pmp, i, init_config, 0);
    }

    /* Detect the region granularity by writing all 1s to pmpaddr0 while
     * pmpcfg0 = 0. */
    if (pmp_set_address(pmp, 0, (uint32_t)-1) != 0) {
        /* Failed to detect granularity */
        return;
    }

    /* Calculate the granularity based on the value that pmpaddr0 takes on */
    pmp->_granularity[cpu_get_mhartid()] =
        _get_detected_granularity(pmp_get_address(pmp, 0));

    /* Clear pmpaddr0 */
    pmp_set_address(pmp, 0, 0);
}

int32_t pmp_set_region(const struct pmp *pmp, uint32_t region,
                       struct pmp_config config, size_t address)
{
    struct pmp_config old_config;
    size_t old_address;
    size_t cfgmask;
    size_t pmpcfg;
    int32_t rc;

    if (!pmp) {
        /* Device handle cannot be NULL */
        return 1;
    }

    if (region > _pmp_regions()) {
        /* Region outside of supported range */
        return 2;
    }

    if ((uint8_t)config.A == PMP_NA4 && pmp->_granularity[cpu_get_mhartid()] > 4) {
        /* The requested granularity is too small */
        return 3;
    }

    if ((uint8_t)config.A == PMP_NAPOT
        && pmp->_granularity[cpu_get_mhartid()]
            > _get_pmpaddr_granularity(address)) {
        /* The requested granularity is too small */
        return 3;
    }

    rc = pmp_get_region(pmp, region, &old_config, &old_address);
    if (rc) {
        /* Error reading region */
        return rc;
    }

    if ((uint8_t)old_config.L == PMP_LOCKED) {
        /* Cannot modify locked region */
        return 4;
    }

    /* Update the address first, because if the region is being locked we won't
     * be able to modify it after we set the config */
    if (old_address != address) {
        switch (region) {
            case 0:
                __asm__("csrw pmpaddr0, %[addr]" ::[addr] "r"(address) :);
                break;
            case 1:
                __asm__("csrw pmpaddr1, %[addr]" ::[addr] "r"(address) :);
                break;
            case 2:
                __asm__("csrw pmpaddr2, %[addr]" ::[addr] "r"(address) :);
                break;
            case 3:
                __asm__("csrw pmpaddr3, %[addr]" ::[addr] "r"(address) :);
                break;
            case 4:
                __asm__("csrw pmpaddr4, %[addr]" ::[addr] "r"(address) :);
                break;
            case 5:
                __asm__("csrw pmpaddr5, %[addr]" ::[addr] "r"(address) :);
                break;
            case 6:
                __asm__("csrw pmpaddr6, %[addr]" ::[addr] "r"(address) :);
                break;
            case 7:
                __asm__("csrw pmpaddr7, %[addr]" ::[addr] "r"(address) :);
                break;
            case 8:
                __asm__("csrw pmpaddr8, %[addr]" ::[addr] "r"(address) :);
                break;
            case 9:
                __asm__("csrw pmpaddr9, %[addr]" ::[addr] "r"(address) :);
                break;
            case 10:
                __asm__("csrw pmpaddr10, %[addr]" ::[addr] "r"(address) :);
                break;
            case 11:
                __asm__("csrw pmpaddr11, %[addr]" ::[addr] "r"(address) :);
                break;
            case 12:
                __asm__("csrw pmpaddr12, %[addr]" ::[addr] "r"(address) :);
                break;
            case 13:
                __asm__("csrw pmpaddr13, %[addr]" ::[addr] "r"(address) :);
                break;
            case 14:
                __asm__("csrw pmpaddr14, %[addr]" ::[addr] "r"(address) :);
                break;
            case 15:
                __asm__("csrw pmpaddr15, %[addr]" ::[addr] "r"(address) :);
                break;
            default:
                break;
        }
    }

#if __riscv_xlen == 32
    if (CONFIG_TO_INT(old_config) != CONFIG_TO_INT(config)) {
        /* Mask to clear old pmpcfg */
        cfgmask = (0xFFU << (8 * (region % 4)));
        pmpcfg = ((size_t)CONFIG_TO_INT(config) << (8 * (region % 4)));

        // lint can not recognize the variables used in assembler syntax
        UNUSED(cfgmask);
        UNUSED(pmpcfg);

        switch (region / 4) {
            case 0:
                __asm__("csrc pmpcfg0, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg0, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            case 1:
                __asm__("csrc pmpcfg1, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg1, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            case 2:
                __asm__("csrc pmpcfg2, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg2, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            case 3:
                __asm__("csrc pmpcfg3, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg3, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            default:
                break;
        }
    }
#elif __riscv_xlen == 64
    if (CONFIG_TO_int32_t(old_config) != CONFIG_TO_int32_t(config)) {
        /* Mask to clear old pmpcfg */
        cfgmask = (0xFF << (8 * (region % 8)));
        pmpcfg = (CONFIG_TO_int32_t(config) << (8 * (region % 8)));

        switch (region / 8) {
            case 0:
                __asm__("csrc pmpcfg0, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg0, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            case 1:
                __asm__("csrc pmpcfg2, %[mask]" ::[mask] "r"(cfgmask) :);

                __asm__("csrs pmpcfg2, %[cfg]" ::[cfg] "r"(pmpcfg) :);
                break;
            default:
                break;
        }
    }
#else
#error XLEN is not set to supported value for PMP driver
#endif

    return 0;
}

int32_t pmp_get_region(const struct pmp *pmp, uint32_t region,
                       struct pmp_config *config, size_t *address)
{
    size_t pmpcfg = 0;
    char *pmpcfg_convert = (char *)&pmpcfg;

    if (!pmp || !config || !address) {
        /* NULL point32_ters are invalid arguments */
        return 1;
    }

    if (region > _pmp_regions()) {
        /* Region outside of supported range */
        return 2;
    }

    // lint can not recognize the variables used in assembler syntax
    UNUSED(address);
#if __riscv_xlen == 32
    switch (region / 4) {
        case 0:
            __asm__("csrr %[cfg], pmpcfg0" : [cfg] "=r"(pmpcfg)::);
            break;
        case 1:
            __asm__("csrr %[cfg], pmpcfg1" : [cfg] "=r"(pmpcfg)::);
            break;
        case 2:
            __asm__("csrr %[cfg], pmpcfg2" : [cfg] "=r"(pmpcfg)::);
            break;
        case 3:
            __asm__("csrr %[cfg], pmpcfg3" : [cfg] "=r"(pmpcfg)::);
            break;
        default:
            break;
    }

    pmpcfg = (0xFF & (pmpcfg >> (8 * (region % 4))));

#elif __riscv_xlen == 64
    switch (region / 8) {
        case 0:
            __asm__("csrr %[cfg], pmpcfg0" : [cfg] "=r"(pmpcfg)::);
            break;
        case 1:
            __asm__("csrr %[cfg], pmpcfg2" : [cfg] "=r"(pmpcfg)::);
            break;
        default:
            break;
    }

    pmpcfg = (0xFF & (pmpcfg >> (8 * (region % 8))));

#else
#error XLEN is not set to supported value for PMP driver
#endif

    *config = INT_TO_CONFIG(*pmpcfg_convert);

    switch (region) {
        case 0:
            __asm__("csrr %[addr], pmpaddr0" : [addr] "=r"(*address)::);
            break;
        case 1:
            __asm__("csrr %[addr], pmpaddr1" : [addr] "=r"(*address)::);
            break;
        case 2:
            __asm__("csrr %[addr], pmpaddr2" : [addr] "=r"(*address)::);
            break;
        case 3:
            __asm__("csrr %[addr], pmpaddr3" : [addr] "=r"(*address)::);
            break;
        case 4:
            __asm__("csrr %[addr], pmpaddr4" : [addr] "=r"(*address)::);
            break;
        case 5:
            __asm__("csrr %[addr], pmpaddr5" : [addr] "=r"(*address)::);
            break;
        case 6:
            __asm__("csrr %[addr], pmpaddr6" : [addr] "=r"(*address)::);
            break;
        case 7:
            __asm__("csrr %[addr], pmpaddr7" : [addr] "=r"(*address)::);
            break;
        case 8:
            __asm__("csrr %[addr], pmpaddr8" : [addr] "=r"(*address)::);
            break;
        case 9:
            __asm__("csrr %[addr], pmpaddr9" : [addr] "=r"(*address)::);
            break;
        case 10:
            __asm__("csrr %[addr], pmpaddr10" : [addr] "=r"(*address)::);
            break;
        case 11:
            __asm__("csrr %[addr], pmpaddr11" : [addr] "=r"(*address)::);
            break;
        case 12:
            __asm__("csrr %[addr], pmpaddr12" : [addr] "=r"(*address)::);
            break;
        case 13:
            __asm__("csrr %[addr], pmpaddr13" : [addr] "=r"(*address)::);
            break;
        case 14:
            __asm__("csrr %[addr], pmpaddr14" : [addr] "=r"(*address)::);
            break;
        case 15:
            __asm__("csrr %[addr], pmpaddr15" : [addr] "=r"(*address)::);
            break;
        default:
            break;
    }

    return 0;
}

int32_t pmp_lock(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &address);
    if (rc) {
        return rc;
    }

    if ((uint8_t)config.L == PMP_LOCKED) {
        return 0;
    }

    config.L = PMP_LOCKED;

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

int32_t pmp_set_address(const struct pmp *pmp, uint32_t region, size_t address)
{
    struct pmp_config config;
    size_t old_address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &old_address);
    if (rc) {
        return rc;
    }

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

size_t pmp_get_address(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address = 0;

    pmp_get_region(pmp, region, &config, &address);

    return address;
}

int32_t pmp_set_address_mode(const struct pmp *pmp, uint32_t region,
                             enum pmp_address_mode mode)
{
    struct pmp_config config;
    size_t address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &address);
    if (rc) {
        return rc;
    }

    config.A = mode;

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

enum pmp_address_mode pmp_get_address_mode(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address = 0;

    pmp_get_region(pmp, region, &config, &address);

    return config.A;
}

int32_t pmp_set_executable(const struct pmp *pmp, uint32_t region, int32_t X)
{
    struct pmp_config config;
    size_t address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &address);
    if (rc) {
        return rc;
    }

    config.X = (uint32_t)X;

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

int32_t pmp_get_executable(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address = 0;

    pmp_get_region(pmp, region, &config, &address);

    return config.X;
}

int32_t pmp_set_writeable(const struct pmp *pmp, uint32_t region, int32_t W)
{
    struct pmp_config config;
    size_t address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &address);
    if (rc) {
        return rc;
    }

    config.W = (uint32_t)W;

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

int32_t pmp_get_writeable(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address = 0;

    pmp_get_region(pmp, region, &config, &address);

    return config.W;
}

int32_t pmp_set_readable(const struct pmp *pmp, uint32_t region, int32_t R)
{
    struct pmp_config config;
    size_t address;
    int32_t rc;

    rc = pmp_get_region(pmp, region, &config, &address);
    if (rc) {
        return rc;
    }

    config.R = (uint32_t)R;

    rc = pmp_set_region(pmp, region, config, address);

    return rc;
}

int32_t pmp_get_readable(const struct pmp *pmp, uint32_t region)
{
    struct pmp_config config;
    size_t address = 0;

    pmp_get_region(pmp, region, &config, &address);

    return config.R;
}
