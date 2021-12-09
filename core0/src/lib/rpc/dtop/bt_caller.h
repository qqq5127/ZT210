/* Auto Generated File! DO NOT EDIT! */

#ifndef BT_CALLER_H
#define BT_CALLER_H

#include "types.h"

uint32_t rpc_bt_handle_user_cmd(uint32_t, uint32_t, uint32_t);
void rpc_bt_handle_user_shutdown(void);
void rpc_asrc_play_error_handler(uint32_t, uint32_t);
uint8_t rpc_power_mgnt_set_sleep_thr(uint32_t, uint32_t);
uint8_t rpc_dbglog_set_log_level(uint8_t, uint8_t);
void rpc_btcore_stream_resume(uint32_t);
void rpc_btcore_player_restart_req(uint32_t, uint32_t);
uint8_t rpc_player_delay_mode_set(uint8_t);
uint8_t rpc_player_delay_mode_config(uint32_t);

#endif