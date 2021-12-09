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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>   // standard integer definitions
#include "string.h"
#include "os_mem.h"
#include "lib_dbglog.h"
#include "iot_share_task.h"
#include "iot_timer.h"
#include "dbglog_cache.h"   // SW profiling definition

/*
 * ENVIRONMENT VARIABLE DEFINITIONS
 ****************************************************************************************
 */
log_buf_t *log_buffer = NULL;
uint8_t log_read_idx = 0;
uint8_t log_write_idx = 0;
uint16_t log_miss_count = 0;
uint32_t dbglog_cache_enable_bitmap_low = 0;
uint32_t dbglog_cache_enable_bitmap_high = 0;
uint8_t g_num_malloc = 0;
uint32_t dbglog_cache_clk_us_prev = 0;

#define IOT_CLK_DIFF(clock_a, clock_b) \
    (((clock_b) > (clock_a)) ? ((clock_b) - (clock_a)) : ((0xFFFFFFFF - (clock_a)) + (clock_b) + 1))

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void dbglog_cache_init(uint8_t num_malloc)
{
    uint64_t trace_bitmap = 0;
    uint32_t ret;

    g_num_malloc = num_malloc;

    if (log_buffer != NULL) {
        DBGLOG_LIB_LOGGER_INFO("dbglog_cache_init:log_buffer isn't NULL error\n");
        return;
    }

    log_buffer = (log_buf_t *)os_mem_malloc(IOT_LOGGER_MID, ((sizeof(log_buf_t)) * g_num_malloc));

    if (log_buffer == NULL) {
        DBGLOG_LIB_LOGGER_INFO("dbglog_cache_init malloc fail error ...\n");
        return;
    }
    memset((log_buf_t *)log_buffer, 0, ((sizeof(log_buf_t)) * g_num_malloc));

    //save log print in share task event
    ret = iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_DBGLOG_CACHE_EVENT,
                                        dbglog_cache_print, NULL);
    if (ret != RET_OK) {
        return;
    }

    trace_bitmap |= (uint64_t)1 << DBG_SWDIAG_WQ_PHY;
    trace_bitmap |= (uint64_t)1 << DBG_SWDIAG_WQ_P3;
    trace_bitmap |= (uint64_t)1 << DBG_SWDIAG_WQ_COMMON_LOG;
    //trace_bitmap |= (uint64_t)1<<DBG_SWDIAG_INQ;

    dbglog_cache_set_band_bitmap(trace_bitmap);
}

void dbglog_cache_set_band_bitmap(uint64_t bitmap)
{
    dbglog_cache_enable_bitmap_low = (uint32_t)bitmap & 0xffffffff;
    dbglog_cache_enable_bitmap_high = (uint32_t)((bitmap >> 32) & 0xffffffff);
}

void dbglog_cache_save(uint16_t bank, uint16_t offset, const char *fmt, uint32_t value0,
                       uint32_t value1, uint32_t value2) IRAM_TEXT(dbglog_cache_save);
void dbglog_cache_save(uint16_t bank, uint16_t offset, const char *fmt, uint32_t value0,
                       uint32_t value1, uint32_t value2)
{
    if (log_buffer == NULL) {
        DBGLOG_LIB_LOGGER_INFO("dbglog_cache_save malloc fail...\n");
        return;
    }

    if (bank < 32 && ((dbglog_cache_enable_bitmap_low & (1U << bank)) == 0))
        return;
    if (bank >= 32 && ((dbglog_cache_enable_bitmap_high & (1U << (bank - 32))) == 0))
        return;
    if (((log_write_idx + 1) & (g_num_malloc - 1)) == log_read_idx) {
        log_miss_count++;
        log_read_idx =
            (log_read_idx + 1) & (g_num_malloc - 1);   //write idx catch the read idx, buffer full
    }

    log_buffer[log_write_idx].bank = bank;
    log_buffer[log_write_idx].offset = offset;
    log_buffer[log_write_idx].clock = iot_timer_get_time();
    log_buffer[log_write_idx].fmt = fmt;
    log_buffer[log_write_idx].value0 = value0;
    log_buffer[log_write_idx].value1 = value1;
    log_buffer[log_write_idx].value2 = value2;

    log_write_idx = (log_write_idx + 1) & (g_num_malloc - 1);
}

/**
 * @brief This function is used to trigger print dbglog cache.
 *        only called from task.
 */
void dbglog_cache_print_trigger(void)
{
    uint32_t clk_diff;
    uint32_t clk_us_curr = iot_timer_get_time();

    clk_diff = IOT_CLK_DIFF(dbglog_cache_clk_us_prev, clk_us_curr);

    if (((dbglog_cache_get_used_buf_num() > (LOG_BUFFER_SIZE >> 1))
         && (clk_diff > (DBGLOG_CACHE_INTERVAL_US >> 1)))
        || (clk_diff > DBGLOG_CACHE_INTERVAL_US)) {
        /* post event to share task to print dbglog cache */
        iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_LP,
            IOT_SHARE_EVENT_DBGLOG_CACHE_EVENT);
        dbglog_cache_clk_us_prev = clk_us_curr;
    }

    return;
}

uint8_t dbglog_cache_read(log_buf_t *log_bank, uint16_t *log_miss_cnt)
{
    if (log_buffer == NULL) {
        DBGLOG_LIB_LOGGER_INFO("dbglog_cache_read malloc fail...\n");
        return 0;
    }

    if (log_read_idx == log_write_idx) {
        return 0;   //buffer empty
    }

    log_bank->bank = log_buffer[log_read_idx].bank;
    log_bank->offset = log_buffer[log_read_idx].offset;
    log_bank->clock = log_buffer[log_read_idx].clock;
    log_bank->fmt = log_buffer[log_read_idx].fmt;
    log_bank->value0 = log_buffer[log_read_idx].value0;
    log_bank->value1 = log_buffer[log_read_idx].value1;
    log_bank->value2 = log_buffer[log_read_idx].value2;

    log_read_idx = (log_read_idx + 1) & (g_num_malloc - 1);
    *log_miss_cnt = log_miss_count;
    log_miss_count = 0;
    return 1;
}


uint8_t dbglog_cache_get_used_buf_num(void)
{
    uint8_t num;

    if (log_write_idx > log_read_idx) {
        num = log_write_idx - log_read_idx;
    } else if (log_write_idx < log_read_idx) {
        num = (LOG_BUFFER_SIZE - log_read_idx) + log_write_idx;
    } else {
        num = 0;
    }

    return num;
}

void dbglog_cache_print(void *arg)
{
    uint16_t log_miss_cnt;
    log_buf_t log_bank;

    UNUSED(arg);
    //DBGLOG_LIB_LOGGER_INFO("%d:used num: %d\n", iot_timer_get_time(), dbglog_cache_get_used_buf_num());

    while (dbglog_cache_read(&log_bank, &log_miss_cnt)) {
        if (log_bank.bank == DBG_SWDIAG_WQ_COMMON_LOG) {
            DBGLOG_LIB_LOGGER_RAW(log_bank.fmt, log_bank.value0, log_bank.value1, log_bank.value2);
        } else {
            DBGLOG_LIB_LOGGER_INFO("%x:%d, %d, %d, %d, %d\n", log_bank.clock, log_bank.bank,
                            log_bank.offset, log_bank.value0, log_bank.value1, log_bank.value2);
        }

        if (log_miss_cnt != 0) {
            DBGLOG_LIB_LOGGER_INFO("%d FW logs are overflowed\n", log_miss_cnt);
        }
    }
}

void dbglog_cache_assert_dump(void (*func)(char *fmt, ...))
{
    log_buf_t log_bank;

    func("----- dbglog_cache_assert_dump -----\n");
    //Dump all log in cache.Because logs may send to share memory , but not output from serial port.
    for (uint32_t i = 0; i < LOG_BUFFER_SIZE; i++) {
        log_bank = log_buffer[(log_write_idx + i) & (g_num_malloc - 1)];
        func("[%2d]:", i);
        if (log_bank.bank == DBG_SWDIAG_WQ_COMMON_LOG) {
            func((char *)log_bank.fmt, log_bank.value0, log_bank.value1, log_bank.value2);
        } else {
            func("%x:%d, %d, %d, %d, %d\n", log_bank.clock, log_bank.bank, log_bank.offset,
                 log_bank.value0, log_bank.value1, log_bank.value2);
        }
    }
}
