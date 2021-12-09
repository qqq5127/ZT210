#include "types.h"
#include "stdio.h"
#include "version.h"
#include "riscv_cpu.h"
#include "os_utils.h"
#include "os_task.h"
#include "os_hook.h"
#include "os_systick.h"
#include "sections.h"
#include "mmon.h"

#include "iot_irq.h"
#include "iot_debug.h"
#include "iot_soc.h"
#include "iot_dsp.h"
#include "iot_rtc.h"
#include "iot_ipc.h"
#include "iot_timer.h"
#include "iot_gpio.h"
#include "iot_share_task.h"
#include "iot_rpc.h"
#include "iot_loader.h"
#include "iot_wdt.h"
#include "iot_dma.h"
#include "iot_charger.h"
#include "iot_resource.h"
#include "iot_wic.h"
#include "iot_lpm.h"
#include "iot_uart.h"
#include "iot_adc.h"

#include "string.h"
#include "power_mgnt.h"
#include "dev_pm.h"
#include "lib_dbglog.h"

#include "key_value.h"
#include "iot_flash.h"
#include "iot_cache.h"
#include "storage_controller.h"
#include "iot_debounce.h"
#include "iot_touch_key.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "caldata.h"

#ifdef LIB_DFS_ENABLE
#include "dfs.h"
#endif

#ifdef LIB_AUDIO_ENABLE
#include "aud_sys.h"
#endif

#ifdef LIB_PLAYER_ENABLE
#include "nplayer_api.h"
#endif

#ifdef LIB_CLI_ENABLE
#include "cli.h"
#endif
#ifdef LIB_DBGLOG_CACHE_ENABLE
#include "dbglog_cache.h"
#endif

#ifdef LIB_KEY_MGMT_ENABLE
#include "key_mgmt.h"
#endif

#ifdef LIB_BATTERY_ENABLE
#include "battery.h"
#endif

#ifdef DBG_BUS_CFG
#include "iot_dbus.h"
#endif

#ifdef BUILD_PATCH
#include "patch_main.h"
extern const uint32_t rom_lib_version;
#endif

#ifndef DEEPSLP_THRE_MS
#define DEEPSLP_THRE_MS 60
#endif
#ifndef LIGHTSLP_THRE_MS
#define LIGHTSLP_THRE_MS 0xFFFFFFFF
#endif

static const task_priority_t task_list[] = {
    {"share_task_fast", 10},   //
    {"app_entry", 9},          //
#ifdef LIB_AUDIO_ENABLE
    {"audio", 9},      //
#endif                 /* LIB_AUDIO_ENABLE */
    {"dp_task", 8},    //
    {"rpc_task", 10},   //
    {"cli", 6},        //
    {"app_main", 6},   //
    {"share_task_slow", 4},
};

extern uint32_t _iram_text_start;
extern uint32_t _iram_text_end;
/*lint -esym(526, _iram_text_load_addr) defined at link script */
extern uint32_t _iram_text_load_addr;

static void ram_text_init(void)
{
    uint32_t *start = (uint32_t *)&_iram_text_start;
    uint32_t *end = (uint32_t *)&_iram_text_end;
    uint32_t *load = (uint32_t *)&_iram_text_load_addr;

    while (start != end) {
        *start = *load;
        start++;
        load++;
    }
}

static void data_section_init(void)
{
    uint8_t *start = (uint8_t *)&_data_start;
    uint8_t *end = (uint8_t *)&_data_end;
    uint8_t *load = (uint8_t *)&_data_load_addr;
    size_t len = (size_t)(end - start);

    memcpy(start, load, len);
}

static void bss_section_init(void)
{
    uint8_t *start = (uint8_t *)&_bss_start;
    uint8_t *end = (uint8_t *)&_bss_end;
    size_t len = (size_t)(end - start);

    memset(start, 0x0, len);
}

static void os_hook_init(void)
{
    os_systick_register_callback(iot_systick_init, iot_systick_clear);
    os_hook_sleep_register_callback(power_mgnt_process_sleep);
    os_hook_time_stamp_register_callback(iot_rtc_get_time);
    os_hook_sys_save_register_callback(iot_dev_save);
    os_hook_sys_restore_register_callback(iot_dev_restore);

    os_hook_failed_dump_register_callback(power_mgnt_failed_dump);
}

/*lint -esym(714, software_init) referenced at start.S */
void software_init(void);
void software_init(void)
{
    /* ram text section copy */
    ram_text_init();

    /*data section copy*/
    data_section_init();

    /*bss section init*/
    bss_section_init();

#ifdef BUILD_PATCH
    /* rom init */
    patch_init();
#endif

    riscv_cpu_controller_interrupt_init();
    cpu_enable_irq();

    /* zero address access disabled */
    mmon_disable_address_access(0x0, 0x40);
}

#ifdef PRINTF_DEBUG
int putchar(int c);
int putchar(int c)
{
    iot_uart_putc(IOT_UART_PORT_1, c);
    return c;
}

static void debug_print_uart_init(void)
{
    iot_uart_configuration_t uart_cfg;
    uart_cfg.baud_rate = 2000000;
    uart_cfg.data_bits = IOT_UART_DATA_BITS_8;
    uart_cfg.parity = IOT_UART_PARITY_NONE;
    uart_cfg.stop_bits = IOT_UART_STOP_BITS_1;

    iot_uart_pin_configuration_t pin_cfg;
    pin_cfg.tx_pin = 73;
    pin_cfg.rx_pin = 74;

    iot_uart_init(IOT_UART_PORT_1);
    iot_uart_open(IOT_UART_PORT_1, &uart_cfg, &pin_cfg);
}
#endif

static void wdt_idle_hook_cb(void)
{
    if (iot_wdt_need_feed()) {
        /* can't use mutex as dbglog in idle */
        //        dbg_save_fmt_log("core%d feed dog\n", cpu_get_mhartid(), 0, 0);
        iot_wdt_do_feed();
    }
}

static void show_version_task(void *arg)
{
    UNUSED(arg);
#ifdef BUILD_PATCH
    // Print the rom version
    // main function use modile id 0
    DBGLOG_LIB_INFO("ROM LIB VERSION %d.%d\n", rom_lib_version >> 16, rom_lib_version & 0xFFFF);
#endif

    const char *ver_str = version_get_global_version();
    DBGLOG_LIB_RAW("%s\n", ver_str);

#if !defined(NDEBUG) && !defined(RELEASE)
    DBGLOG_LIB_RAW("%s - %s\n", FIRMWARE_BUILD_COMMIT, FIRMWARE_BUILD_USER);
#endif
}

static void main_os_init(void)
{
    os_heap_region_t heap[2] = {0};

    heap[0].start = (uint8_t *)&_heap_start;
    heap[0].length = (uint32_t)&_heap_size;

    os_utils_init((uint32_t)&__stack_top, heap);
    os_task_init(task_list, ARRAY_SIZE(task_list));
    os_hook_init();
}

static void main_hardware_init(void)
{
    iot_rtc_global_init();
    iot_rtc_init();

    iot_gpio_init();
#ifdef DBG_BUS_CFG
    iot_dbus_init();
#endif
    iot_debounce_init();
    iot_wic_init();
    iot_dma_init();
    iot_adc_init();
    iot_ipc_init();
    iot_timer_init();
    iot_charger_init();
    iot_flash_init();

    iot_wdt_init();
    iot_wdt_global_init();
    iot_touch_key_init();
    os_hook_idle_register_callback(wdt_idle_hook_cb);
}

static void main_chip_init(void)
{
    if (cal_data_ana_load() == RET_OK) {
        early_printf("cal data load successfully.\n");
    } else {
        early_printf("cal data load failed.\n");
    }
    iot_cache_restore_register();
    iot_soc_chip_init(true);
    iot_irq_init();
    iot_debug_init();
    power_mgnt_init(DEEPSLP_THRE_MS, LIGHTSLP_THRE_MS);
}

static void main_lib_init(void)
{
    // Debug log init the log share memory, so it must init before logger task
    dbglog_init();
    generic_transmission_init();

    int32_t ret = iot_rpc_init();
    assert(ret == RET_OK);
    iot_share_task_init();

    key_value_init();
    storage_init();

#ifdef LIB_DFS_ENABLE
    dfs_init_cfg_t dfs_init_cfg;

    dfs_init_cfg.init_obj = DFS_OBJ_SHARED_ATLEAST_32_32_32M;
    dfs_init_cfg.init_timeout_ms = 10000;

    dfs_init(&dfs_init_cfg);
#endif

#ifdef LIB_BATTERY_ENABLE
    battery_init();
#endif

#ifdef LIB_AUDIO_ENABLE
    audio_sys_init();
#endif

#ifdef NEW_ARCH   //LIB_PLAYER_ENABLE
    player_init(NULL);
#endif

    os_create_task_ext(show_version_task, NULL, 7, 512, "show_version_task");

#ifdef APP_TWS_ENABLE
    extern void app_entry(void *arg);
    os_create_task_ext(app_entry, NULL, 7, 512, "app_entry");
#endif

#if defined(LIB_CLI_ENABLE)
    cli_config_t cli_config = {
        .cli_task_size = 512,
        .cli_prio = 6,
    };
    cli_init(&cli_config);
#endif
    cal_data_ana_cfg_dump();
#ifdef LIB_DBGLOG_CACHE_ENABLE
    //dbglog_cache_init use share task. must be initialized after share task
    dbglog_cache_init(LOG_BUFFER_SIZE);
#endif
}

int main(void)
{
    main_chip_init();
    main_os_init();
    main_hardware_init();
    main_lib_init();

#ifdef PRINTF_DEBUG
    debug_print_uart_init();
    printf("debug printf uart init done.\n");
#endif
    iot_loader_load_dsp();
    uint32_t entry = iot_loader_load_bt();

    iot_soc_bt_start(entry);
    iot_dsp_start();

//USAGE:   scons app=pkg_patch_tws_1_1_debug_bus def=DBG_BUS_CFG -j32
#ifdef DBG_BUS_CFG
    // using the general debug way
    uint8_t group = 4;
    uint8_t dbus_sel = 7;
    uint8_t sub_dbus_sel = 0;
    // 0: G00 3: G03  63: A00 64: A01 71~74: A0~A3
    uint8_t gpio[] = {0, 3, 63, 64, 71, 72, 73, 74};
    uint32_t mask = 0x000000FF;

    /*  JTAG will reuse below gpio
    *      0: G00, 3: G03, 63: A00, 64: A01, 71 A0,
    *  CLI will reuse below gpio
    *      0: G00, 3: G03,
    */
    iot_dbus_general_init(group, dbus_sel, sub_dbus_sel, mask, gpio, sizeof(gpio));

    /* using the AON gpio 92 as the dbus signal output way,
     * only for pmm domain dbus signal

    uint8_t sub_dbus_sel = 5;
    uint8_t internal_sig_bit = 21;
    iot_dbus_pmm_config(sub_dbus_sel, internal_sig_bit);
     */
#endif
    os_start_kernel();
}
