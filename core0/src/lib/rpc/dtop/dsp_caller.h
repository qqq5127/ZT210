/* Auto Generated File! DO NOT EDIT! */

#ifndef DSP_CALLER_H
#define DSP_CALLER_H

#include "types.h"

uint32_t rpc_dsp_test(void);
uint32_t rpc_dsp_create_stream(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t rpc_dsp_destory_stream(uint32_t);
uint32_t rpc_dsp_config_voice_param(uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t rpc_dsp_start_tone(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t rpc_dsp_stop_tone(uint32_t);
uint32_t rpc_dsp_set_vol(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
uint8_t rpc_dbglog_set_dsp_log_level(uint8_t, uint8_t);
uint32_t rpc_dsp_config_voice_module_switch(uint32_t);
uint32_t rpc_dsp_sync_sys_info(uint32_t, uint32_t, uint32_t, uint32_t);

#endif