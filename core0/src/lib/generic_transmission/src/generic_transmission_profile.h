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

#ifndef __LIB_GENERIC_TRANSMISSION_PROFILE_H__
#define __LIB_GENERIC_TRANSMISSION_PROFILE_H__

typedef struct  {
    uint8_t io;
    uint8_t mode;
    uint8_t type;
    uint8_t tid;
    bool_t need_ack;
} generic_transmission_prf_data_tx_param_t;

typedef void (*generic_transmission_prf_data_rx_cb_t)(uint8_t tid,
                                                      uint8_t type,
                                                      uint8_t *data, int32_t data_len,
                                                      uint8_t );

int32_t generic_transmission_prf_register_rx_callback(uint8_t tid, const generic_transmission_data_rx_cb_t cb);

int32_t generic_transmission_prf_set_tid_priority(uint8_t tid, uint8_t prio);

int32_t generic_transmission_prf_data_tx(const uint8_t *data, uint32_t data_len, const generic_transmission_prf_data_tx_param_t *param);

int32_t generic_transmission_prf_data_tx_panic(const uint8_t *data, uint32_t data_len, const generic_transmission_prf_data_tx_param_t *param);

void generic_transmission_prf_init(void);

void generic_transmission_prf_deinit(void);

void generic_transmission_prf_tx_flush_panic(void);

int32_t generic_transmission_prf_tx_set_priority(uint8_t priority);

int32_t generic_transmission_prf_tx_restore_priority(void);
#endif /* __LIB_GENERIC_TRANSMISSION_PROFILE_H__ */
