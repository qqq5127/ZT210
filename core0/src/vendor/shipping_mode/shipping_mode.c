#include <stdint.h>
#include "shipping_mode.h"
#include "iot_gpio.h"
#include "iot_resource.h"
#include "dbglog.h"
#include "modules.h"

#ifndef SHIPPING_MODE_ENABLED
#define SHIPPING_MODE_ENABLED 1
#endif

#ifndef SHIPPING_MODE_GPIO
#define SHIPPING_MODE_GPIO GPIO_CUSTOMIZE_4
#endif

#define DBGLOG_SHIPPING_DBG(fmt, arg...) DBGLOG_STREAM_INFO(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)
#define DBGLOG_SHIPPING_ERR(fmt, arg...) DBGLOG_STREAM_ERROR(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)

void shipping_mode_enter(void)
{
#if SHIPPING_MODE_ENABLED
    uint8_t gpio;
    IOT_GPIO_PULL_MODE pull_mode;

    gpio = iot_resource_lookup_gpio(SHIPPING_MODE_GPIO);

    if (gpio == 0xFF) {
        DBGLOG_SHIPPING_ERR("shipping_mode_enter error, gpio not found\n");
        return;
    }

    pull_mode = iot_resource_lookup_pull_mode(gpio);

    DBGLOG_SHIPPING_DBG("shipping_mode_enter gpio:%d pull_mode:%d\n", gpio, pull_mode);

    iot_gpio_open(gpio, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_set_pull_mode(gpio, pull_mode);

    if (pull_mode == IOT_GPIO_PULL_UP) {
        iot_gpio_write(gpio, 0);
    } else {
        iot_gpio_write(gpio, 1);
    }
#else
    DBGLOG_SHIPPING_ERR("shipping_mode_enter error, not enabled\n");
#endif
}
