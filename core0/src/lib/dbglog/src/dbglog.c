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
/* os shim includes */
#include "types.h"
#include "stdio.h"
#include "string.h"
#include "modules.h"
#include "riscv_cpu.h"

#include "iot_rtc.h"
#include "iot_timer.h"

#include "os_lock.h"
#include "os_mem.h"
#include "os_task.h"

#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "dbglog.h"
#include "storage_controller.h"

#ifdef BUILD_CORE_CORE0
#include "rpc_caller.h"
#endif

#if defined(BUILD_CORE_CORE0)
#define LOG_CORE_SELF 0
#elif defined(BUILD_CORE_CORE1)
#define LOG_CORE_SELF 1
#else
#define LOG_CORE_SELF 2
#endif

#define DBGLOG_MAX_LENGTH 256

#ifndef DBGLOG_LEVEL_DEFAULT
#if defined(RELEASE)
#define DBGLOG_LEVEL_DEFAULT DBGLOG_LEVEL_NONE
#elif defined(DEVELOPMENT)
#define DBGLOG_LEVEL_DEFAULT DBGLOG_LEVEL_ALL
#else
#define DBGLOG_LEVEL_DEFAULT DBGLOG_LEVEL_ALL
#endif
#endif

#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint8_t global_log_level;
    uint8_t coredump_mode:2;
    uint8_t coredump_io:4;
    uint8_t reserved_bits:2;
    uint8_t reserved[1];
} dbglog_cfg_t;
#pragma pack(pop)

/*lint -esym(749, DBGLOG_CFG_ID_MAX) not referenced */
enum {
    DBGLOG_CFG_ID = LIB_DBGLOG_BASE_ID,
    DBGLOG_CFG_ID_MAX = LIB_DBGLOG_END_ID,
};

static char dbglog_temp_buffer[DBGLOG_MAX_LENGTH];
static uint8_t dbglog_level_ctrl[MAX_MID_NUM];
static uint8_t dbglog_coredump_mode;
static uint8_t dbglog_coredump_io;
#ifdef BUILD_CORE_CORE0
static dbglog_cfg_t dbglog_cfg = {0};
#endif

static uint16_t dbglog_get_sequence_number(void) IRAM_TEXT (dbglog_get_sequence_number);
static uint16_t dbglog_get_sequence_number(void)
{
    static uint16_t dbglog_sequence_number = 0;
    return (dbglog_sequence_number++ & 0x03FF);
}

static bool_t dbglog_pack_time_seq(const uint8_t* buffer) IRAM_TEXT (dbglog_pack_time_seq);
static bool_t dbglog_pack_time_seq(const uint8_t* buffer)
{
    dbglog_common_header_t *common_header = (dbglog_common_header_t *)buffer; /*lint !e826 Area is not too small */
    if (common_header->core_id != LOG_CORE_SELF || common_header->version != DBGLOG_VERSION) {
        return false;
    }
    common_header->timestamp = iot_rtc_get_global_time();
    common_header->sequence_num = dbglog_get_sequence_number();
    return true;
}

#ifdef BUILD_CORE_CORE0
static int32_t dbglog_update_task_prio(void)
{
    if( dbglog_level_ctrl[UNKNOWN_MID] <= DBGLOG_LEVEL_VERBOSE ) {
        return generic_transmission_set_priority(CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO_HIGH);
    }
    else {
        return generic_transmission_restore_priority();
    }
}
#endif

void dbglog_init(void)
{
    dbglog_level_ctrl[UNKNOWN_MID] = DBGLOG_LEVEL_DEFAULT;
    for (uint8_t i = 1; i < MAX_MID_NUM; i++) {
        dbglog_level_ctrl[i] = DBGLOG_LEVEL_ALL;
    }

    generic_transmission_register_repack_callback(GENERIC_TRANSMISSION_DATA_TYPE_STREAM_LOG, dbglog_pack_time_seq);
    generic_transmission_register_repack_callback(GENERIC_TRANSMISSION_DATA_TYPE_RAW_LOG, dbglog_pack_time_seq);
}

void dbglog_config_load(void)
{
#ifdef BUILD_CORE_CORE0
    uint32_t ret;
    uint32_t cfg_len = sizeof(dbglog_cfg);

    ret = storage_read(LIB_DBGLOG_BASE_ID, DBGLOG_CFG_ID, (void *)(&dbglog_cfg), &cfg_len);
    if (ret != RET_OK || cfg_len != sizeof(dbglog_cfg)) {
        dbglog_level_ctrl[UNKNOWN_MID] = DBGLOG_LEVEL_DEFAULT;
    } else {
        dbglog_level_ctrl[UNKNOWN_MID] = dbglog_cfg.global_log_level;
        dbglog_coredump_mode = dbglog_cfg.coredump_mode;
        dbglog_coredump_io = dbglog_cfg.coredump_io;
        rpc_dbglog_set_log_level(UNKNOWN_MID, dbglog_level_ctrl[UNKNOWN_MID]);
        rpc_dbglog_set_dsp_log_level(UNKNOWN_MID, dbglog_level_ctrl[UNKNOWN_MID]);
    }
    dbglog_update_task_prio();
#endif
    return;
}

uint8_t dbglog_get_dump_mode(void)
{
    return (dbglog_coredump_mode == 0)?DBGLOG_DUMP_FULL:DBGLOG_DUMP_MINI;
}

static int dbglog_gtp_write_buffer(bool_t irq_context, int log_type, const uint8_t *buffer, uint32_t length) IRAM_TEXT (dbglog_gtp_write_buffer);
static int dbglog_gtp_write_buffer(bool_t irq_context, int log_type, const uint8_t *buffer, uint32_t length)
{
    uint32_t remain_len = length;
    int32_t ret;
    do {
        if (irq_context) {
            ret = generic_transmission_data_tx_critical(GENERIC_TRANSMISSION_TX_MODE_LAZY,
                    (generic_transmission_data_type_t)log_type,
                    DBGLOG_TID,
                    GENERIC_TRANSMISSION_IO_UART0,
                    buffer + length - remain_len,
                    remain_len,
                    false);
        } else {
            ret = generic_transmission_data_tx(GENERIC_TRANSMISSION_TX_MODE_LAZY,
                                               (generic_transmission_data_type_t)log_type,
                                               DBGLOG_TID,
                                               GENERIC_TRANSMISSION_IO_UART0,
                                               buffer + length - remain_len,
                                               remain_len,
                                               false);
        }

        if (ret >= 0) {
            remain_len -= (uint32_t)ret;
        }
        if (ret < 0) {
            break;
        }
    } while (remain_len > 0);

    if (ret < 0) {
        return ret;
    }
    return 0;
}

uint8_t dbglog_set_log_level(uint8_t module, uint8_t level)
{
    uint32_t ret = RET_OK;

    if (level >= DBGLOG_LEVEL_MAX) {
        return RET_INVAL;
    }

    if (module == MAX_MID_NUM) {
        for (uint8_t i = 0; i < MAX_MID_NUM; i++) {
            dbglog_level_ctrl[i] = DBGLOG_LEVEL_ALL;
        }
    } else {
        dbglog_level_ctrl[module] = level;
    }

#ifdef BUILD_CORE_CORE0
    if (module == UNKNOWN_MID || module == MAX_MID_NUM) {
        dbglog_cfg.global_log_level = level;
        ret = storage_write(LIB_DBGLOG_BASE_ID, DBGLOG_CFG_ID, (void *)&dbglog_cfg,
                            sizeof(dbglog_cfg));
    }
    rpc_dbglog_set_log_level(module, level);
    rpc_dbglog_set_dsp_log_level(module, level);
    dbglog_update_task_prio();
#endif
    return (uint8_t)ret;
}

/*lint -sem(dbglog_stream_log_write, thread_protected) */
static uint8_t dbglog_stream_log_write_wrap(uint8_t module_id, DBGLOG_LEVEL level, uint16_t file_id,
                                       uint16_t line_num, uint32_t param_num, va_list ap)
    IRAM_TEXT(dbglog_stream_log_write_wrap);
static uint8_t dbglog_stream_log_write_wrap(uint8_t module_id, DBGLOG_LEVEL level, uint16_t file_id,
                                       uint16_t line_num, uint32_t param_num, va_list ap)
{
    uint32_t loop = 0;
    char *buffer = NULL;
    dbglog_stream_log_header_t header;

    if (level < dbglog_level_ctrl[IOT_BASIC_MID_START] ||
        level < dbglog_level_ctrl[module_id] ||
        generic_transmission_in_panic()) {
        return RET_FAIL;
    }

    /* In task context use a heap buffer for log output, and ISR context use global buffer.
    because the API dbglog_gtp_write_buffer is thread safe and interruption can't nesting,
    so the Mutex and critical protect is not needed */

    if (!in_irq() && cpu_get_int_enable() && in_scheduling()) {
        buffer = os_mem_malloc(IOT_DBGLOG_MID, DBGLOG_MAX_LENGTH);
        if (buffer == NULL) {
            dbglog_get_sequence_number();
            return RET_NOMEM;
        }
    } else {
        buffer = dbglog_temp_buffer;
    }

    if (param_num != 0) {
        if (param_num > (int)((DBGLOG_MAX_LENGTH - sizeof(dbglog_stream_log_header_t)) / 4)) {
            param_num = (DBGLOG_MAX_LENGTH - sizeof(dbglog_stream_log_header_t)) / 4;
        }
        for (loop = 0; loop < param_num; loop++) {
            int param = va_arg(ap, int);
            memcpy(&buffer[sizeof(dbglog_stream_log_header_t) + loop * 4], &param, sizeof(int));
        }
    }

    header.common_header.core_id = LOG_CORE_SELF;
    header.common_header.version = DBGLOG_VERSION;
    header.common_header.payload_length = (uint16_t)(param_num * 4);
    header.level = level;
    header.module_id = module_id;
    header.file_id = file_id;
    header.line_num = line_num;
    memcpy(buffer, &header, sizeof(dbglog_stream_log_header_t));
    int ret = dbglog_gtp_write_buffer((in_irq() || (!cpu_get_int_enable())), GENERIC_TRANSMISSION_DATA_TYPE_STREAM_LOG,
                                  (const uint8_t *)buffer,
                                  sizeof(dbglog_stream_log_header_t) + param_num * 4);
    if (buffer != dbglog_temp_buffer) {
        os_mem_free(buffer);
    }

    if (ret == -RET_INVAL || ret == -RET_NOMEM) {
        dbglog_get_sequence_number();
    }

    return (ret == 0) ? RET_OK : RET_FAIL;
} /*lint !e818 ap could not be declared as pointing to const */

uint8_t dbglog_raw_log_write(uint8_t module_id, const char *format, ...) IRAM_TEXT(dbglog_raw_log_write);
uint8_t dbglog_raw_log_write(uint8_t module_id, const char *format, ...)
{
    char *buffer = NULL;

    if (dbglog_level_ctrl[IOT_BASIC_MID_START] == DBGLOG_LEVEL_NONE ||
        dbglog_level_ctrl[module_id] == DBGLOG_LEVEL_NONE ||
        generic_transmission_in_panic()) {
        return RET_FAIL;
    }

    /* In task context use a heap buffer for log output, and ISR context use global buffer.
    because the API dbglog_gtp_write_buffer is thread safe and interruption can't nesting,
    so the Mutex and critical protect is not needed */

    if (!in_irq() && cpu_get_int_enable() && in_scheduling()) {
        buffer = os_mem_malloc(IOT_DBGLOG_MID, DBGLOG_MAX_LENGTH);
        if (buffer == NULL) {
            dbglog_get_sequence_number();
            return RET_NOMEM;
        }
    } else {
        buffer = dbglog_temp_buffer;
    }

    va_list ap;
    va_start(ap, format);
    int32_t length = vsnprintf(buffer + (sizeof(dbglog_raw_log_header_t)),
                               (DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t)), format, ap);
    va_end(ap);

    if (length < 0) {
        if (buffer != dbglog_temp_buffer) {
            os_mem_free(buffer);
        }
        return RET_FAIL;
    } else if (length >= (int)(DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t))) {
        // Remove '\0'
        length = (DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t)) - 1;
    } else {
    }

    dbglog_raw_log_header_t header;
    header.common_header.core_id = LOG_CORE_SELF;
    header.common_header.version = DBGLOG_VERSION;
    header.common_header.payload_length = (uint16_t)length;
    header.module_id = module_id;

    memcpy((char *)&buffer[0], (const char *)&header, sizeof(dbglog_raw_log_header_t));
    int ret = dbglog_gtp_write_buffer((in_irq() || (!cpu_get_int_enable())), GENERIC_TRANSMISSION_DATA_TYPE_RAW_LOG,
                                      (const uint8_t *)buffer,
                                      sizeof(dbglog_raw_log_header_t) + (uint32_t)length);
    if (buffer != dbglog_temp_buffer) {
        os_mem_free(buffer);
    }
    if (ret == -RET_INVAL || ret == -RET_NOMEM) {
        dbglog_get_sequence_number();
    }
    return (ret == 0) ? RET_OK : RET_FAIL;
}

/**
 * Write buffer with no os api called
 * Only use crash or other interruption context !!!
 */
uint8_t dbglog_crash_log_write(const uint8_t *buffer, uint8_t length) IRAM_TEXT(dbglog_crash_log_write);
uint8_t dbglog_crash_log_write(const uint8_t *buffer, uint8_t length)
{
    if (generic_transmission_get_status(cpu_get_mhartid()) < GTB_ST_IN_PANIC) {
        generic_transmission_panic_start();
    }

    int ret = RET_OK;
    if (length >= DBGLOG_MAX_LENGTH - sizeof(dbglog_crash_log_header_t)) {
        // Remove '\0'
        length = (DBGLOG_MAX_LENGTH - sizeof(dbglog_crash_log_header_t)) - 1;
    }

    dbglog_crash_log_header_t header;
    header.common_header.core_id = LOG_CORE_SELF;
    header.common_header.version = DBGLOG_VERSION;
    header.common_header.timestamp = iot_rtc_get_global_time();
    header.common_header.sequence_num = dbglog_get_sequence_number();
    header.common_header.payload_length = length;
    header.module_id = UNKNOWN_MID;
    memcpy(dbglog_temp_buffer, &header, sizeof(dbglog_crash_log_header_t));
    memcpy(&dbglog_temp_buffer[sizeof(dbglog_crash_log_header_t)], buffer, length);
    uint32_t remain_len = sizeof(dbglog_crash_log_header_t) + length;
    do {
        ret = generic_transmission_data_tx_panic(GENERIC_TRANSMISSION_TX_MODE_LAZY,
                GENERIC_TRANSMISSION_DATA_TYPE_PANIC_LOG,
                DBGLOG_TID,
                GENERIC_TRANSMISSION_IO_UART0,
                (const uint8_t *)dbglog_temp_buffer + sizeof(dbglog_crash_log_header_t) + length - remain_len,
                remain_len);
        iot_timer_delay_ms(1);
        if (ret >= 0) {
            remain_len -= (uint32_t)ret;
        }
        if (ret == -RET_NOMEM) {
            continue;
        }
        if (ret < 0) {
            break;
        }
    } while (remain_len > 0);

    if (dbglog_coredump_io & DBGLOG_DUMP_FLASH) {
        remain_len = sizeof(dbglog_crash_log_header_t) + length;
        do {
            ret = generic_transmission_data_tx_panic(GENERIC_TRANSMISSION_TX_MODE_LAZY,
                    GENERIC_TRANSMISSION_DATA_TYPE_PANIC_LOG,
                    DBGLOG_TID,
                    GENERIC_TRANSMISSION_IO_FLASH,
                    (const uint8_t *)dbglog_temp_buffer + sizeof(dbglog_crash_log_header_t) + length - remain_len,
                    remain_len);
            iot_timer_delay_ms(1);
            if (ret >= 0) {
                remain_len -= (uint32_t)ret;
            }
            if (ret == -RET_NOMEM) {
                continue;
            }
            if (ret < 0) {
                break;
            }
        } while (remain_len > 0);
    }
    return (remain_len == 0) ? RET_OK : RET_FAIL;
}

uint8_t dbglog_buffer_write_crash(const uint8_t *buffer, uint8_t length) IRAM_TEXT(dbglog_buffer_write_crash);
uint8_t dbglog_buffer_write_crash(const uint8_t *buffer, uint8_t length)
{
    return dbglog_crash_log_write(buffer, length);
}

uint8_t dbglog_wrap(const char *file_name, uint32_t line, DBGLOG_LEVEL level,
                    const char *format, ...) IRAM_TEXT(dbglog_wrap);
uint8_t dbglog_wrap(const char *file_name, uint32_t line, DBGLOG_LEVEL level,
                    const char *format, ...)
{
    UNUSED(file_name);
    UNUSED(line);
    UNUSED(level);
    char *buffer = NULL;

    if (level < dbglog_level_ctrl[UNKNOWN_MID] || generic_transmission_in_panic()) {
        return RET_FAIL;
    }
    /* In task context use a heap buffer for log output, and ISR context use global buffer.
       because the API dbglog_gtp_write_buffer is thread safe and interruption can't nesting,
       so the Mutex and critical protect is not needed */
    if (!in_irq() && cpu_get_int_enable() && in_scheduling()) {
        buffer = os_mem_malloc(IOT_DBGLOG_MID, DBGLOG_MAX_LENGTH);
        if (buffer == NULL) {
            dbglog_get_sequence_number();
            return RET_NOMEM;
        }
    } else {
        buffer = dbglog_temp_buffer;
    }

    va_list ap;
    va_start(ap, format);
    int32_t length = vsnprintf(buffer + (sizeof(dbglog_raw_log_header_t)), DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t), format, ap);
    va_end(ap);

    if (length < 0) {
        if (buffer != dbglog_temp_buffer) {
            os_mem_free(buffer);
        }
        return RET_FAIL;
    } else if (length >= (int)(DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t))) {
        // Remove '\0'
        length = (DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t)) - 1;
    } else {
    }

    dbglog_raw_log_header_t header;
    header.common_header.core_id = LOG_CORE_SELF;
    header.common_header.version = DBGLOG_VERSION;
    header.common_header.payload_length = (uint16_t)length;
    header.module_id = UNKNOWN_MID;

    memcpy((char *)&buffer[0], (const char *)&header, sizeof(dbglog_raw_log_header_t));
    int ret = dbglog_gtp_write_buffer((in_irq() || (!cpu_get_int_enable())), GENERIC_TRANSMISSION_DATA_TYPE_RAW_LOG,
                                      (const uint8_t *)buffer,
                                      sizeof(dbglog_raw_log_header_t) + (uint32_t)length);
    if (buffer != dbglog_temp_buffer) {
        os_mem_free(buffer);
    }
    return (ret == 0) ? RET_OK : RET_FAIL;
}

uint8_t dbglog(const char *file_name, uint32_t line, DBGLOG_LEVEL level,
               const char *format, ...) IRAM_TEXT(dbglog);
uint8_t dbglog(const char *file_name, uint32_t line, DBGLOG_LEVEL level,
               const char *format, ...)
{
    UNUSED(file_name);
    UNUSED(line);
    UNUSED(level);
    char *buffer = NULL;

    if (level < dbglog_level_ctrl[UNKNOWN_MID] || generic_transmission_in_panic()) {
        return RET_FAIL;
    }

    /* In task context use a heap buffer for log output, and ISR context use global buffer.
       because the API dbglog_gtp_write_buffer is thread safe and interruption can't nesting,
       so the Mutex and critical protect is not needed */
    if (!in_irq() && cpu_get_int_enable() && in_scheduling()) {
        buffer = os_mem_malloc(IOT_DBGLOG_MID, DBGLOG_MAX_LENGTH);
        if (buffer == NULL) {
            dbglog_get_sequence_number();
            return RET_NOMEM;
        }
    } else {
        buffer = dbglog_temp_buffer;
    }

    va_list ap;
    va_start(ap, format);
    int32_t length = vsnprintf(buffer + (sizeof(dbglog_raw_log_header_t)), DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t), format, ap);
    va_end(ap);

    if (length < 0) {
        if (buffer != dbglog_temp_buffer) {
            os_mem_free(buffer);
        }
        return RET_FAIL;
    } else if (length >= (int)(DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t))) {
        // Remove '\0'
        length = (DBGLOG_MAX_LENGTH - sizeof(dbglog_raw_log_header_t)) - 1;
    } else {
    }

    dbglog_raw_log_header_t header;
    header.common_header.core_id = LOG_CORE_SELF;
    header.common_header.version = DBGLOG_VERSION;
    header.common_header.payload_length = (uint16_t)length;
    header.module_id = UNKNOWN_MID;

    memcpy((char *)&buffer[0], (const char *)&header, sizeof(dbglog_raw_log_header_t));
    int ret = dbglog_gtp_write_buffer((in_irq() || (!cpu_get_int_enable())), GENERIC_TRANSMISSION_DATA_TYPE_RAW_LOG,
                                      (const uint8_t *)buffer,
                                      sizeof(dbglog_raw_log_header_t) + (uint32_t)length);
    if (buffer != dbglog_temp_buffer) {
        os_mem_free(buffer);
    }
    return (ret == 0) ? RET_OK : RET_FAIL;
}

uint8_t dbglog_stream_log_write(uint8_t module_id, DBGLOG_LEVEL level, uint16_t file_id,
                                uint16_t line_num, uint32_t param_num, ...)
    IRAM_TEXT(dbglog_stream_log_write);
uint8_t dbglog_stream_log_write(uint8_t module_id, DBGLOG_LEVEL level, uint16_t file_id,
                                uint16_t line_num, uint32_t param_num, ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, level, file_id, line_num, param_num, ap);
    va_end(ap);
    return ret;
}

uint8_t dbglog_stream_log_write_verbose(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
    IRAM_TEXT(dbglog_stream_log_write_verbose);
uint8_t dbglog_stream_log_write_verbose(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, DBGLOG_LEVEL_VERBOSE, file_line >> 16,
                                               file_line & 0xFFFF, param_num, ap);
    va_end(ap);
    return ret;
}

uint8_t dbglog_stream_log_write_debug(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
    IRAM_TEXT(dbglog_stream_log_write_debug);
uint8_t dbglog_stream_log_write_debug(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, DBGLOG_LEVEL_DEBUG, file_line >> 16,
                                               file_line & 0xFFFF, param_num, ap);
    va_end(ap);
    return ret;
}

uint8_t dbglog_stream_log_write_info(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
    IRAM_TEXT(dbglog_stream_log_write_info);
uint8_t dbglog_stream_log_write_info(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, DBGLOG_LEVEL_INFO, file_line >> 16,
                                               file_line & 0xFFFF, param_num, ap);
    va_end(ap);
    return ret;
}

uint8_t dbglog_stream_log_write_warning(uint8_t module_id, uint32_t file_line, uint32_t param_num,
                                        ...) IRAM_TEXT(dbglog_stream_log_write_warning);
uint8_t dbglog_stream_log_write_warning(uint8_t module_id, uint32_t file_line, uint32_t param_num,
                                        ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, DBGLOG_LEVEL_WARNING, file_line >> 16,
                                               file_line & 0xFFFF, param_num, ap);
    va_end(ap);
    return ret;
}

uint8_t dbglog_stream_log_write_error(uint8_t module_id, uint32_t file_line, uint32_t param_num,
                                      ...) IRAM_TEXT(dbglog_stream_log_write_error);
uint8_t dbglog_stream_log_write_error(uint8_t module_id, uint32_t file_line, uint32_t param_num,
                                      ...)
{
    va_list ap;
    va_start(ap, param_num);
    uint8_t ret = dbglog_stream_log_write_wrap(module_id, DBGLOG_LEVEL_ERROR, file_line >> 16,
                                               file_line & 0xFFFF, param_num, ap);
    va_end(ap);
    return ret;
}
