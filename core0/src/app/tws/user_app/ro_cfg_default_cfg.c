#include "types.h"
#include "os_mem.h"
#include "string.h"
#include "modules.h"
#include "ro_cfg.h"

/** temp functions before config tool is ready */
void ro_cfg_default_general(ro_cfg_general_t *general);
void ro_cfg_default_system(ro_cfg_system_t *system);
void ro_cfg_default_volume(ro_cfg_volume_t *volume);
void ro_cfg_default_battery(ro_cfg_battery_t *battery);
void ro_cfg_default_timeout(ro_cfg_timeout_t *timeout);

void ro_cfg_default_general(ro_cfg_general_t *general)
{
    memset(general, 0, sizeof(ro_cfg_general_t));

    general->default_listen_mode = RO_LISTEN_MODE_NORMAL;
    general->default_listen_mode_toggle_config = RO_LISTEN_MODE_TOGGLE_NORMAL_ANC_TRANSPARENCY;
}

void ro_cfg_default_system(ro_cfg_system_t *system)
{
    memset(system, 0, sizeof(ro_cfg_system_t));

    system->power_on_reconnect_retry_count = 4;
    system->link_loss_reconnect_retry_count = 4;

    system->features.auto_power_off = 0;
    system->features.sync_sys_state = 1;
    system->features.auto_reconnect_power_on = 1;
    system->features.queue_tone = 0;
}

void ro_cfg_default_volume(ro_cfg_volume_t *volume)
{
    memset(volume, 0, sizeof(ro_cfg_volume_t));

    volume->default_call_volume = 12;
    volume->default_music_volume = 12;
    volume->default_tone_volume = 12;
    volume->default_mic_gain = 0;
    volume->volume_level_num = 16;

    volume->volume_level_num = 16;
    static const struct ro_cfg_volume_level vol_lvls[16] = {
        {-45, -45, -45},   //0
        {-42, -42, -42},   //1
        {-39, -39, -39},   //2
        {-36, -36, -36},   //3
        {-33, -33, -33},   //4
        {-30, -30, -30},   //5
        {-27, -27, -27},   //6
        {-24, -24, -24},   //7
        {-21, -21, -21},   //8
        {-18, -18, -18},   //9
        {-15, -15, -15},   //10
        {-12, -12, -12},   //11
        {-9, -9, -9},      //12
        {-6, -6, -6},      //13
        {-3, -3, -3},      //14
        {0, 0, 0},         //15
    };
    memcpy(volume->volume_levels, vol_lvls, sizeof(volume->volume_levels));
}

void ro_cfg_default_battery(ro_cfg_battery_t *battery)
{
    memset(battery, 0, sizeof(ro_cfg_battery_t));

    battery->battery_low_event_voltage = 3400;
    battery->battery_low_power_off_voltage = 3100;
    battery->battery_low_prompt_interval = 20;
    battery->battery_level_check_interval = 20;

    battery->battery_level_voltages[0] = 3000;
    battery->battery_level_voltages[1] = 3100;
    battery->battery_level_voltages[2] = 3200;
    battery->battery_level_voltages[3] = 3300;
    battery->battery_level_voltages[4] = 3400;
    battery->battery_level_voltages[5] = 3500;
    battery->battery_level_voltages[6] = 3600;
    battery->battery_level_voltages[7] = 3700;
    battery->battery_level_voltages[8] = 3800;
    battery->battery_level_voltages[9] = 3900;
}

void ro_cfg_default_timeout(ro_cfg_timeout_t *timeout)
{
    memset(timeout, 0, sizeof(ro_cfg_timeout_t));

    timeout->auto_power_off = 300;
    timeout->disable_power_off_after_power_on = 5;
    timeout->link_loss_reconnect_interval = 5;
    timeout->power_on_reconnect_interval = 5;
}
