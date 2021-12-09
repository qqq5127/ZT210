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

#ifndef TONE_STORAGE_H_
#define TONE_STORAGE_H_

/*
 ****************** tone storage map ****************************************************
 *
 *         |-----------------------------|
 *         |                             |    // bin文件通用头域（iot_flash_image_header_t） size:32 bytes
 *         |         bin header          |
 *         |-----------------------------|
 *         |                             |    // tone header（tone_header_t） size:32 bytes
 *         |         tone header         |
 *         |-----------------------------|
 *         |                             |    // tone table(tone_tbl_t), size: M * 12 bytes
 *         |         tone table          |
 *         |-----------------------------|
 *         |                             |
 *         |         tone data           |    // 提示音原始语言数据
 *         |                             |
 *         |-----------------------------|
 *
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "types.h"

/*
 * STRUCT DEFINITIONS
 ****************************************************************************************
 */
// the determiner of tone volume
enum tone_vol_mode {
    VOL_MANUFACTURER_MODE,              // manufacturer determined
    VOL_FOLLOW_MODE,                    // peer devices determined
    VOL_CUSTOM_MODE,                    // customer determined
    MAX_VOL_MODE = 3,
};

// codec type of the tone data
enum tone_codec_type {
    CODEC_MSBC_TYPE,
    MAX_CODEC_TYPE = 0x01,
};

// select channel for the current tone
enum tone_channel_select {
    TONE_LIFT_CHNL,
    TONE_RIGHT_CHNL,
    TONE_DOUBLE_CHNL,
    MAX_TONE_CHNL = 3,
};

// tone header info
typedef struct tone_header {
    uint8_t tone_num;                   // tone number
    uint8_t language_num;               // tone language number
    uint8_t version;                    // version of the tone bin file
    uint8_t codec_type;                 // codec type of the tone data
    uint32_t sample_freq;               // sample frequeccy of the tone data
    uint8_t rfu[24];                    // reserved for future
} tone_header_t;

// tone config data
typedef struct tone_config_info {
    uint32_t base_addr;                 // pointer to tone header.
    tone_header_t header;               // tone header
} tone_config_info_t;

// the tone data corresponding to a certain tone id stored in flash
typedef struct tone_table {
    uint32_t *data;                     // the tone data address
    uint32_t len;                       // the data lennth
    uint32_t sample_num;                // sample number of the tone data
    uint8_t vol;                        // the determine mode of volume
    uint8_t rfu[3];
} tone_table_t;

// tone status
typedef struct tone_stat_info {
    tone_table_t *table;                // pointer to current tone table corresponding to current tone language
    uint8_t curr_language;              // current tone language
} tone_stat_info_t;

/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */
/**
******************************************************************************************
* @brief       从kv中获取tone索引信息，计算evt table、tone table的地址并保存.
*
* @param[in]   is_reset                 true:恢复出厂设置，false:初始化
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_init(bool is_reset);

/**
******************************************************************************************
* @brief       获取tone header info.
*
* @param[out]  header                   指向tone header的二重指针
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_header_info_get(tone_header_t **header);

/**
******************************************************************************************
* @brief       从evt table表中获取tone id对应的tone压缩数据信息.
*
* @param[in]   tone_id                  tone_id
* @param[out]  tone_out                 tone table表中，tone id对应的tone地址等信息的指针
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_tbl_data_get(const uint16_t tone_id, tone_table_t *tone_out);

/**
******************************************************************************************
* @brief       获取当前tone语种.
*
* @param[out]  language                 获取的当前使用的tone音语言类别
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_language_get(uint8_t *language);

/**
******************************************************************************************
* @brief       设置当前tone语种.
*
* @param[in]   language                 设置tone音的语言类别
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_language_set(const uint8_t language);

/**
******************************************************************************************
* @brief 根据tone存储地址和长度复制flash中的tone压缩数据.
*
* @param[out]  buf                      复制tone原始数据的buffer
* @param[in]   tone_data_addr           某个tone id对应的tone音数据在flash中的地址
* @param[in]   len                      某个tone id对应的tone音数据长度
*
* @return      status
******************************************************************************************
*/
uint8_t tone_storage_data_read(uint8_t *buf, const uint32_t *tone_data_addr, const uint32_t len);

#endif // TONE_STORAGE_H_
