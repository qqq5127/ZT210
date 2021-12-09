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

#ifndef __LIB_GENERIC_TRANSMISSION_IO_H__
#define __LIB_GENERIC_TRANSMISSION_IO_H__

#define GENERIC_TRANSMISSION_IO_RECV_CACHE_SIZE         (640U + 260U)

/*
 * @brief return handled length
 */
typedef int32_t (* generic_transmission_io_recv_cb_t)(uint8_t io, const uint8_t *buf, uint32_t len, bool_t in_isr);

void generic_transmission_io_register_recv_callback(generic_transmission_io_recv_cb_t recv_cb);

int32_t generic_transmission_io_send(uint8_t io, const uint8_t *buf, uint32_t len);

int32_t generic_transmission_io_send_panic(uint8_t io, const uint8_t *buf, uint32_t len);

void generic_transmission_io_init(void);

void generic_transmission_io_deinit(void);

uint8_t *generic_transmission_io_recv_cache_get(uint8_t io);

#endif /* __LIB_GENERIC_TRANSMISSION_IO_H__ */
