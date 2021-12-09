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
 ****************************************************************************
 */
#include "types.h"
#include "os_task.h"
#include "app_main.h"
#include "cfg_dsp.h"
#include "dtop_app.h"

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************
 */

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************
 */
void app_entry(void *arg);
/**
 ******************************************************************************
 * @brief tws_app_entry
 ******************************************************************************
 */
void app_entry(void *arg)
{
    dtop_app_main(arg);

    extern void rpc_bt_mgnt_dummy(void);
    rpc_bt_mgnt_dummy();

    app_main_entry();

    //prevent init of other processes
    cfg_dsp_entry();
}
