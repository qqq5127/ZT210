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
#ifndef __AUDIO_ANC_H__
#define __AUDIO_ANC_H__

#include "types.h"

//enable anc on dfs req 32M
#define ANC_DFS_32M

#define AUDIO_ANC_MODE_MAX 10

typedef enum {
    AUDIO_DENOISE_PHYSICAL,
    AUDIO_DENOISE_ANC_LIGHT,
    AUDIO_DENOISE_ANC_MIDDLE,
    AUDIO_DENOISE_ANC_DEEP,//as well as commuting pattern
    AUDIO_DENOISE_TRANSPARENT_FULL,
    AUDIO_DENOISE_ANC_INDOOR,
    AUDIO_DENOISE_ANC_OUTDOOR,
    AUDIO_DENOISE_TRANSPARENT_VOICE,
    AUDIO_DENOISE_CUSTOM,
    AUDIO_DENOISE_RESERVED1,
    AUDIO_DENOISE_RESERVED2,
    AUDIO_DENOISE_MODE_MAX,
} AUDIO_DENOISE_MODE;

typedef enum {
    AUDIO_ANC_MODE_NONE = 0,
    AUDIO_ANC_FF = BIT(0),
    AUDIO_ANC_FB = BIT(1),
    AUDIO_ANC_MIX = BIT(0) | BIT(1),
}AUDIO_ANC_MODE;

typedef enum {
    AUDIO_ANC_DETECT_NONE = 0,
    AUDIO_ANC_DETECT_HOWLROUND = BIT(0),
    AUDIO_ANC_DETECT_WIND_NOSIE = BIT(1),
    AUDIO_ANC_DETECT_ALL = BIT(0) | BIT(1),
}AUDIO_ANC_DETECK;

typedef enum {
    AUDIO_SPK_FROM_ANC,
    AUDIO_SPK_FROM_PLAYER,
    AUDIO_SPK_FROM_MAX,
}AUDIO_SPK_CONTROL;

enum {
    AUDIO_ANC_STATE_CLOSE,
    AUDIO_ANC_STATE_OPEN,
};

typedef enum {
    AUDIO_ANC_EVENT_HOWLROUND_START = BIT(0),
    AUDIO_ANC_EVENT_HOWLROUND_STOP = BIT(1),
    AUDIO_ANC_EVENT_WIND_NOISE_START = BIT(2),
    AUDIO_ANC_EVENT_WIND_NOISE_STOP = BIT(3),
    AUDIO_ANC_EVENT_MAX,
}AUDIO_ANC_EVENT;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint8_t ff_coeff_mode:4;
    uint8_t fb_coeff_mode:4;
    int8_t ff_coeff_gain;
    int8_t fb_coeff_gain;
    int8_t ec_coeff_gain;
    uint8_t ff_user_gain;
}anc_coeff_idx_cfg;

typedef struct {
    uint8_t anc_coeff_enable;
    uint8_t anc_detect_enable;
    uint8_t anc_frame_cnt;
    uint8_t reserved[4];
    anc_coeff_idx_cfg coeff_cfg[AUDIO_ANC_MODE_MAX];
} audio_anc_coeff_cfg_t;
#pragma pack(pop)

typedef void (*audio_anc_switch_done_handle_cb)(AUDIO_DENOISE_MODE mode);
typedef void (*audio_anc_noise_handle_cb)(AUDIO_ANC_EVENT event);

/**
 * @brief init anc path
 * @return uint8_t
 */
uint8_t audio_anc_init(void);

/**
 * @brief  deinit anc path
 *
 */
void audio_anc_deinit(void);

/**
 * @brief open the audio anc path
 * @return uint8_t
 */
uint8_t audio_anc_open(void);

/**
 * @brief close the audio anc path
 * @return uint8_t
 */
uint8_t audio_anc_close(void);

/**
 * @brief switch anc coefficient table
 * @param mode anc denoise mode
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_anc_switch_mode(AUDIO_DENOISE_MODE mode);

/**
 * @brief mute anc path at clock raising
 *
 * @param src_mode current clock mode
 * @param dst_mode next clock mode
 */
void audio_anc_switch_clock_mute_hook(uint32_t src_mode, uint32_t dst_mode);

/**
 * @brief unmute anc path at frequency raising
 *
 * @param src_mode current clock mode
 * @param dst_mode next clock mode
 */
void audio_anc_switch_clock_unmute_hook(uint32_t src_mode, uint32_t dst_mode);

/**
 * @brief open spk through ANC
 * @return the speaker of start ref count.
 */
uint8_t audio_anc_spk_open(AUDIO_SPK_CONTROL from);

/**
 * @brief close spk through ANC
 * @return the speaker of stop ref count.
 */
uint8_t audio_anc_spk_close(AUDIO_SPK_CONTROL from);

/**
 * @brief get ANC open/close state
 * @return true is open else false is close.
 */
bool audio_get_anc_state(void);

/**
 * @brief set ANC drc limit
 * @param drc is the limit anc drc num
 * @return true is OK else error.
 */
uint8_t audio_anc_drc_set(int8_t drc);

/**
 * @brief audio_anc_coeff_idx_cfg
 * @param coeff_cfg config ANC FF/fb mic coeff idx
 * @return true is OK else error.
 */
uint8_t audio_anc_coeff_idx_cfg(anc_coeff_idx_cfg *coeff_cfg);

/**
 * @brief audio_anc_user_coeff_gain_get
 * @param mode get the mode of coeff gain
 * @return usr current cfg user gain
 */
uint8_t audio_anc_user_coeff_gain_get(AUDIO_DENOISE_MODE mode);

/**
 * @brief audio_anc_user_coeff_gain_get
 * @param mode get the mode of coeff gain
 * @param usr_gain set usr current cfg user gain
 * @return true is OK else error.
 */
uint8_t audio_anc_user_coeff_gain_set(AUDIO_DENOISE_MODE mode, uint8_t usr_gain);

/**
 * @brief audio_anc_coeff_switch_done_register
 * @param cb when anc coeff done call this cb, it's in timer task.
 * @return true is OK else error.
 */
uint8_t audio_anc_coeff_switch_done_register(audio_anc_switch_done_handle_cb cb);

/**
 * @brief audio_noise_detection_init.
 */
void audio_noise_detection_init(void);

/**
 * @brief sudio_noise_detection_deal.
 * @param data check noise detect data ptr.
 * @param len check noise detect data length.
 * @return 0 is not noise, 1 is noise data.
 */
uint8_t audio_noise_detection_deal(int16_t *data, uint16_t len);

/**
 * @brief audio_anc_cfg_fb_coeff.
 * @param cfg config fb mic pass through or real coeff.
 * @return RET_OK is cfg ok else if error.
 */
uint8_t audio_anc_cfg_coeff(uint8_t cfg);

/**
 * @brief audio_anc_callback_bypass_set.
 * @param anc_callback_bypass_flag 0 need call register cb when anc coeff switch
 *         done else not call the callback.
 * @return RET_OK is cfg ok else if error.
 */
uint8_t audio_anc_callback_bypass_set(uint8_t anc_callback_bypass_flag);

/**
 * @brief audio_anc_noise_handle_register
 * @param cb when anc open then check noise call this cb, then need close anc.
 * @return true is OK else error.
 */
uint8_t audio_anc_noise_handle_register(audio_anc_noise_handle_cb cb);

/**
 * @brief audio_anc_howlround_enable
 * @param enable enable/disable howlround detect.
 * @return true is OK else error.
 */
uint8_t audio_anc_howlround_enable(bool enable);

/**
 * @brief audio_anc_howlround_cfg
 * @param buf param of anc noise check
 * @param len the buf of len
 * @return true is OK else error.
 */
uint8_t audio_anc_howlround_cfg(uint8_t *buf, uint8_t len);
#endif
