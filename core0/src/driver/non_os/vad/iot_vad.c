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
#include "chip_reg_base.h"

/* hal includes */
#include "iot_irq.h"
#include "iot_vad.h"

/* hw includes */
#include "apb.h"
#include "aud_glb.h"
#include "aud_if.h"

#define IOT_VAD_SECTION_SIZE           512
#define IOT_VAD_DBG_OFFECT             0x70
#define IOT_VAD_MEMORY_SIZE            1024
#define IOT_VAD_FIRST_SECTION_START    0x00
#define IOT_VAD_SECOND_SECTION_START   0x200
#define IOT_VAD_MEMORY_BASE_ADDR_START (VAD_BASEADDR + 0x0200)
/*lint -esym(750,IOT_VAD_MEMORY_BASE_ADDR_END) IOT_VAD_MEMORY_BASE_ADDR_END is not referenced */
#define IOT_VAD_MEMORY_BASE_ADDR_END (VAD_BASEADDR + 0x1200)

#define IOT_VAD_END_DETECTION   150
#define IOT_VAD_START_DETECTION 50

typedef struct iot_vad_trans {
    iot_vad_callback cb;
    uint32_t *buffer;
    uint32_t length;
    uint32_t rd_index;
    uint16_t bitmap;
} iot_vad_trans_t;

static iot_vad_trans_t dbg;
static iot_vad_trans_t tvad;
static iot_vad_trans_t gvad;
static iot_vad_trans_t mem;

static uint32_t iot_tvad_isr_handler(uint32_t vector, uint32_t data);
static uint32_t iot_vad_mem_isr_handler(uint32_t vector, uint32_t data);
static uint32_t iot_gvad_isr_handler(uint32_t vector, uint32_t data);
static uint32_t iot_vad_dbg_isr_handler(uint32_t vector, uint32_t data);

static const iot_vad_iir_coeff_t coeff_table[IOT_VAD_FRQ_MAX] = {
    {-29863, 13716, 14991, -29982, 14991},
    {-31313, 14991, 15672, -31344, 15672},
    {-32040, 15672, 16024, -32048, 16024},
    {-32404, 16204, 16203, -32406, 16203},
};

static const int energy_calculat_table[IOT_VAD_FRQ_MAX][6] = {
    {40, 1, 2, 0, 7, 0},
    {80, 1, 2, 0, 8, 0},
    {160, 1, 2, 0, 9, 1},
    {320, 1, 2, 0, 10, 2},
};

static const int zcr_cfg_table[IOT_VAD_FRQ_MAX][6] = {
    {416, 1536, 7, 240, 12, 240},
    {384, 1766, 8, 480, 24, 480},
    {230, 1075, 9, 480, 24, 480},
    {154, 768, 10, 480, 24, 480},
};

static const vad_gmm_ctrl_config_t gmm_ctrl_cfg_table[IOT_VAD_FRQ_MAX] = {
    {3, 0, 14},
    {2, 0, 14},
    {1, 0, 14},
    {0, 0, 14},
};

void iot_vad_dbg_init(IOT_VAD_DBG_MODE mode)
{
    dbg.cb = NULL;
    dbg.buffer = NULL;
    dbg.length = 0;
    dbg.bitmap = 0;
    vad_dbg_it_mode(mode == IOT_VAD_DBG_DEBUG);

    if (mode == IOT_VAD_DBG_LARGE_BKGE) {
        vad_large_noise_config_t cfg;
        cfg.end_detection = IOT_VAD_END_DETECTION;
        cfg.start_detection = IOT_VAD_START_DETECTION;
        cfg.hardware_detection_en = true;
        cfg.false_ararm_delay = 4;
        vad_strong_backgroud_noise_config(&cfg);
    }
}

void iot_tvad_init(IOT_TVAD_MODE mode)
{
    tvad.cb = NULL;
    tvad.buffer = NULL;
    tvad.length = 0;
    tvad.bitmap = 0;
    vad_time_domain_it_mode(mode == IOT_TVAD_SPEECH_DOUBLE_EDGE);
}

void iot_gvad_init(void)
{
    gvad.cb = NULL;
    gvad.buffer = NULL;
    gvad.length = 0;
    gvad.bitmap = 0;
}

void iot_vad_mem_init(void)
{
    mem.cb = NULL;
    mem.buffer = NULL;
    mem.length = 0;
    mem.bitmap = 0;
}

static void iot_tvad_irq_config(void)
{
    iot_irq_t vad_td_isr;
    vad_td_isr = iot_irq_create(VAD_TDVAD_INT, 0, iot_tvad_isr_handler);
    vad_time_domain_it_clear(true);
    vad_time_domain_it_clear(false);
    iot_irq_unmask(vad_td_isr);
}

static void iot_vad_mem_irq_config(void)
{
    iot_irq_t vad_buf_isr;

    vad_buf_isr = iot_irq_create(VAD_BUF_INT, 0, iot_vad_mem_isr_handler);
    vad_buf_it_clear(true);
    vad_buf_it_clear(false);
    iot_irq_unmask(vad_buf_isr);
}

static void iot_gvad_irq_config(void)
{
    iot_irq_t vad_gmm_isr;
    vad_gmm_isr = iot_irq_create(VAD_GMM_INT, 0, iot_gvad_isr_handler);
    vad_gmm_it_clear(true);
    vad_gmm_it_clear(false);
    iot_irq_unmask(vad_gmm_isr);
}

static void iot_vad_dbg_irq_config(void)
{
    iot_irq_t vad_dbg_isr;
    vad_dbg_isr = iot_irq_create(VAD_TDVAD_DBG_INT, 0, iot_vad_dbg_isr_handler);
    vad_dbg_it_clear(true);
    vad_dbg_it_clear(false);
    iot_irq_unmask(vad_dbg_isr);
}

uint8_t iot_tvad_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb)
{
    if (cb == NULL || buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    if (tvad.cb || tvad.buffer || tvad.length) {
        return RET_AGAIN;
    }

    tvad.cb = cb;
    tvad.buffer = buffer;
    tvad.length = length;
    tvad.rd_index = 0;
    tvad.bitmap = BIT(IOT_VAD_DBG_ZF) | BIT(IOT_VAD_DBG_NOISE_NE) | BIT(IOT_VAD_SPEECH_STATE);

    iot_tvad_irq_config();
    vad_time_domain_it_enable(true);
    return RET_OK;
}

uint8_t iot_tvad_close(void)
{
    iot_vad_td_stop();
    vad_time_domain_it_enable(false);
    return RET_OK;
}

uint8_t iot_gvad_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb, IOT_WEBRTC_MODE mode)
{
    if (cb == NULL || buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    if (gvad.cb || gvad.buffer || gvad.length) {
        return RET_AGAIN;
    }

    gvad.cb = cb;
    gvad.buffer = buffer;
    gvad.length = length;
    gvad.bitmap = 0;
    gvad.rd_index = 0;

    iot_gvad_irq_config();
    if (mode == IOT_WEBRTC_MODE_SLEEP) {
        vad_gmm_it_enable(false);
    }
    if (mode == IOT_WEBRTC_MODE_ALL) {
        iot_vad_gmm_start();
        vad_gmm_it_enable(true);
    }

    return RET_OK;
}

uint8_t iot_gvad_close(void)
{
    iot_vad_gmm_stop();
    vad_gmm_it_enable(false);
    return RET_OK;
}

uint8_t iot_vad_mem_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb,
                         IOT_WEBRTC_MODE mode)
{
    if (cb == NULL || buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    if (mem.cb || mem.buffer || mem.length) {
        return RET_AGAIN;
    }

    mem.cb = cb;
    mem.buffer = buffer;
    mem.length = length;
    mem.rd_index = 0;

    iot_vad_mem_irq_config();
    /* get start address of the memory buffer */

    if (mode == IOT_WEBRTC_MODE_SLEEP) {
        vad_buf_it_enable(false);
    }
    if (mode == IOT_WEBRTC_MODE_ALL) {
        vad_buf_it_enable(true);
    }

    return RET_OK;
}

uint8_t iot_vad_mem_close(void)
{
    vad_buf_it_enable(false);
    return RET_OK;
}

uint8_t iot_vad_dbg_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb,
                         IOT_WEBRTC_MODE mode)
{
    if (cb == NULL || buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    if (dbg.cb || dbg.buffer || dbg.length) {
        return RET_AGAIN;
    }

    dbg.cb = cb;
    dbg.buffer = buffer;
    dbg.length = length;
    dbg.bitmap = BIT(IOT_VAD_DBG_ZF) | BIT(IOT_VAD_DBG_NOISE_NE) | BIT(IOT_VAD_SPEECH_STATE);

    dbg.rd_index = 0;

    iot_vad_dbg_irq_config();

    if (mode == IOT_WEBRTC_MODE_SLEEP) {
        vad_dbg_it_enable(false);
    }
    if (mode == IOT_WEBRTC_MODE_ALL) {
        vad_dbg_it_enable(true);
    }

    return RET_OK;
}

uint8_t iot_vad_dbg_close(void)
{
    vad_dbg_it_enable(false);
    return RET_OK;
}

static void iot_vad_read_reg(uint32_t *buffer, uint32_t length, uint16_t bitmap)
{
    uint32_t size = length;
    uint16_t bit_map = bitmap;
    uint32_t *dst_buffer = buffer;
    volatile uint32_t *dbg_reg_addr;

    dbg_reg_addr = (volatile uint32_t *)(VAD_BASEADDR + IOT_VAD_DBG_OFFECT);
    for (uint32_t i = 0; i < (size - 1); i++) {
        if (bit_map & (1U << i)) {
            dst_buffer[i] = *dbg_reg_addr++;
        } else {
            dst_buffer[i] = 0xFFFFFFFF;
            dbg_reg_addr++;
        }
    }
    if (bit_map & (1U << (size - 1))) {
        dst_buffer[size - 1] = (uint32_t)vad_get_buf_speeck_state();
    } else {
        dst_buffer[size - 1] = 0xFFFFFFFF;
    }
}

static void iot_gvad_read_subband(uint32_t *buffer, uint32_t length)
{
    int size = (int)length;
    uint32_t *dst_buffer = buffer;
    for (int i = 0; i < size; i++) {
        *dst_buffer++ = vad_gmm_subband_energy(VAD_GMM_SUB_BAND_0 + (VAD_GMM_SUB_BAND_ID)i);
    }
}

uint32_t iot_vad_read_memory_first(void)
{
    uint16_t i;
    iot_vad_trans_t *transmit = &mem;

    volatile uint32_t *mem_addr;   //32bit address of vad ram
    //32bit address of internal ram
    uint32_t *trans_index = transmit->buffer + transmit->rd_index;

    uint16_t buf_ptr = vad_get_buf_pointer();
    mem_addr = (volatile uint32_t *)(buf_ptr * sizeof(uint32_t) + IOT_VAD_MEMORY_BASE_ADDR_START);

    if (buf_ptr > IOT_VAD_SECTION_SIZE) {   //>512
        for (i = 0; i < (1024 - buf_ptr); i++) {
            *trans_index++ = *mem_addr++;
        }
        mem_addr = (volatile uint32_t *)(IOT_VAD_MEMORY_BASE_ADDR_START);
        for (i = 0; i < IOT_VAD_SECTION_SIZE; i++) {
            *trans_index++ = *mem_addr++;
        }
        transmit->rd_index = (IOT_VAD_MEMORY_SIZE - buf_ptr) + IOT_VAD_SECTION_SIZE;
        transmit->bitmap = IOT_VAD_SECOND_SECTION_START;

    } else {   //<512
        for (i = 0; i < (IOT_VAD_MEMORY_SIZE - buf_ptr); i++) {
            *trans_index++ = *mem_addr++;
        }
        transmit->rd_index = IOT_VAD_MEMORY_SIZE - buf_ptr;
        transmit->bitmap = IOT_VAD_FIRST_SECTION_START;
    }
    if (transmit->rd_index > transmit->length) {
        transmit->rd_index -= transmit->length;
    }
    if (transmit->cb != NULL) {
        transmit->cb(transmit->buffer, transmit->rd_index);
    }
    return transmit->rd_index;
}

static void iot_vad_read_memory(iot_vad_trans_t *trans)
{
    uint32_t i, left;
    volatile uint32_t *mem_addr;
    iot_vad_trans_t *transmit = trans;
    uint32_t *trans_index = transmit->buffer + transmit->rd_index;
    mem_addr =
        (volatile uint32_t *)(transmit->bitmap * sizeof(uint32_t) + IOT_VAD_MEMORY_BASE_ADDR_START);

    if (transmit->rd_index + IOT_VAD_SECTION_SIZE > transmit->length) {
        left = transmit->length - transmit->rd_index;
        for (i = 0; i < left; i++) {
            *trans_index++ = *mem_addr++;
        }
        trans_index = transmit->buffer;
        for (i = 0; i < (IOT_VAD_SECTION_SIZE - left); i++) {
            *trans_index++ = *mem_addr++;
        }
    } else {
        for (i = 0; i < IOT_VAD_SECTION_SIZE; i++) {
            *trans_index++ = *mem_addr++;
        }
    }
    transmit->rd_index += IOT_VAD_SECTION_SIZE;   //add 512
    if (transmit->rd_index >= transmit->length) {
        transmit->rd_index -= transmit->length;
    }
    //transmit->bitmap=0,512
    transmit->bitmap = (transmit->bitmap + IOT_VAD_SECTION_SIZE) % IOT_VAD_MEMORY_SIZE;
    if (transmit->cb != NULL) {
        transmit->cb((transmit->buffer + transmit->rd_index), transmit->rd_index);
    }
}

static uint32_t iot_vad_mem_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_vad_mem_isr_handler);
static uint32_t iot_vad_mem_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    vad_buf_it_enable(false);

    iot_vad_read_memory(&mem);

    vad_buf_it_enable(true);
    vad_buf_it_clear(true);
    vad_buf_it_clear(false);

    return RET_OK;
}

static uint32_t iot_gvad_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_gvad_isr_handler);
static uint32_t iot_gvad_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    vad_gmm_it_enable(false);

    gvad.length = VAD_GMM_SUB_BAND_MAX;
    iot_gvad_read_subband(gvad.buffer, gvad.length);
    if (gvad.cb != NULL) {
        gvad.cb(gvad.buffer, gvad.length);
    }

    vad_gmm_it_enable(true);
    vad_gmm_it_clear(true);
    vad_gmm_it_clear(false);

    return RET_OK;
}

static uint32_t iot_vad_dbg_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_vad_dbg_isr_handler);
static uint32_t iot_vad_dbg_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    vad_dbg_it_enable(false);

    if (dbg.bitmap) {
        iot_vad_read_reg(dbg.buffer, dbg.length, dbg.bitmap);
        if (dbg.cb != NULL) {
            dbg.cb(dbg.buffer, dbg.length);
        }
    }
    vad_dbg_it_enable(true);
    vad_dbg_it_clear(true);
    vad_dbg_it_clear(false);

    return RET_OK;
}

static uint32_t iot_tvad_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_tvad_isr_handler);
static uint32_t iot_tvad_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    vad_time_domain_it_enable(false);

    if (tvad.bitmap) {
        iot_vad_read_reg(tvad.buffer, tvad.length, tvad.bitmap);
        if (tvad.cb != NULL) {
            tvad.cb(tvad.buffer, tvad.length);
        }
    }

    vad_time_domain_it_enable(true);
    vad_time_domain_it_clear(true);
    vad_time_domain_it_clear(false);
    return RET_OK;
}

void iot_vad_clk_enable(void)
{
    audio_enable_module(AUDIO_MODULE_VAD);
}

void iot_vad_reset(void)
{
    audio_reset_module(AUDIO_MODULE_VAD);
}

void iot_vad_soft_reset(void)
{
    apb_m_s_reset(APB_M_S_VAD_CLK);
}

void iot_vad_td_start(void)
{
    vad_td_enable(true);
}

void iot_vad_td_stop(void)
{
    vad_td_enable(false);
}

void iot_vad_gmm_start(void)
{
    vad_gmm_enable(true);
}

void iot_vad_gmm_stop(void)
{
    vad_gmm_enable(false);
}

void iot_vad_ram_remap_enable(bool_t en)
{
    apb_misc_remapsfc_enable(en);
}

void iot_vad_src_link(IOT_VAD_SRC_ID src)
{
    vad_input_source_link(src == IOT_VAD_SRC_MADC);
}

bool_t iot_vad_get_buf_speeck_state(void)
{
    return vad_get_buf_speeck_state();
}

uint16_t iot_vad_get_buf_pointer(void)
{
    return vad_get_buf_pointer();
}

bool_t iot_vad_get_buf_it_status(void)
{
    return vad_get_buf_it_status();
}

bool_t iot_vad_get_buf_it_raw(void)
{
    return vad_get_buf_it_raw();
}

bool_t iot_vad_vad_get_dbg_it_status(void)
{
    return vad_get_dbg_it_status();
}

void iot_vad_iir_coefficient_config(const iot_vad_iir_coeff_t *coeff)
{
    vad_iir_coefficient_config(VAD_IIR_B0, (int16_t)coeff->iir_b0);
    vad_iir_coefficient_config(VAD_IIR_B1, (int16_t)coeff->iir_b1);
    vad_iir_coefficient_config(VAD_IIR_B2, (int16_t)coeff->iir_b2);
    vad_iir_coefficient_config(VAD_IIR_A0, (int16_t)coeff->iir_a0);
    vad_iir_coefficient_config(VAD_IIR_A1, (int16_t)coeff->iir_a1);
}

void iot_vad_general_config(const iot_vad_config_t *cfg)
{
    vad_time_domain_src(false);
    vad_gmm_contrlo_config(&cfg->gmm_ctrl);
    vad_decision_config(&cfg->vad_decision);
    vad_noise_control(&cfg->noise_nontrol);
    vad_zcr_config(&cfg->zcr_cfg);
    vad_energy_calculation_config(&cfg->energy_calculat);
    vad_adc_bit_shift_bit(cfg->shift_bit);
}

void iot_vad_strong_backgroud_noise_config(void)
{
    vad_large_noise_config_t cfg;
    cfg.end_detection = 30;
    cfg.start_detection = 20;
    cfg.hardware_detection_en = true;
    vad_strong_backgroud_noise_config(&cfg);
}

void iot_vad_set_bitmap(IOT_VAD_MODE mode, uint16_t bitmap)
{
    switch (mode) {
        case IOT_VAD_TVAD:
            tvad.bitmap = bitmap;
            break;
        case IOT_VAD_DBG:
            dbg.bitmap = bitmap;
            break;
        case IOT_VAD_MEM:
            break;
        case IOT_VAD_GMM:
            break;
        case IOT_VAD_MAX:
            break;
        default:
            break;
    }
}

void iot_vad_config(IOT_VAD_FREQUENCE frq)
{
    iot_vad_config_t cfg;
    iot_vad_iir_coeff_t coeff;

    cfg.shift_bit = 8;

    //IIR
    coeff.iir_a0 = coeff_table[frq].iir_a0;
    coeff.iir_a1 = coeff_table[frq].iir_a1;
    coeff.iir_b0 = coeff_table[frq].iir_b0;
    coeff.iir_b1 = coeff_table[frq].iir_b1;
    coeff.iir_b2 = coeff_table[frq].iir_b2;
    //EZCalc
    cfg.energy_calculat.basic_len = (uint16_t)energy_calculat_table[frq][0];
    cfg.energy_calculat.shift_len_ratio = (uint8_t)energy_calculat_table[frq][1];
    cfg.energy_calculat.frame_len_ratio = (uint8_t)energy_calculat_table[frq][2];
    cfg.energy_calculat.ec_power_mode = !!energy_calculat_table[frq][3];
    cfg.energy_calculat.energy_shit_bit = (uint8_t)energy_calculat_table[frq][4];
    cfg.energy_calculat.zcr_shit_bit = (uint8_t)energy_calculat_table[frq][5];
    //ESEL
    cfg.zcr_cfg.low_thr = (uint16_t)zcr_cfg_table[frq][0];
    cfg.zcr_cfg.hight_thr = (uint16_t)zcr_cfg_table[frq][1];
    cfg.zcr_cfg.adj_shift = (uint8_t)zcr_cfg_table[frq][2];
    cfg.zcr_cfg.zcr_max = (uint16_t)zcr_cfg_table[frq][3];
    cfg.zcr_cfg.zcr_min = (uint16_t)zcr_cfg_table[frq][4];
    cfg.zcr_cfg.hight_bias = (uint16_t)zcr_cfg_table[frq][5];
    cfg.zcr_cfg.zcr_sel = false;
    cfg.zcr_cfg.sts_frame_num = 16;
    cfg.zcr_cfg.hold_in_speech_flag = true;
    cfg.zcr_cfg.low_bias = 0;
    cfg.zcr_cfg.low_ratio = 64;
    cfg.zcr_cfg.hight_ratio = 32;
    cfg.zcr_cfg.zcr_update_thr0 = 4;
    cfg.zcr_cfg.zcr_update_thr1 = 4;
    cfg.zcr_cfg.force_zcr_sel = false;
    cfg.zcr_cfg.default_zcr = 2;
    cfg.zcr_cfg.noise_init_frm_num = 20;
    cfg.zcr_cfg.zcr_hold_period = 3;
    //NE
    cfg.noise_nontrol.sigma_init_frm_num = 10;
    cfg.noise_nontrol.pow_change_thr = 16;
    cfg.noise_nontrol.pow_change_en = true;
    cfg.noise_nontrol.win_pre = 5;
    cfg.noise_nontrol.win_post = 2;
    cfg.noise_nontrol.alpha = 252;
    cfg.noise_nontrol.mode = true;
    cfg.noise_nontrol.always_upd_thr_exp = 0;
    cfg.noise_nontrol.always_upd_thr_val = 20;
    //VAD
    cfg.vad_decision.low_ratio = 1024;
    cfg.vad_decision.high_ratio = 1024;
    cfg.vad_decision.method = true;
    cfg.vad_decision.low_len = 7;
    cfg.vad_decision.high_len = 4;
    cfg.vad_decision.thr_min = 100;

    /* if freqence from i2s,td_vad_src_sel chang value, aud_smp_rate
     * always 0. if aud_smp_rate changed, td_vad_src_sel always 0,
     */
    cfg.gmm_ctrl.aud_smp_rate = gmm_ctrl_cfg_table[frq].aud_smp_rate;
    cfg.gmm_ctrl.td_vad_src_sel = gmm_ctrl_cfg_table[frq].td_vad_src_sel;
    cfg.gmm_ctrl.frame_band_energy = gmm_ctrl_cfg_table[frq].frame_band_energy;

    vad_zcr_calculation_threshold(0);
    vad_reference_noise_estimation_store_gap(0);
    vad_reference_noise_estimation_signal_enable(true);

    iot_vad_soft_reset();
    iot_vad_td_stop();
    iot_vad_gmm_stop();
    iot_vad_ram_remap_enable(false); /* it buf need ram remap disable */
    iot_vad_clk_enable();
    iot_vad_iir_coefficient_config(&coeff);
    iot_vad_general_config(&cfg);
}

uint8_t iot_vad_read_memory_poll(uint32_t *mem_pointer, bool_t state)
{
    uint16_t i;
    uint32_t mem_addr;
    if (state) {
        /* read 512 bytes to mem_addr */
        for (i = 0; i < IOT_VAD_SECTION_SIZE; i++) {
            mem_addr = (0 + i) * sizeof(uint32_t)   //lint !e835:code style
                + IOT_VAD_MEMORY_BASE_ADDR_START;

            *mem_pointer++ = *(uint32_t *)(mem_addr);
        }
    } else {
        for (i = 0; i < IOT_VAD_SECTION_SIZE; i++) {
            mem_addr =
                (IOT_VAD_SECTION_SIZE + i) * sizeof(uint32_t) + IOT_VAD_MEMORY_BASE_ADDR_START;
            *mem_pointer++ = *(uint32_t *)(mem_addr);
        }
    }
    return RET_OK;
}

uint16_t iot_vad_get_dbg_frame_index(void)
{
    return vad_get_dbg_frame_index();
}

uint8_t iot_vad_get_debug_mode(void)
{
    return vad_get_debug_mode();
}

uint8_t iot_vad_get_long_large_bkg_flag(void)
{
    return vad_get_long_large_bkg_flag();
}

void iot_vad_long_large_bkg_flag_clr(void)
{
    vad_long_large_bkg_flag_clr();
}

uint32_t iot_vad_get_noise_ne(void)
{
    return vad_get_noise_ne();
}

uint16_t iot_vad_get_frame_length(void)
{
    uint16_t length = 480;

    uint16_t sample_rate = vad_get_aud_smp_rate();
    uint16_t enrg_frm = vad_get_enrg_frm();

    switch (sample_rate) {
        case 0:
            length = (uint16_t)(32 * (enrg_frm + 1) * 2);   //sample rate is 4kHz
            break;
        case 1:
            length = (uint16_t)(16 * (enrg_frm + 1) * 2);   //sample rate is 8kHz
            break;
        case 2:
            length = (uint16_t)(8 * (enrg_frm + 1) * 2);   //sample rate is 16kHz
            break;
        case 3:
            length = (uint16_t)(4 * (enrg_frm + 1) * 2);   //sample rate is 32kHz
            break;
        default:
            break;
    }
    return length;
}
