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

#ifndef LIB_IOT_DEBUG_H
#define LIB_IOT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*iot_debug_log_print_t)(const char *fmt, ...);
typedef void (*iot_debug_assert_dump_fun_t)(iot_debug_log_print_t cb);

/**
 * @brief This function is used to regisiter an assert dump function.
 *
 * @param fn is assert dump function pointer.
 * @return RET_TYPE is error nu.
 */
RET_TYPE iot_debug_reg_assert_dump_fun(iot_debug_assert_dump_fun_t fn);

/**
 * @brief This function is used to assert for debug.
 *
 * @param file_name is debug file name.
 * @param line is debug line.
 */
void iot_debug_assert(const char *file_name, unsigned long line);

/**
 * @brief This function is used to init debug.
 *
 */
void iot_debug_init(void);

/**
 * @brief This function is used to assert dump for debug.
 *
 * @param file_name is debug file name.
 * @param line is debug line.
 * @param dump_p is debug dump pointer of start address.
 * @param dump_size is debug dump size.
 */
void iot_debug_assert_dump(const char *file_name, unsigned long line,
        const unsigned long *dump_p, unsigned long dump_size);


#ifdef __cplusplus
}
#endif

#endif /* LIB_IOT_DEBUG_H */
