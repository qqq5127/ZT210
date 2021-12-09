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
#include "lib_dbglog.h"
#include "critical_sec.h"

#include "os_utils.h"

#include "generic_transmission_api.h"
#include "generic_transmission_io.h"

struct generic_transmission_io_env_tag {
    generic_transmission_io_recv_cb_t recv_cb;
    generic_transmission_io_method_t *methods[GENERIC_TRANSMISSION_IO_NUM];
    uint8_t *recv_caches[GENERIC_TRANSMISSION_IO_NUM];
};

static struct generic_transmission_io_env_tag s_generic_transmission_io_env = {
    .recv_cb = NULL,
    .methods = {NULL},
    .recv_caches = {NULL},
};

uint8_t *generic_transmission_io_recv_cache_get(uint8_t io)
{
    if (io >= GENERIC_TRANSMISSION_IO_NUM) {
        return NULL;
    }

    return s_generic_transmission_io_env.recv_caches[io];
}

int32_t generic_transmission_io_send(uint8_t io, const uint8_t *buf, uint32_t len)
{
    int32_t ret = -RET_NOT_EXIST;

    if (io < GENERIC_TRANSMISSION_IO_NUM &&
            s_generic_transmission_io_env.methods[io] &&
            s_generic_transmission_io_env.methods[io]->send) {
        ret = s_generic_transmission_io_env.methods[io]->send(buf, len);
    }

    return ret;
}

int32_t generic_transmission_io_send_panic(uint8_t io, const uint8_t *buf, uint32_t len)
{
    int32_t ret = -RET_NOT_EXIST;

    if (io < GENERIC_TRANSMISSION_IO_NUM &&
            s_generic_transmission_io_env.methods[io] &&
            s_generic_transmission_io_env.methods[io]->send_panic) {
        ret = s_generic_transmission_io_env.methods[io]->send_panic(buf, len);
    }

    return ret;
}

void generic_transmission_io_register_recv_callback(generic_transmission_io_recv_cb_t recv_cb)
{
    s_generic_transmission_io_env.recv_cb = recv_cb;
}

void generic_transmission_io_init(void)
{
    for (int32_t io = 0; io < GENERIC_TRANSMISSION_IO_NUM; io++) {
        if (s_generic_transmission_io_env.methods[io] &&
                s_generic_transmission_io_env.methods[io]->init) {
            s_generic_transmission_io_env.methods[io]->init();
        }
    }
}

void generic_transmission_io_deinit(void)
{
    for (int32_t io = 0; io < GENERIC_TRANSMISSION_IO_NUM; io++) {
        if (s_generic_transmission_io_env.methods[io] &&
                s_generic_transmission_io_env.methods[io]->deinit) {
            s_generic_transmission_io_env.methods[io]->deinit();
        }
    }
}

//below functions are exposed to external.
void generic_transmission_io_recv(generic_transmission_io_t io, const uint8_t *data, uint32_t data_len, bool_t in_isr)
{
    if (s_generic_transmission_io_env.recv_cb) {
        if (!in_isr) {
            cpu_critical_enter();
            s_generic_transmission_io_env.recv_cb(io, data, data_len, in_isr);
            cpu_critical_exit();
        } else {
            s_generic_transmission_io_env.recv_cb(io, data, data_len, in_isr);
        }
    }
}

int32_t generic_transmission_io_method_register(generic_transmission_io_t io,
                                            generic_transmission_io_method_t *method)
{
    if (io >= GENERIC_TRANSMISSION_IO_NUM) {
        return -RET_INVAL;
    }

    s_generic_transmission_io_env.recv_caches[io] =
            (uint8_t *)os_mem_malloc(IOT_GENERIC_TRANSMISSION_MID, GENERIC_TRANSMISSION_IO_RECV_CACHE_SIZE);
    if (s_generic_transmission_io_env.recv_caches[io] == NULL) {
        return -RET_NOMEM;
    }

    s_generic_transmission_io_env.methods[io] = method;

    return RET_OK;
}
