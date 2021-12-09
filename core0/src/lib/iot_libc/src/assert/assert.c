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
#include "types.h"

#ifdef BUILD_PATCH
extern assert_handler _assert;
extern assert_dump_handler _assert_dump;
#else
assert_handler _assert = NULL;
assert_dump_handler _assert_dump = NULL;
#endif

void __assert(const char *file, int line) IRAM_TEXT(__assert);
void __assert(const char *file, int line)
{
    if (_assert) {
        _assert(file, (unsigned long)line);
    }
}

void assert_failed_dump(const char *pucFile, unsigned long ulLine, const unsigned long *dump_p,
                        unsigned long dump_size) IRAM_TEXT(assert_failed_dump);
void assert_failed_dump(const char *pucFile, unsigned long ulLine, const unsigned long *dump_p,
                        unsigned long dump_size)
{
    if (_assert_dump) {
        _assert_dump(pucFile, ulLine, dump_p, dump_size);
    }
}

void assert_handler_register(assert_handler handler)
{
    _assert = handler;
}

void assert_dump_handler_register(assert_dump_handler handler)
{
    _assert_dump = handler;
}
