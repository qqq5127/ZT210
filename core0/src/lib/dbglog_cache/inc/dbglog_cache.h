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

#ifndef DBGLOG_CACHE_H_
#define DBGLOG_CACHE_H_

#define LOG_BUFFER_SIZE 32
#define DBGLOG_CACHE_INTERVAL_US 100000 // 100ms
#define DBGLOG_LIB_LOGGER_RAW(fmt, arg...)      DBGLOG_LOG(IOT_LOGGER_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_LIB_LOGGER_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_LOGGER_MID, fmt, ##arg)

/*
 * SW LOG MAP
 ****************************************************************************************
 */
enum wq_swdiag
{
    // set to 49 in order to be consistent with the patch version.
    DBG_SWDIAG_WQ_SWDIAG = 49,
    DBG_SWDIAG_WQ_PHY, // noy used
    DBG_SWDIAG_WQ_ACL,
    DBG_SWDIAG_WQ_P3,
    DBG_SWDIAG_WQ_COMMON_LOG,
};

/*
 * TYPES DEFINITIONS
 ****************************************************************************************
 */
typedef struct _log_buf_t
{
    uint16_t bank;
    uint16_t offset;
    uint32_t clock;
    const char* fmt;
    uint32_t value0;
    uint32_t value1;
    uint32_t value2;
} log_buf_t;

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 * @brief This function is used to init dbglog cache.
 *
 * @param num_malloc is malloc size.
 */
void dbglog_cache_init(uint8_t num_malloc);

/**
 * @brief This function is used to enable bitmap for dbglog cache.
 *
 * @param bitmap which is to enable.
 */
void dbglog_cache_set_band_bitmap(uint64_t bitmap);

/**
 * @brief This function is used to save dbglog cache.
 *
 * @param bank is log buffer's bank.
 * @param offset is struct log buffer's offset.
 * @param fmt is struct log buffer's fmt.
 * @param value0 is struct log buffer's value0.
 * @param value1 is struct log buffer's value1.
 * @param value2 is struct log buffer's value2.
 */
void dbglog_cache_save(uint16_t bank, uint16_t offset, const char* fmt,
                 uint32_t value0,uint32_t value1, uint32_t value2);

/**
 * @brief This function is used to trigger print dbglog cache.
 *        only called from task.
 */
void dbglog_cache_print_trigger(void);

/**
 * @brief This function is used to read info from dbglog cache.
 *
 * @param log_bank is the pointer of log.
 * @param log_miss_cnt is miss count of log.
 * @return uint8_t
 */
uint8_t dbglog_cache_read(log_buf_t *log_bank, uint16_t *log_miss_cnt);

/**
 * @brief This function is used to get the number of used buffer for dbglog.
 *
 * @return uint8_t is the number of dbglog.
 */
uint8_t dbglog_cache_get_used_buf_num(void);

/**
 * @brief This function is used to print loginfo for dbglog cache.
 *
 * @param arg UNUSED.
 */
void dbglog_cache_print(void *arg);

/**
 * @brief This function is used to print all logs when assert.
 *
 * @param fun print function pointer.
 */
void dbglog_cache_assert_dump(void (*fun)(char *fmt, ...));

#define dbg_save_log(bank, offset, value0, value1, value2)          \
            dbglog_cache_save(bank, offset, "", value0, value1, value2)

#define dbg_save_fmt_log(fmt, value0, value1, value2)               \
            dbglog_cache_save(DBG_SWDIAG_WQ_COMMON_LOG, 0, fmt, value0, value1, value2)

#endif // DBGLOG_CACHE_H_
