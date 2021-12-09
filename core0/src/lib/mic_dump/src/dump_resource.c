
#include "types.h"
#include "audio_cfg.h"
#include "iot_resource.h"
#include "dump_resource.h"

#define DUMP_IVALID_RESOURCE_NUM 0xFF

/*lint -esym(754, _dump_resource_msg_t::anc_open_flag) not referenced */
typedef struct _dump_resource_msg_t {
    uint8_t anc_open_flag;
    uint8_t asrc_bitmap;
    uint8_t fifo_bitmap;
    uint8_t chan_bitmap;
} dump_resource_msg_t;

static dump_resource_msg_t resource_env;

static void dump_set_chan_resource(uint8_t id)
{
    resource_env.chan_bitmap |= (uint8_t)BIT(id);
}

uint8_t dump_get_chan_resource(void)
{
    uint8_t chan_idx = DUMP_IVALID_RESOURCE_NUM;

    for (uint8_t idx = 0; idx < AUDIO_AUDIO_RX_DFE_MAX; idx++) {
        if (!(BIT(idx) & resource_env.chan_bitmap)) {
            chan_idx = idx;
            dump_set_chan_resource(idx);
            break;
        }
    }

    assert(chan_idx != DUMP_IVALID_RESOURCE_NUM);

    return chan_idx;
}

static void dump_set_fifo_resource(uint8_t id)
{
    resource_env.fifo_bitmap |= (uint8_t)BIT(id);
}

uint8_t dump_get_fifo_resource(void)
{
    uint8_t fifo_idx = DUMP_IVALID_RESOURCE_NUM;

    for (uint8_t idx = 0; idx < AUDIO_AUDIO_RX_FIFO_MAX; idx++) {
        if (!(BIT(idx) & resource_env.fifo_bitmap)) {
            fifo_idx = idx;
            dump_set_fifo_resource(idx);
            break;
        }
    }

    assert(fifo_idx != DUMP_IVALID_RESOURCE_NUM);

    return fifo_idx;
}

static void dump_set_asrc_resource(uint8_t id)
{
    resource_env.asrc_bitmap |= (uint8_t)BIT(id);
}

uint8_t dump_get_asrc_resource(void)
{
    uint8_t asrc_idx = DUMP_IVALID_RESOURCE_NUM;

    for (uint8_t idx = 0; idx < AUDIO_MIC_ASRC_CHANNEL_MAX; idx++) {
        if (!(BIT(idx) & resource_env.asrc_bitmap)) {
            asrc_idx = idx;
            dump_set_asrc_resource(idx);
            break;
        }
    }

    assert(asrc_idx != DUMP_IVALID_RESOURCE_NUM);

    return asrc_idx;
}

void dump_set_anc_resource(void)
{
    uint8_t feedback_port = iot_resource_lookup_adc(RESOURCE_AUDIO_ANC_FB);
    uint8_t feedforward_port = iot_resource_lookup_adc(RESOURCE_AUDIO_ANC_FF);
    uint8_t feedback_chn = iot_resource_lookup_channel(RESOURCE_AUDIO_ANC_FB);
    uint8_t feedforward_chn = iot_resource_lookup_channel(RESOURCE_AUDIO_ANC_FF);

    if (feedback_port < AUDIO_ADC_PORT_MAX || feedback_chn < AUDIO_AUDIO_RX_DFE_MAX) {
        dump_set_chan_resource(feedback_chn);
    }

    if (feedforward_port < AUDIO_ADC_PORT_MAX || feedforward_chn < AUDIO_AUDIO_RX_DFE_MAX) {
        dump_set_chan_resource(feedforward_chn);
    }

    dump_set_asrc_resource(PLAYER_ASRC_CHANNEL_RIGHT);
}
