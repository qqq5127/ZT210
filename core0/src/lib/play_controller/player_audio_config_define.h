/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef AUDIO_CFG_H
#define AUDIO_CFG_H

#include "types.h"

#define MAX_MUSIC_EQ_BAND_NUM_CNT 20
#define MAX_VOICE_EQ_BAND_NUM_CNT 5

#define AUDIO_VOICE_PARAM_VER     1

typedef enum {
    SAMPLE_RATE_8000  = 0,
    SAMPLE_RATE_16000 = 1,
    SAMPLE_RATE_24000 = 2,
    SAMPLE_RATE_32000 = 3,
    SAMPLE_RATE_44100 = 4,
    SAMPLE_RATE_48000 = 5,
    SAMPLE_RATE_88200 = 6,
    SAMPLE_RATE_96000 = 7,
    SAMPLE_RATE_MAX   = 8
} sample_freq_t;

typedef enum {
    EQ_SLOP_BW6  = 0,
    EQ_SLOP_BW12 = 1,
    EQ_SLOP_BW18 = 2,
    EQ_SLOP_BW24 = 3,
    EQ_SLOP_MAX  = 4
} eq_slop_t;

typedef enum
{
    VOICE_PARAM_AEC_ANS_SEND = 0,  //deprecated
    VOICE_PARAM_EQ_SEND,
    VOICE_PARAM_AGC_SEND,
    VOICE_PARAM_DMA_SEND,
    VOICE_PARAM_RNN_SEND,
    VOICE_PARAM_DMNR_SEND,
    VOICE_PARAM_WIND_NOISE_SEND, //deprecated
    VOICE_PARAM_EQ_RECV,
    VOICE_PARAM_AGC_RECV,
    VOICE_PARAM_ANR_SEND,
    VOICE_PARAM_AEC_SEND,
    VOICE_PARAM_MCRA_SEND,
    VOICE_PARAM_DMNR2_SEND,
    VOICE_PARAM_NS_RECV,
    VOICE_PARAM_ALL = 0xFF,
    VOICE_PARAM_INIT = 0x100,
} voice_param_type_t;

typedef enum
{
    EQ_TYPE_PEAK = 0,  //peak
    EQ_TYPE_LOW_SHELF = 1, //low shelf
    EQ_TYPE_HIGH_SHELF = 2, //high shelf
} eq_type_t;

#pragma pack(push)  /* save the pack status */
#pragma pack(1)     /* 1 byte align */

//**************** COMMON ********************//
typedef struct eq_band_cfg {
    uint16_t            freq_hz;                //freq: 0~65535, 0 means do not enable
    int16_t             gain_db;                //eq gain: -1800~1800(real gain*100)
    uint16_t            q;                      //eq q: 1~1000(real q*100)
    uint8_t             type : 3;               //eq type, refer to enum eq_type_t
    uint8_t             reserved : 5;           //reserved
} eq_band_cfg_t;

typedef struct eq_cfg {
    uint8_t             sample_rate;                          //sample rate: refer to sample_freq_e
    uint8_t             enable_lp_filter;                     //if enable LP filter: 0/1
    uint16_t            lp_freq_hz;                           //LP freq: 0~65535
    uint8_t             lp_slop;                              //HP slope: refer to eq_slop_e
    uint8_t             enable_hp_filter;                     //if enabel HF filter: 0/1
    uint16_t            hp_freq_hz;                           //HP freq: 0~65535
    uint8_t             hp_slop;                              //HP slope: refer to eq_slop_e
    uint8_t             enable_eq_filter;                     //if enable EQ band filer: 0/1
    uint8_t             eq_gain;                              //reserved: TBD
    uint8_t             eq_mode;                              //the type of EQ, like rock, pop, classical, user defined: 0~10
    uint8_t             reserved[8];                          //reserved
} eq_cfg_t;

typedef struct music_eq_cfg {
    eq_cfg_t            music_eq_cfg;
    eq_band_cfg_t       eq_band_list[MAX_MUSIC_EQ_BAND_NUM_CNT];    //20 bands config: refer to eq_band_cfg_t
} music_eq_cfg_t;

typedef struct voice_eq_cfg {
    eq_cfg_t            voice_eq_cfg;
    eq_band_cfg_t       eq_band_list[MAX_VOICE_EQ_BAND_NUM_CNT];    //5 bands config: refer to eq_band_cfg_t
} voice_eq_cfg_t;

typedef struct agc_cfg {
    uint8_t             sample_rate;                //sample rate: refer to sample_freq_e
    uint8_t             enable_agc;                 //if enable agc: 0/1
    uint8_t             target_level_dbfs;          //target level: 0~30
    uint8_t             compression_gain_dB;        //compression gain: 0~90
    uint8_t             enable_limiter;             //if enable limiter: 0/1
    uint8_t             agc_mode;                   //agc mode: 0~3
    uint8_t             gain_step;                  //step of gain: 0~100(real gain_step*100)
    uint8_t             reserved[2];
} agc_cfg_t;

//deprecated
typedef struct aec_ans_send_cfg {
    uint8_t             sample_rate;                            //sample rate: refer to sample_freq_e
    uint8_t             enable_aec;                             //if enable aec: 0/1
    uint8_t             enable_rer;                             //if enable rer: 0/1
    uint8_t             enable_anr;                             //if enable anr: 0/1
    uint8_t             reference_loop_delay_ms;                //reference delay: 0~100
    uint8_t             echo_tail_10ms;                         //echo tail: 0~10
    uint8_t             double_talk_aggressiveness;             //double talk aggressiveness: 0~100(actual value*100)
    uint8_t             noise_reduction_aggressveness;          //noise reduction aggressiveness: 0~100(actual value*100)
    uint8_t             residual_echo_reduciton_threshold;      //residual_echo_reduciton_threshold: 0~100
    uint8_t             power;                                  //power: 0~100
    uint8_t             smooth_para;                            //smooth param: 0~100(actual value*100)
    uint8_t             reseng;                                 //reseng: 0~100
    uint8_t             reserved[2];
} aec_ans_send_cfg_t;

typedef struct anr_send_cfg {
    uint8_t             sample_rate;                            //sample rate: refer to sample_freq_e
    uint8_t             enable_anr;                             //if enable anr: 0/1
    uint16_t            anr_gain_ctr;                           //0~100 (actual value*100)
    uint8_t             reserved[4];
} anr_send_cfg_t;

typedef struct aec_send_cfg {
    uint8_t             sample_rate;                            //sample rate: refer to sample_freq_e
    uint8_t             enable_aec;                             //if enable aec: 0/1
    uint8_t             enable_res;                             //if enable rer: 0/1
    uint8_t             reference_loop_delay_ms;                //reference delay: 0~100
    uint8_t             echo_tail_10ms;                         //echo tail: 0~10
    uint8_t             aec_res_threshold;                      //residual_echo_reduciton_threshold: 0~100
    uint16_t            od_res_ctr;                             //(actual value*100)
    uint8_t             reserved[4];
} aec_send_cfg_t;

typedef struct dma_send_cfg {
    uint8_t             sample_rate;                //sample rate: refer to sample_freq_e
    uint8_t             enable_dma;                 //if enable dma(Differential microphone arrays): 0/1
    uint8_t             mic_distance_mm;            ///distance of 2 mics: 0~50
    uint8_t             beta;                       //beta: 0~10(actuall value*10)
    uint8_t             theta0;                     //theta: 0~100(actual value*10)
    uint8_t             reserved[2];
} dma_send_cfg_t;

typedef struct rnn_send_cfg {
    uint8_t             sample_rate;                //sample rate: refer to sample_freq_e
    uint8_t             enable_rnn;            //if enable AI noise
    uint8_t             reserved[4];
} rnn_send_cfg_t;

typedef struct dmnr_send_cfg {
    uint8_t             sample_rate;
    uint8_t             enable_dmnr;
    uint8_t             mic_distance_mm;                    //distance of mics: 0~50
    uint8_t             ild_mask_thld;                      //0~200(actual value*10)
    uint8_t             ild_vad_thld;                       //0~100(actual value*10)
    uint16_t            f_fb;                               //500~3000
    int16_t             pow_xref_thld;                      //-300~400(actual value*10)
    int16_t             ref_mask_thld;                      //-300~400(actual value*10)
    uint16_t            enlarge_thld;                       //0~300 (actual value*10)
    int16_t             estinoise_thld;                     //-200~200 (actual value*10)
    uint8_t             snr_thld;                           //0~100 (actual value*10)
    uint8_t             vad_thld;                           //0~100(actual value*100)
    uint8_t             use_fb_for_main:1;                  //if use FB mic as main mic. 1: use as main mic； 0： not use as main.
    uint8_t             fb_status_in_echo:1;                //if use FB Mic during echo, 1: use. 0: not use, switch to main mic.
    uint8_t             gain_compesate_eq_fb:6;             //FB eq gain compesate. 0~63(actual value*10)
    uint8_t             gain_compesate_eq_fb_beam:6;        //FB beanforming EQ gain compesate. 0~60(actual value*10)
    uint8_t             reserved0:2;
    uint8_t             gain_compesate_eq_fb_beam_anc_on:6; //FB beanforming EQ gain compesate when ANC on. 0~60(actual value*10)
    uint8_t             reserved1:2;
    uint8_t             n_eq_fb0:4;                         //FB EQ 0: 0~15
    uint8_t             n_eq_fb1:4;                         //FB EQ 1: 0~15
    uint8_t             n_eq_fb_beam0:4;                    //FB beamforming EQ 0: 0~15
    uint8_t             n_eq_fb_beam1:4;                    //FB beamforming EQ 1: 0~15
    uint8_t             n_eq_fb_beam_anc_on0:4;             //FB beamforming EQ when ANC On 0: 0~15
    uint8_t             n_eq_fb_beam_anc_on1:4;             //FB beamforming EQ when ANC On 1: 0~15
    uint8_t             loud_noisy_pow_thld;                //loud noise power threshold: 160~255.
    uint8_t             reserved2;                          //reserved
} dmnr_send_cfg_t;

typedef struct mcra_send_cfg {
    uint8_t             sample_rate;
    uint8_t             enable_mcra;
    uint8_t             pribf_delta_max;            //0~200 (actual value*10)
    uint8_t             pribf_delta_min;            //0~200 (actual value*10)
    uint8_t             postbf_delta_min;           //0~200 (actual value*10)
    uint8_t             postbf_delta_max;           //0~200 (actual value*10)
    int16_t             zeta_min;                   //-250~200 (actual value*10)
    int16_t             zeta_max;                   //-200~200 (actual value*10)
    int16_t             zeta_frame_min;             //-200~200 (actual value*10)
    int16_t             zeta_frame_max;             //-150~150 (actual value*10)
    int16_t             g_min;                      //-600~-100 (actual value*10)
    uint8_t             p_nn_mask_thld;             //10~100 (actual value*100)
    uint8_t             protect_bias;               //0~80 (actual value*100)
    uint8_t             reserved[8];                //TBD
} mcra_send_cfg_t;

//deprecated
typedef struct wind_noise_send_cfg {
    uint8_t             sample_rate;                //sample rate: refer to sample_freq_e
    uint8_t             enable_wind_noise;          //if enable wind noise: 0/1
    uint8_t             reserved[8];                //TBD
} wind_noise_send_cfg_t;

typedef struct dmnr_send_cfg2 {
    uint8_t             enable_dmnr_cfg2;
    uint16_t            loud_noisy_fb_voice_thld;           //loud noise fb threshold: 1000~2550(actual value*10)
    uint8_t             h_thld_up[4];                       //10~90(acutal value*100)
    uint8_t             h_thld_dn[4];                       //10~90(acutal value*100)
    uint8_t             h_thld_up_fb[4];                    //10~90(acutal value*100)
    uint8_t             h_thld_dn_fb[4];                    //10~90(acutal value*100)
    uint8_t             dmnr_pre_gain;                      //20~200(actual value*100)
    uint8_t             reserved[63];                       //reserved
} dmnr_send_cfg2_t;

typedef struct ns_recv_cfg{
    uint8_t             enable_ns_recv;
    uint8_t             delta_max;                  //0~200 (actual value*10)
    uint8_t             delta_min;                  //0~200 (actual value*10)
    int16_t             zeta_min;                   //-250~200 (actual value*10)
    int16_t             zeta_max;                   //-200~200 (actual value*10)
    int16_t             zeta_frame_min;             //-200~200 (actual value*10)
    int16_t             zeta_frame_max;             //-150~150 (actual value*10)
    int16_t             g_min;                      //-600~-0 (actual value*10)
    uint8_t             reserved[16];               //reserved
}ns_recv_cfg_t;

typedef struct voice_cfg_all {
    dma_send_cfg_t                  voice_dma_send_cfg;
    dmnr_send_cfg_t                 voice_dmnr_send_cfg;
    mcra_send_cfg_t                 voice_mcra_send_cfg;
    aec_send_cfg_t                  voice_aec_send_cfg;
    anr_send_cfg_t                  voice_anr_send_cfg;
    rnn_send_cfg_t                  voice_rnn_send_cfg;
    agc_cfg_t                       voice_agc_send_cfg;
    agc_cfg_t                       voice_agc_recv_cfg;
    voice_eq_cfg_t                  voice_eq_send_cfg;
    voice_eq_cfg_t                  voice_eq_recv_cfg;
    dmnr_send_cfg2_t                voice_dmnr_send_cfg2;
    ns_recv_cfg_t                   voice_ns_recv_cfg;
} voice_cfg_all_t;

#pragma pack(pop)   /* restore the pack status */

#endif //AUDIO_CFG_H

