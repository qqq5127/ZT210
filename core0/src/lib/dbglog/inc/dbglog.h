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

#ifndef LIB_DBGLOG_H
#define LIB_DBGLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DBGLOG_VERSION 2

typedef enum {
    DBGLOG_LEVEL_ALL = 0,
    DBGLOG_LEVEL_VERBOSE = 1,
    DBGLOG_LEVEL_DEBUG = 2,
    DBGLOG_LEVEL_INFO = 3,
    DBGLOG_LEVEL_WARNING = 4,
    DBGLOG_LEVEL_ERROR = 5,
    DBGLOG_LEVEL_CRITICAL = 6,
    DBGLOG_LEVEL_NONE = 7,
    DBGLOG_LEVEL_MAX = 8,
} DBGLOG_LEVEL;

/* coredump io define,default is uart dump, and uart dump is always ON now*/
#define    DBGLOG_DUMP_UART   0x00
#define    DBGLOG_DUMP_FLASH  0x01

/* coredump mode define,default is full dump, can enable minidump mode only as specific scenario*/
#define    DBGLOG_DUMP_FULL  0x00
#define    DBGLOG_DUMP_MINI  0x01

// Regular DBGLOG_[MODULE_NAME]_LOG
#define DBGLOG_LOG(module, lvl, fmt, ...) dbglog_raw_log_write(module, fmt, ##__VA_ARGS__)
#define DBGLOG_INFO(module, fmt, ...)     dbglog_raw_log_write(module, fmt, ##__VA_ARGS__)
#define DBGLOG_WARNING(module, fmt, ...)  dbglog_raw_log_write(module, fmt, ##__VA_ARGS__)
#define DBGLOG_ERROR(module, fmt, ...)    dbglog_raw_log_write(module, fmt, ##__VA_ARGS__)
#define DBGLOG_LOG_RAW(module, fmt, ...)  dbglog_raw_log_write(module, fmt, ##__VA_ARGS__)

/*lint -emacro(835, __FILE_LINE__) __FILE_ID maybe 0 */
#define __FILE_LINE__ ((uint32_t)(__FILE_ID << 16) | (__LINE__))

#define get_param_id(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, \
                     ...)                                                                        \
    a17
#define get_param_num(...) \
    get_param_id(a0, ##__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define DBGLOG_STREAM_LOG(module, lvl, fmt, ...)                                          \
    dbglog_stream_log_write(module, lvl, __FILE_ID, __LINE__, get_param_num(__VA_ARGS__), \
                            ##__VA_ARGS__)

#define DBGLOG_STREAM_VERBOSE(module, fmt, ...) \
    dbglog_stream_log_write_verbose(module, __FILE_LINE__, get_param_num(__VA_ARGS__), ##__VA_ARGS__)

#define DBGLOG_STREAM_DEBUG(module, fmt, ...) \
    dbglog_stream_log_write_debug(module, __FILE_LINE__, get_param_num(__VA_ARGS__), ##__VA_ARGS__)

#define DBGLOG_STREAM_INFO(module, fmt, ...) \
    dbglog_stream_log_write_info(module, __FILE_LINE__, get_param_num(__VA_ARGS__), ##__VA_ARGS__)

#define DBGLOG_STREAM_WARNING(module, fmt, ...)                                        \
    dbglog_stream_log_write_warning(module, __FILE_LINE__, get_param_num(__VA_ARGS__), \
                                    ##__VA_ARGS__)

#define DBGLOG_STREAM_ERROR(module, fmt, ...) \
    dbglog_stream_log_write_error(module, __FILE_LINE__, get_param_num(__VA_ARGS__), ##__VA_ARGS__)

typedef struct dbglog_common_header {
    uint32_t timestamp;
    uint16_t core_id : 2;
    uint16_t sequence_num : 10;
    uint16_t version : 4;
    uint16_t payload_length;
} __attribute((packed)) dbglog_common_header_t;

typedef struct dbglog_stream_log_header {
    dbglog_common_header_t common_header;
    uint8_t level : 3;
    uint8_t reserved : 5;
    uint8_t module_id;
    uint16_t file_id;
    uint16_t line_num;
} __attribute((packed)) dbglog_stream_log_header_t;

typedef struct dbglog_raw_log_header {
    dbglog_common_header_t common_header;
    uint8_t level : 3;
    uint8_t reserved : 5;
    uint8_t module_id;
} __attribute((packed)) dbglog_raw_log_header_t;

typedef struct dbglog_crash_log_header {
    dbglog_common_header_t common_header;
    uint8_t level : 3;
    uint8_t reserved : 5;
    uint8_t module_id;
} __attribute((packed)) dbglog_crash_log_header_t;

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param level is struct the stream log of header's level.
 * @param file_id is struct the stream log of header's file id.
 * @param line_num is struct the stream log of header's line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write(uint8_t module_id, DBGLOG_LEVEL level, uint16_t file_id,
                                uint16_t line_num, uint32_t param_num, ...);

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param file_line high 16-bit is file id, low 16bit is line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write_verbose(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...);

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param file_line high 16-bit is file id, low 16bit is line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write_debug(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...);

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param file_line high 16-bit is file id, low 16bit is line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write_info(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...);

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param file_line high 16-bit is file id, low 16bit is line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write_warning(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...);

/**
 * @brief This function is used to write stream log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param file_line high 16-bit is file id, low 16bit is line number.
 * @param param_num is the number of parameter which generated when pre-process.
 * @return uint8_t RET_OK or RET_NOMEM.
 */
uint8_t dbglog_stream_log_write_error(uint8_t module_id, uint32_t file_line, uint32_t param_num, ...);

/**
 * @brief This function is used to write raw log for dbglog.
 *
 * @param module_id is struct the stream log of header's module id.
 * @param format is the format of dbglog.
 * @param ...
 * @return uint8_t RET_OK or RET_FAIL or RET_NOMEM.
 */
uint8_t dbglog_raw_log_write(uint8_t module_id, const char *format, ...);

/**
 * @brief This function is used to write crash buffer info for dbglog.
 *
 * @param buffer is the pointer of write buffer.
 * @param length is the length of write buffer.
 * @return uint8_t RET_OK.
 */
uint8_t dbglog_crash_log_write(const uint8_t *buffer, uint8_t length);

/**
 * @brief This function is used to write crash buffer info for dbglog.
 *
 * @param buffer is the pointer of write buffer.
 * @param length is the length of write buffer.
 * @return uint8_t RET_OK.
 */
uint8_t dbglog_buffer_write_crash(const uint8_t *buffer, uint8_t length);

/**
 * @brief This function is used to init dbglog module.
 *
 * @return void.
 */
void dbglog_init(void);

/**
 * @brief This function is used to config dblglog module according to kv.
 *  this API should be called at core 0.
 *  and need to be called after storage init done and dsp and core1 is both up
 *
 * @return void.
 */
void dbglog_config_load(void);

/**
 * @brief Set output log level.
 *
 * @param module Module to be set
 * @param level The level to be set to
 * @return uint8_t RET_OK for success, otherwise fail.
 */
uint8_t dbglog_set_log_level(uint8_t module, uint8_t level);

/**
 * @brief Get dbglog coredump mode
 *
 * @return uint8_t coredump mode in dbglog
 */
uint8_t dbglog_get_dump_mode(void);

/********** NOTE: these API in deprecated , add them only for core1 patch compile***************/
uint8_t dbglog_wrap(const char *file_name, uint32_t line, DBGLOG_LEVEL level, const char *format,
                    ...);
uint8_t dbglog(const char *file_name, uint32_t line, DBGLOG_LEVEL level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* LIB_DBGLOG_H */
