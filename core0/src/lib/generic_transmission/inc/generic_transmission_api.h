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

#ifndef _LIB_GENERIC_TRANSMISSION_API_H__
#define _LIB_GENERIC_TRANSMISSION_API_H__

#include "types.h"

/******************** Uart Dump Example START ********************
 *
 * Note: Important!!!!!
 * As this test task is to test throughput, so the task mode is always retry.
 * It means, if the test task on BT Core, it will always push data without schedule out.
 * Thus, if the consumer task and profile task has the highest priority, the other tasks
 * may not have oppotunities to be called. If you use uart dump both on BT core and DTOP
 * Core, the throughput relationshiop between the two cores depends on the tasks priority.
 *
 * In common use, the producer should not always push data with no pending, such as
 * semaphore take, queue receive, task delay and etc.
 *
 * Please take care of task priority configurations!!!
 * To make a common use has no problem, default task priority is set to the same (5).
 * If you want to improve the throughput on BT core uart dump, you can modify the consumer
 * task priority and profile task prority in this file and generic_transmission_profile.c.
 *
 *
 * #include "generic_transmission_api.h"
 *
 * #ifdef CONFIG_GENERIC_TRANSMISSION_TEST_ON_CORE0
 * #define CONFIG_GENERIC_TRANSMISSION_TEST_TASK_PRIO     5
 *
 * #define GENERIC_TRANSMISSION_FIXED_SEND_BUF_SIZE       1600
 *
 * #define GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT      (128)
 * #define GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT_MASK (GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT - 1)
 *
 * #define GENERIC_TRANSMISSION_TEST_USER_ID GENERIC_TRANSMISSION_USER_ID1
 * #endif
 *
 * #ifdef CONFIG_GENERIC_TRANSMISSION_TEST_DEBUG
 * #define GENERIC_TRANSMISSION_TEST_LOGD   DBGLOG_LIB_RAW
 * #define GENERIC_TRANSMISSION_TEST_LOGI   DBGLOG_LIB_INFO
 * #define GENERIC_TRANSMISSION_TEST_LOGE   DBGLOG_LIB_ERROR
 * #else
 * #define GENERIC_TRANSMISSION_TEST_LOGD(fmt, arg...)
 * #define GENERIC_TRANSMISSION_TEST_LOGI   DBGLOG_LIB_INFO
 * #define GENERIC_TRANSMISSION_TEST_LOGE   DBGLOG_LIB_ERROR
 * #endif
 *
 * #ifdef CONFIG_GENERIC_TRANSMISSION_TEST_ON_CORE0
 *
 * static void generic_transmission_producer_test_task(void *arg)
 * {
 *     int32_t count = 0;
 *     uint32_t total_size = 0;
 *     uint32_t t1 = 0;
 *     uint32_t t2 = 0;
 *
 *     UNUSED(arg);
 *
 *     os_delay(1000);
 *
 *     while (1) {
 *         int32_t remain_len = sizeof(s_generic_transmission_producer_test_buf);
 *         int32_t ret;
 *
 *         GENERIC_TRANSMISSION_TEST_LOGD("Producer Test Push, len %d, count %d\n", remain_len, count);
 *
 *         if ((count & GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT_MASK) == 0) {
 *             t1 = iot_rtc_get_global_time_ms();
 *         }
 *              // Attention!!!!
 *              // This test code is used for "throughput" testing, there's no schedule out operations,
 *              // such as semaphore take, queue wait, os_delay and etc.
 *              // In actual scenario, should not call the API in a forever loop without any delay.
 *              // Especially, If the retern value is a negative value, it means there's error, such
 *              // as -RET_NOMEM, please do delay, semaphore take or other method which can cause yield.
 *              //
 *         do {
 *             ret = generic_transmission_data_tx(GENERIC_TRANSMISSION_TX_MODE_LAZY,
 *                                                GENERIC_TRANSMISSION_DATA_TYPE_AUDIO_DUMP,
 *                                                GENERIC_TRANSMISSION_TID0,
 *                                                GENERIC_TRANSMISSION_IO_UART0,
 *                                                s_generic_transmission_producer_test_buf + (sizeof(s_generic_transmission_producer_test_buf) - remain_len),
 *                                                remain_len,
 *                                                true);// need ack
 *             if (ret >= 0) {
 *                 remain_len -= ret;
 *                 total_size += ret;
 *                 GENERIC_TRANSMISSION_TEST_LOGD("Producer Test Push over, sent %d, remain %d\n", ret, remain_len);
 *             } else {
 *                 GENERIC_TRANSMISSION_TEST_LOGD("Producer Test Push error %d, remain %d\n", ret, remain_len);
 *             }
 *
 *         } while (ret < 0 || remain_len > 0);
 *
 *         count++;
 *
 *         if ((count & GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT_MASK) == GENERIC_TRANSMISSION_TEST_CALC_RATE_COUNT_MASK) {
 *             t2 = iot_rtc_get_global_time_ms();
 *
 *             GENERIC_TRANSMISSION_TEST_LOGI("Producer Test Core[%d] Data Rate is %u Kbps, delta time %d\n", cpu_get_mhartid(), (total_size << 3) / (t2 - t1), t2 - t1);
 *             total_size = 0;
 *         }
 *     }
 * }
 *
 * static void generic_transmission_producer_test_data_init(void)
 * {
 *     uint32_t data_inc = 0xAB000000;
 *
 *     for (uint32_t i = 0; i < GENERIC_TRANSMISSION_FIXED_SEND_BUF_SIZE; i += 4) {
 *         *(uint32_t *)(s_generic_transmission_producer_test_buf + i) = data_inc;
 *         data_inc++;
 *     }
 * }
 *
 * static void generic_transmission_test_init(void)
 * {
 *     GENERIC_TRANSMISSION_TEST_LOGI("[Uart Dump Test] init on core %d\n", cpu_get_mhartid());
 *
 *     generic_transmission_producer_test_data_init();
 *
 *     s_generic_transmission_producer_test_task_hdl = os_create_task_ext(generic_transmission_producer_test_task,
 *                                                             NULL,
 *                                                             CONFIG_GENERIC_TRANSMISSION_TEST_TASK_PRIO,
 *                                                             256,
 *                                                             "generic_transmission_producer_test");
 * }
 *
 * static void generic_transmission_test_deinit(void)
 * {
 *     GENERIC_TRANSMISSION_TEST_LOGI("[Uart Dump Test] deinit on core %d\n", cpu_get_mhartid());
 *
 *     if (s_generic_transmission_producer_test_task_hdl) {
 *         os_delete_task(s_generic_transmission_producer_test_task_hdl);
 *         s_generic_transmission_producer_test_task_hdl = NULL;
 *     }
 * }
 *
 * generic_transmission_init();
 * generic_transmission_test_init();
 *       ......
 * generic_transmission_test_deinit();
 * generic_transmission_deinit();
 ********************* Uart Dump Example END ********************/

/*********************** Common Type Define Start ***********************/
#define  GTB_ST_DISABLE   0x00
#define  GTB_ST_ENABLE    0x01
#define  GTB_ST_IN_PANIC  0x02
#define  GTB_ST_PANIC_END 0x03

#define CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO 5
#define CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO_HIGH 7

/* IO method, such as UART0/UART1/BLE/I2C/SPI/etc. Now, only support UART0 */
typedef enum {
    GENERIC_TRANSMISSION_IO_UART0,
    /* please add other IO here, such as GENERIC_TRANSMISSION_IO_UART1, SPP and etc. */
    GENERIC_TRANSMISSION_IO_SPP,
    GENERIC_TRANSMISSION_IO_FLASH,
    GENERIC_TRANSMISSION_IO_NUM,
} generic_transmission_io_t;
/*********************** Common Type Define End ***********************/

/*********************** User API Define Start ***********************/
/* Please don't change this enum value, TID max numbers is 8,
 * but you can rename the name to make it easy remember
 * */
typedef enum {
    GENERIC_TRANSMISSION_TID0 = 0,
    GENERIC_TRANSMISSION_TID1,
    GENERIC_TRANSMISSION_TID2,
    GENERIC_TRANSMISSION_TID3,
    GENERIC_TRANSMISSION_TID4,
    GENERIC_TRANSMISSION_TID5,
    GENERIC_TRANSMISSION_TID6,
    GENERIC_TRANSMISSION_TID7,
    /* should be equal to or less than GENERIC_TRANSMISSION_PRF_TID_NUM in generic_transmission_profile.c */
    GENERIC_TRANSMISSION_TID_NUM = 8,
} generic_transmission_tid_t;

/* TID priority, more larger value, more higher priority */
typedef enum {
    GENERIC_TRANSMISSION_PRIO0 = 0,
    GENERIC_TRANSMISSION_PRIO1,
    GENERIC_TRANSMISSION_PRIO2,
    GENERIC_TRANSMISSION_PRIO3,
    GENERIC_TRANSMISSION_PRIO4,
    GENERIC_TRANSMISSION_PRIO5,
    GENERIC_TRANSMISSION_PRIO6,
    GENERIC_TRANSMISSION_PRIO7,
    GENERIC_TRANSMISSION_PRIO_NUM,
} generic_transmission_prio_t;

/* Corresponding to GTP_PKT_TYPE_SUB_XXX in generic_transmission_protocol.h */
typedef enum {
    GENERIC_TRANSMISSION_DATA_TYPE_DFT = 0,
    GENERIC_TRANSMISSION_DATA_TYPE_STREAM_LOG,
    GENERIC_TRANSMISSION_DATA_TYPE_RAW_LOG,
    GENERIC_TRANSMISSION_DATA_TYPE_CLI,
    GENERIC_TRANSMISSION_DATA_TYPE_AUDIO_DUMP,
    GENERIC_TRANSMISSION_DATA_TYPE_PANIC_LOG,
    GENERIC_TRANSMISSION_DATA_TYPE_HCI_LOG,
    GENERIC_TRANSMISSION_DATA_TYPE_NUM,
} generic_transmission_data_type_t;

/* TX mode */
typedef enum {
    GENERIC_TRANSMISSION_TX_MODE_LAZY,
    GENERIC_TRANSMISSION_TX_MODE_ASAP,
    GENERIC_TRANSMISSION_TX_MODE_NUM,
} generic_transmission_tx_mode_t;

/* RX Callback status. Now, only report status OK */
typedef enum {
    GENERIC_TRANSMISSION_DATA_RX_CB_ST_OK = 0,
} generic_transmission_data_rx_cb_st_t;

typedef void (*generic_transmission_data_rx_cb_t)(generic_transmission_tid_t tid,
                                                  generic_transmission_data_type_t type,
                                                  uint8_t *data, uint32_t data_len,
                                                  generic_transmission_data_rx_cb_st_t status);

typedef bool_t (*generic_transmission_repack_cb_t)(const uint8_t* buffer);

/**
 * @brief Tx data in non-critical mode.
 * @param mode: tx mode
 * @param type: data type
 * @param tid: transport id, can be used to set different priority
 * @param io: IO method, such as UART0/UART1/BLE/I2C and etc.
 * @param data: data pointer
 * @param data_len: data length
 * @param need_ack: whether this data transmission need ack by remote device or not.
 *                  Warning!!!: TX with the same TID must use unique need_ack value,
 *                              or it may cause data missing!!!
 * @return >= 0 - handled size,  < 0 - fail.
 */
int32_t generic_transmission_data_tx(generic_transmission_tx_mode_t mode,
                                 generic_transmission_data_type_t type,
                                 generic_transmission_tid_t tid,
                                 generic_transmission_io_t io,
                                 const uint8_t *data, uint32_t data_len,
                                 bool_t need_ack);

/**
 * @brief Tx data in critical mode, such as in ISR and in Interrupt Disabled Context
 * @param mode: tx mode
 * @param type: data type
 * @param tid: transport id, can be used to set different priority
 * @param io: IO method, such as UART0/UART1/BLE/I2C and etc.
 * @param data: data pointer
 * @param data_len: data length
 * @param need_ack: whether this data transmission need ack by remote device or not.
 *                  Warning!!!: TX with the same TID must use unique need_ack value,
 *                              or it may cause data missing!!!
 * @return >= 0 - handled size,  < 0 - fail.
 */
int32_t generic_transmission_data_tx_critical(generic_transmission_tx_mode_t mode,
                                          generic_transmission_data_type_t type,
                                          generic_transmission_tid_t tid,
                                          generic_transmission_io_t io,
                                          const uint8_t *data, uint32_t data_len,
                                          bool_t need_ack);

/**
 * @brief Set generic transmission in panic status
 * Used for indicate each share memory in panic status.
 * In panic status, the tx process is different form normal status.
 */
void generic_transmission_panic_start(void);

/**
 * @brief Set generic transmission in panic end status.
 * This status means panic is end.
 * For exmpale, if core1 is in panic, then core0 is in panic, too.
 * Core0 should call `generic_transmission_consumer_tx_process_panic`
 * to handler the data which is in TX Queue and in share memory in excpetion
 * context(ISR), `generic_transmission_consumer_tx_process_panic` will return,
 * When core1 call `generic_transmission_panic_end`. Then core0 can handle itself's
 * panic log.
 */
void generic_transmission_panic_end(void);

/**
 * @brief Get the core's generic transmission status.
 * return GTP status.
 */
uint8_t generic_transmission_get_status(uint32_t core_id);

/**
 * @brief Get the system_status in panic or not, get true if in panic status, otherwise get false.
 * return system gtp status in panic or not.
 */
bool_t generic_transmission_in_panic(void);

/**
 * @brief Tx data type in panic, force to not need ack.
 * @param mode: tx mode
 * @param type: data type
 * @param tid: transport id, can be used to set different priority
 * @param io: IO method, such as UART0/UART1/BLE/I2C and etc.
 * @param data: data pointer
 * @param data_len: data length
 * @return >= 0 - handled size,  < 0 - fail.
 */
int32_t generic_transmission_data_tx_panic(generic_transmission_tx_mode_t mode,
                                       generic_transmission_data_type_t type,
                                       generic_transmission_tid_t tid,
                                       generic_transmission_io_t io,
                                       const uint8_t *data, uint32_t data_len);

/**
 * @brief TX process other share memory instead of consumer task in core0 panic,
 */
void generic_transmission_consumer_tx_process_panic(void);

/**
 * @brief register RX callback function when recieve packet.
 *        only allow it be called on core0. The other cores call
 *        it will cause return failure value.
 * @param tid: data stream tid
 * @param cb: callback function
 * @return 0 - success, other value - fail.
 */
int32_t generic_transmission_register_rx_callback(generic_transmission_tid_t tid,
                                              generic_transmission_data_rx_cb_t cb);

/**
 * @brief set priority for specified TID. only allow it be called on core0.
 *        The other cores call it will cause return failure value.
 * @param tid: transport ID
 * @param prio: priority value
 * @return 0 - success, other value - fail.
 */
int32_t generic_transmission_set_tid_priority(generic_transmission_tid_t tid,
                                          generic_transmission_prio_t prio);

/**
 * @brief register repack data callback function when send packet.
 *        will modify send data before put pata to share memory
 * @param type: data type
 * @param cb: callback function
 * @return 0 - success, other value - fail.
 */
int32_t generic_transmission_register_repack_callback(generic_transmission_data_type_t type,
                                              generic_transmission_repack_cb_t cb);

/**
 * @brief set priority of generic transmission protocol
 * @param priority: priority of generic transmission consumer task and  generic transmission profile task
 * @return 0 - success, other value - fail.
 */
int32_t generic_transmission_set_priority(uint8_t priority);

/**
 * @brief restore priority of generic transmission protocol to default
 * @return 0 - success, other value - fail.
 */
int32_t generic_transmission_restore_priority(void);

/**
 * @brief Initialise generic transmission module.
 * @return None.
 */
void generic_transmission_init(void);

/**
 * @brief De-initialise generic transmission module.
 * @return None.
 */
void generic_transmission_deinit(void);
/*********************** User API Define End ***********************/

/*********************** IO API Define Start ***********************/
/* IO related functions type definition
 * The IO owener should implement all the functions, then register
 * it do generic tranmsission IO abstract layer.
 */
typedef int32_t (* generic_transmission_io_send_t)(const uint8_t *buf, uint32_t len);
typedef int32_t (* generic_transmission_io_send_panic_t)(const uint8_t *buf, uint32_t len);
typedef void (* generic_transmission_io_init_t)(void);
typedef void (* generic_transmission_io_deinit_t)(void);

typedef struct {
    /* send is necessary, each IO owner should implement it */
    generic_transmission_io_send_t send;
    /* send_panic is not necessary. It is used for log output in panic */
    generic_transmission_io_send_panic_t send_panic;
    /* If the IO initialise need be called by generic transmission, it's necessary.
     * Otherwise, if IO initialise is called by IO owner, init is not necessary.
     */
    generic_transmission_io_init_t init;
    /* If the IO de-initialise need be called by generic transmission, it's necessary.
     * Otherwise, if IO de-initialise is called by IO owner, init is not necessary.
     */
    generic_transmission_io_deinit_t deinit;
} generic_transmission_io_method_t;

/**
 * @brief Register IO methods to generic transmission IO abstract layer,
 * generic transmission IO abstract layer will call the function pointer
 * of these methods.
 * @param io: IO method, such as UART0/UART1/BLE/I2C and etc.
 * @param method: methods structure pointer.
 * @return = 0 - success,  other - fail.
 */
int32_t generic_transmission_io_method_register(generic_transmission_io_t io,
                                            generic_transmission_io_method_t *method);


/**
 * @brief IO owner should call this function when it want transfer its RX data
 * to generic transmssion IO abstract layer.
 * @param io: IO method, such as UART0/UART1/BLE/I2C and etc.
 * @param data: RX data pointer.
 * @param data_len: RX data length.
 * @param in_isr
 */
void generic_transmission_io_recv(generic_transmission_io_t io, const uint8_t *data, uint32_t data_len, bool_t in_isr);
/*********************** IO API Define End ***********************/


#endif /* _LIB_GENERIC_TRANSMISSION_API_H__ */
