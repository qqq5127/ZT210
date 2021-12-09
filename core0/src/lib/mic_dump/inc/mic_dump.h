
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
#ifndef MIC_DUMP__H_
#define MIC_DUMP__H_

#ifdef __cplusplus
extern "C" {
#endif

#define DBGLOG_MIC_DUMP_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(LIB_MICDUMP_MID, fmt, ##arg)
#define DBGLOG_MIC_DUMP_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(LIB_MICDUMP_MID, fmt, ##arg)

typedef struct _spp_dump_param
{
    /* spp audio dump param*/
    uint8_t dump_delay_ms : 7;
    uint8_t need_ack : 1;
    uint8_t pkt_size;
} __attribute__((packed)) spp_dump_param;


/**
******************************************************************************
* @brief audio_dump_mic_start
*   start to dump mic data
*  @param[in] sample_cnt dump sample times
*
******************************************************************************
*/
void audio_dump_mic_start(uint16_t sample_cnt);

/**
******************************************************************************
* @brief audio_dump_mic_stop
*   stop to dump mic data
* @param[in] pause is 1 to pause the dump, 0 stop the dump.
*
******************************************************************************
*/
void audio_dump_mic_stop(uint8_t pause);

/**
******************************************************************************
* @brief audio_dump_anc_start
*   start anc func
*@param[in] anc_mode the anc mode assigned to
******************************************************************************
*/
void audio_dump_anc_start(uint8_t anc_mode);

/**
******************************************************************************
* @brief audio_dump_anc_stop
*   stop anc func
******************************************************************************
*/
void audio_dump_anc_stop(void);

/**
******************************************************************************
* @brief audio_dump_init
*   init dump mic variable
* @param[in] mic_bitmap start record mic bitmap
* @param[in] gain is set record mic gain
******************************************************************************
*/
void audio_dump_init(uint8_t mic_bitmap, int16_t gain);

/**
******************************************************************************
* @brief audio_dump_2_uart dump data by uart
* @param[in] buf is the dump buf of addr
* @param[in] buf_len is dump data len
******************************************************************************
*/
void audio_dump_2_uart(const uint8_t *buf, uint32_t buf_len);

/**
******************************************************************************
* @brief audio_dump_mode_set
* @param[in] mode is dump mode usrt/spp/flash
* return RET_OK is right else is wrong.
******************************************************************************
*/
uint8_t audio_dump_mode_set(uint8_t mode);

/**
******************************************************************************
* @brief spp_audio_dump_parameter_set
* @param[in] param is spp dump parameters
* return RET_OK is right else is wrong.
******************************************************************************
*/
uint8_t spp_audio_dump_parameter_set(const spp_dump_param *param);
#endif /* MIC_DUMP__H_ */
