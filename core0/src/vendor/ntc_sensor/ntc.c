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
#include "ntc.h"
#include "os_timer.h"
#include "lib_dbglog.h"
#include "iot_adc.h"
#include "iot_gpio.h"
#include "os_utils.h"
#include "string.h"
#include "iot_adc.h"
#include "iot_uart.h"
#include "vendor_msg.h"
#include "iot_resource.h"
#include "cli.h"

#define DBGLOG_NTC_INFO(fmt, arg...)  DBGLOG_STREAM_INFO(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)
#define DBGLOG_NTC_ERROR(fmt, arg...) DBGLOG_STREAM_ERROR(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)

#ifndef NTC_ADC_GPIO
#define NTC_ADC_GPIO GPIO_CUSTOMIZE_3
#endif

/* NTC_VOLT_TABLE is defined in build.ini
* temperature start from NTC_MINIMUM_TEMPERATURE, step 5C.
*
* table maybe as follow:
* {900,800,...,300,200}
*/
#ifndef NTC_VOLT_TABLE
#define NTC_VOLT_TABLE                                                                             \
    889, 885, 879, 872, 862, 849, 833, 813, 789, 760, 726, 688, 645, 600, 551, 502, 453, 404, 359, \
        316, 276, 241, 209, 181, 157, 135
#endif

#ifndef NTC_MINIMUM_TEMPERATURE
#define NTC_MINIMUM_TEMPERATURE -40
#endif

#define APP_CLI_MSGID_GET_NTC_VOLT 19

typedef struct {
    uint16_t volt;
} __attribute__((packed)) app_cli_get_ntc_volt_rsp_t;

static uint16_t trans_tab[] = {NTC_VOLT_TABLE};

static uint8_t adc_channel;
static bool_t ntc_inited = false;

static void cli_get_ntc_volt(uint8_t *buffer, uint32_t length)
{
    app_cli_get_ntc_volt_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    if (!ntc_inited) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_NTC_VOLT, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    int32_t temp_adc_val = iot_adc_poll_data(adc_channel, 0, 1);
    rsp.volt = iot_adc_2_mv(0, temp_adc_val);

    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_NTC_VOLT,
                               (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
}

static int8_t vlot_to_temperatrue(const uint16_t vlot_mv)
{
    uint16_t vlot_verify = 0;
    uint16_t vlot_verify_prev = 0;

    if (vlot_mv >= trans_tab[0]) {
        return NTC_MINIMUM_TEMPERATURE;
    }

    if (vlot_mv <= trans_tab[ARRAY_SIZE(trans_tab) - 1]) {
        return NTC_MINIMUM_TEMPERATURE + (ARRAY_SIZE(trans_tab) - 1) * 5;
    }

    for (uint8_t i = 1; i < ARRAY_SIZE(trans_tab); i++) {
        vlot_verify = trans_tab[i];
        vlot_verify_prev = trans_tab[i - 1];

        if (vlot_mv >= vlot_verify) {
            return NTC_MINIMUM_TEMPERATURE + (i - 1) * 5
                + (int16_t)(vlot_verify_prev - vlot_mv) * 5 / (vlot_verify_prev - vlot_verify);
        }
    }

    return NTC_MINIMUM_TEMPERATURE + (ARRAY_SIZE(trans_tab) - 1) * 5;
}

int8_t ntc_read(void)
{
    if (!ntc_inited) {
        return NTC_INVALID;
    }

    int32_t temp_adc_val = iot_adc_poll_data(adc_channel, 0, 1);

    float temp_voltage = iot_adc_2_mv(0, temp_adc_val);
    int8_t temperature = vlot_to_temperatrue(temp_voltage);

    DBGLOG_NTC_INFO("ntc volt:%d temp:%d\n", (int16_t)temp_voltage, temperature);

    return temperature;
}

void ntc_init(void)
{
#ifndef NTC_DISABLED
    uint8_t adc_gpio = iot_resource_lookup_gpio(NTC_ADC_GPIO);
    if ((adc_gpio >= IOT_GPIO_71) && (adc_gpio <= IOT_GPIO_74)) {
        adc_channel = adc_gpio - IOT_GPIO_71 + (uint8_t)IOT_ADC_EXT_SIG_CH0;
        iot_adc_open_external_port(adc_gpio);
        DBGLOG_NTC_INFO("NTC channel:%d, gpio:%d\n", adc_channel, adc_gpio);
    } else {
        DBGLOG_NTC_ERROR("NTC adc io get fail\n");
        return;
    }

    ntc_inited = true;
#endif
}

void ntc_deinit(void)
{
    if (!ntc_inited) {
        return;
    }

    ntc_inited = 0;
}

CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_NTC_VOLT, cli_get_ntc_volt);
