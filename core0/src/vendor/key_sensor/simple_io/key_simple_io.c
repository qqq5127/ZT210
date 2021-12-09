#include "key_sensor.h"

#if (KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO)
#include "key_base.h"
#include "vendor_msg.h"

void key_simple_io_init()
{
    return;
}
void key_simple_io_deinit()
{
    return;
}

void key_simple_io_open(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg)
{
    UNUSED(id_cfg);
    UNUSED(time_cfg);
}

#endif
