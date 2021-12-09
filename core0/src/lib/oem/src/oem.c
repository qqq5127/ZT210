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
/* common includes */
#include "types.h"
#include "string.h"
#include "crc.h"

#include "iot_memory_origin.h"

#include "oem.h"

oem_data_t *oem_data_load(void)
{
    return (oem_data_t *)(FLASH_START + FLASH_OEM_DATA_OFFSET);
}

oem_data_anc_t *anc_data_load(void)
{
    return (oem_data_anc_t *)(FLASH_START + FLASH_ANC_DATA_OFFSET);
}

uint8_t oem_data_base_load(uint8_t **data, uint32_t *len)
{
    oem_data_t *oem_data = oem_data_load();
    oem_data_base_t *base = &oem_data->base;

    if (base->header.magic == OEM_DATA_MAGIC) {
        /* Base data*/
        if (base->header.length <= OEM_DATA_BASE_DATA_SIZE) {
            uint32_t crc = getcrc32(base->data, base->header.length);

            if (crc == base->header.crc) {
                /* data check pass */
                *data = base->data;
                *len = base->header.length;

                return RET_OK;
            }
        }
    }

    return RET_FAIL;
}

uint8_t oem_data_iomap_load(uint8_t **data, uint32_t *len)
{
    oem_data_t *oem_data = oem_data_load();
    oem_data_iomap_t *iomap = &oem_data->iomap;

    if (iomap->header.magic == OEM_DATA_MAGIC) {
        /* IO MAP data*/
        if (iomap->header.length <= OEM_DATA_IOMAP_DATA_SIZE) {
            uint32_t crc = getcrc32(iomap->data, iomap->header.length);

            if (crc == iomap->header.crc) {
                /* data check pass */
                *data = iomap->data;
                *len = iomap->header.length;

                return RET_OK;
            }
        }
    }

    return RET_FAIL;
}

uint8_t oem_data_audmap_load(uint8_t **data, uint32_t *len)
{
    oem_data_t *oem_data = oem_data_load();
    oem_data_audmap_t *audmap = &oem_data->audmap;

    if (audmap->header.magic == OEM_DATA_MAGIC) {
        /* IO MAP data*/
        uint32_t crc = getcrc32(audmap->data, audmap->header.length);

        if (crc == audmap->header.crc) {
            /* data check pass */
            *data = audmap->data;
            *len = audmap->header.length;

            return RET_OK;
        }
    }

    return RET_FAIL;
}

uint8_t oem_data_eq_load(uint8_t **data, uint32_t *len)
{
    oem_data_t *oem_data = oem_data_load();
    oem_data_eq_t *eq = &oem_data->eq;

    if (eq->header.magic == OEM_DATA_MAGIC) {
        if (eq->header.length <= OEM_DATA_EQ_DATA_SIZE) {
            uint32_t crc = getcrc32(eq->data, eq->header.length);

            if (crc == eq->header.crc) {
                /* data check pass */
                *data = eq->data;
                *len = eq->header.length;

                return RET_OK;
            }
        }
    }

    return RET_FAIL;
}

uint8_t oem_data_get_ppm(uint8_t *ppm)
{
    oem_data_base_data_t *data;
    uint32_t len = 0;
    uint8_t status;

    status = oem_data_base_load((uint8_t **)&data, &len);   //lint !e740 Unusual pointer cast

    if (status == RET_OK) {
        *ppm = data->ppm;
    }

    return status;
}

uint8_t oem_data_get_mac_addr(uint8_t *mac)
{
    oem_data_base_data_t *data;
    uint32_t len = 0;
    uint8_t status;

    status = oem_data_base_load((uint8_t **)&data, &len);   //lint !e740 Unusual pointer cast

    if (status == RET_OK) {
        memcpy(mac, data->mac, MAC_ADDR_LEN);
    }
    return status;
}

uint8_t oem_data_anc_load(uint8_t **data, uint32_t *len)
{
    oem_data_anc_t *anc = anc_data_load();

    if (anc->header.magic == OEM_DATA_MAGIC) {
        uint32_t crc = getcrc32(anc->data, anc->header.length);

        if (crc == anc->header.crc) {
            /* data check pass */
            *data = anc->data;
            *len = anc->header.length;

            return RET_OK;
        }
    }

    return RET_FAIL;
}
