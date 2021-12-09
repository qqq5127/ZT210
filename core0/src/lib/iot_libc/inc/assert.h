/*
 * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated into another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 */

#ifndef LIB_IOT_LIBC_ASSERT_H
#define LIB_IOT_LIBC_ASSERT_H

#ifndef CONFIG_ASSERT_ENABLE
#if defined(RELEASE)
#define CONFIG_ASSERT_ENABLE 1
#elif defined(DEVELOPMENT)
#define CONFIG_ASSERT_ENABLE 1
#else
#define CONFIG_ASSERT_ENABLE 1
#endif
#endif

//lint -emacro(506, assert)
#if CONFIG_ASSERT_ENABLE
#define assert(x) ((x) ? ((void)0) : __assert(__FILE_NAME__, __LINE__))
#define assert_file(x, file, line) ((x) ? ((void)0) : __assert(file, line))
#define assert_failed(file, line)  __assert(file, line)
#else
#define assert(x)                  ((void)0)
#define assert_file(x, file, line) ((void)0)
#define assert_failed(file, line)  ((void)0)
#endif

#define ASSERT_FAILED_DUMP(dump_p, dump_size) \
    assert_failed_dump(__FILE_NAME__, __LINE__, dump_p, dump_size)

typedef void (*assert_handler)(const char *file_name, unsigned long line);
typedef void (*assert_dump_handler)(const char *file_name, unsigned long line,
                                    const unsigned long *dump_p,
                                    unsigned long dump_size);

/** A compile time assertion check.
 *
 *  Validate at compile time that the predicate is true without
 *  generating code. This can be used at any point in a source file
 *  where typedef is legal.
 *
 *  On success, compilation proceeds normally.
 *
 *  On failure, attempts to typedef an array type of negative size. The
 *  offending line will look like
 *      typedef assertion_failed_file_h_42[-1]
 *  where file is the content of the second parameter which should
 *  typically be related in some obvious way to the containing file
 *  name, 42 is the line number in the file on which the assertion
 *  appears, and -1 is the result of a calculation based on the
 *  predicate failing.
 *
 *  \param predicate The predicate to test. It must evaluate to
 *  something that can be coerced to a normal C boolean.
 *
 *  \param file A sequence of legal identifier characters that should
 *  uniquely identify the source file in which this condition appears.
 */
#define static_assert(predicate, file) \
    static_assert_line(predicate, __LINE__, file)

#define str_paste(a, b) a##b
#define static_assert_line(predicate, line, file)      \
    typedef char str_paste(assertion_failed_##file##_, \
                           line)[2 * !!(predicate)-1] __attribute__((unused))

void __assert(const char *file, int line);
void assert_failed_dump(const char *pucFile, unsigned long ulLine,
                        const unsigned long *dump_p, unsigned long dump_size);
void assert_handler_register(assert_handler handler);
void assert_dump_handler_register(assert_dump_handler handler);

#endif /* !LIB_IOT_LIBC_ASSERT_H */
