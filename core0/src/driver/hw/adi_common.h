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

#ifndef DRIVER_HW_ADIE_REG_API_H
#define DRIVER_HW_ADIE_REG_API_H

#include "iot_memory_config.h"

#include "hw_reg_api.h"

#ifdef __cplusplus
extern "C" {
#endif


#if defined(BUILD_CORE_CORE0)
#define ANA_WRAP_A_REG_START_ADDR       DTOP_ANA_WRAP_A_REG_BASEADDR
#define ANA_WRAP_A_REG_REG_END_ADDR    (DTOP_ANA_WRAP_A_REG_BASEADDR+0x00d0)

// not used
#define APMM_PIN_REG_START_ADDR         APMM_PIN_REG_BASEADDR
#define APMM_PIN_REG_REG_END_ADDR       (APMM_PIN_REG_BASEADDR+0x10)

#define SDM_DEM_REG_START_ADDR          DTOP_SDM_DEM_REG_BASEADDR
#define SDM_DEM_REG_REG_END_ADDR        (DTOP_SDM_DEM_REG_BASEADDR+0x0024)
#elif defined(BUILD_CORE_CORE1)
#define ANA_WRAP_A_REG_START_ADDR       BCP_ANA_WRAP_REG_A_REG_BASEADDR
#define ANA_WRAP_A_REG_REG_END_ADDR     (BCP_ANA_WRAP_REG_A_REG_BASEADDR+0x00d0)

#define SDM_DEM_REG_START_ADDR          BCP_SDM_DEM_REG_BASEADDR
#define SDM_DEM_REG_REG_END_ADDR        (BCP_SDM_DEM_REG_BASEADDR+0x0024)
#elif defined(BUILD_CORE_DSP)
#define ANA_WRAP_A_REG_START_ADDR       AUD_ANA_WRAP_REG_A_REG_BASEADDR
#define ANA_WRAP_A_REG_REG_END_ADDR    (AUD_ANA_WRAP_REG_A_REG_BASEADDR+0x00d0)

#define SDM_DEM_REG_START_ADDR          AUD_SDM_DEM_REG_BASEADDR
#define SDM_DEM_REG_REG_END_ADDR        (AUD_SDM_DEM_REG_BASEADDR+0x0024)
#endif

typedef enum {
    ADI_WIRE_MODE_1,
    ADI_WIRE_MODE_2,
    ADI_WIRE_MODE_4,
    ADI_WIRE_MODE_MAX,
} ADI_WIRE_MODE;

#define ADI_A_REG_WR_REG_FIELD_(N, reg, ...)             \
        ({                                                   \
            typeof(reg) _reg_x_;                             \
            _reg_x_.w = adi_common_reg_read((uint32_t)&reg); \
            CONCATENATE(WR_FIELD_, N)(_reg_x_, __VA_ARGS__); \
            reg = _reg_x_;                                   \
        })

/**
 * WR_REG_FIELD(reg, field1, val1, field2, val2, field3, val3, ...)
*/
#define ADI_A_REG_WR_REG_FIELD(reg, ...) \
        ADI_A_REG_WR_REG_FIELD_(__REG_N_FIELD__(reg, __VA_ARGS__), reg, __VA_ARGS__)

#define ADI_A_REG_RE_REG_FIELD(reg, bitfield)   \
        ({                                          \
            typeof(reg) __x;                        \
            __x.w = adi_common_reg_read((uint32_t)&reg); \
            (uint32_t) __x._b.bitfield;             \
        })

/* the api is the war for the issue of
 * adie reg read conflict to the meter adc
 * so, at least twice time read for the adie reg,
 * to ignore the error value when conflict happend,
 * make sure there are same value from the read opertion */
uint32_t adi_common_reg_read(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_ADIE_REG_API_H */
