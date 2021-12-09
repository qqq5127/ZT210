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
#ifndef LIB_OTA_H
#define LIB_OTA_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup OTA
 * @{
 * This section introduces the LIB OTA module's enum, structure, functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_SIZE_UNKNOWN 0xffffffff /*!< Used for ota_begin() if new image size is unknown */

#define IMAGE_DESC_MAGIC_WORD 0xABCD6543 /*!< The magic word for the image_desc_t structure. */

#define IMAGE_CORE0_FLAG     BIT(0)
#define IMAGE_CORE0_TBL      BIT(1)
#define IMAGE_CORE1_FLAG     BIT(2)
#define IMAGE_CORE1_TBL      BIT(3)
#define IMAGE_DSP_FW         BIT(4)
#define IMAGE_TONE           BIT(5)
#define IMAGE_SBL            BIT(15)
#define DBGLOG_LIB_OTA_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_OTA_MID, fmt, ##arg)
#define DBGLOG_LIB_OTA_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(IOT_OTA_MID, fmt, ##arg)

typedef RET_TYPE (*ota_anc_data_process_cb)(uint8_t* ota_anc_data);

/** @defgroup lib_ota_struct Struct
 * @{
 */
/**
 * @brief image header description , describe information of compressed image block.
 */
typedef struct image_desc_t
{
    uint32_t image_size;
    uint32_t image_compress_size;
    uint32_t image_start_addr;
    uint32_t image_end_addr;
}image_desc;

/**
 * @brief Description about image.
 */
typedef struct {
    uint32_t magic_word;              /*!< Magic word IMAGE_DESC_MAGIC_WORD */
    uint32_t fw_version;              /*!< Firmware version */
    uint32_t image_pack_types;        /*!< Firmware package type */
    uint32_t time_stamp;              /*!< package time stamp*/
    image_desc image_item_desc[];     /*!< description of Compressed OTA package*/
} pack_desc_t;

/** pack_desc_t should be 16 bytes */
static_assert(sizeof(pack_desc_t) == 16, ota_h);
/**
 * @}
 */

/**
 * @brief Opaque handle for an application OTA update
 *
 * ota_begin() returns a handle which is then used for subsequent
 * calls to ota_write() and ota_end().
 */
typedef uint32_t ota_handle_t;

/**
 * @brief   Commence an OTA update writing to the specified zone.

 * The ota zone is erased to the specified compressed ota package size.
 *
 * If package size is not yet known, pass 0 or OTA_SIZE_UNKNOWN which will
 * cause the entire zone to be erased.
 *
 * On success, this function allocates memory that remains in use
 * until ota_end() is called with the returned handle.
 *
 * @param image_size Size of new OTA compressed package image. Ota zone will be erased in order to receive this size of image. If 0 or OTA_SIZE_UNKNOWN, the entire zone is erased.
 * @param out_handle On success, returns a handle(>0) which should be used for subsequent ota_write() and ota_end() calls.

 * @return
 *    - RET_OK: OTA operation commenced successfully.
 *    - RET_*: zone or out_handle arguments were NULL,or other internal error happened,like erase flash failed
 */
RET_TYPE ota_begin(size_t image_size, ota_handle_t* out_handle);

/**
 * @brief   Write OTA compressed package to OTA zone
 *
 * This function can be called multiple times as
 * data is received during the OTA operation. Data is written
 * sequentially to flash.
 *
 * @param handle  Handle obtained from ota_begin
 * @param data    Data buffer to write
 * @param size    Size of data buffer in bytes.
 *
 * @return
 *    - RET_OK: Data was written to flash successfully.
 *    - RET_*: ota not begin firstly or handle arguments were NULL,or other internal error happened,like write flash failed
 */
RET_TYPE ota_write(ota_handle_t handle, const void* data, size_t size);

/**
 * @brief   Write OTA compressed package to OTA zone
 *
 * This function can be called multiple times as
 * data is received during the OTA operation. Data is written
 * sequentially to flash.
 *
 * @param handle  Handle obtained from ota_begin
 * @param data    Data buffer to write
 * @param size    Size of data buffer in bytes.
 * @param offset  Flash write offset of ota partition.
 *
 * @return
 *    - RET_OK: Data was written to flash successfully.
 *    - RET_*: ota not begin firstly or handle arguments were NULL,or other internal error happened,like write flash failed
 */
RET_TYPE ota_write_data(ota_handle_t handle, const void* data, size_t size, uint32_t offset);

/**
 * @brief   Read OTA compressed package from OTA zone
 *
 * @param data    Data buffer from read
 * @param size    Size of data buffer in bytes.
 * @param offset  Flash read offset of ota partition.
 *
 * @return
 *    - RET_OK: Data was read to flash successfully.
 *    - RET_*: ota not begin firstly or handle arguments were NULL,or other internal error happened,like read flash failed
 */
RET_TYPE ota_read_data(void *data, size_t size, uint32_t offset);

/**
 * @brief Finish OTA update and validate newly written app image.
 *
 * @param handle  Handle obtained from ota_begin().
 *
 * @note After calling ota_end(), the handle is no longer valid and any memory associated with it is freed.
 *
 * @return
 *    - RET_OK: Newly written OTA app image is valid.
 *    - RET_*: OTA handle was not found.
 */
RET_TYPE ota_end(ota_handle_t handle);

/**
 * @brief Commit flag which enable OTA image to flash.
 *
 * @param reboot  True will do soft reboot when commit flag finished, otherwise only commit flag to flash.
 *
 * @note Pleace call this API After calling ota_end() finished correctly
 *
 * @return
 *    - RET_OK: Enable ota flag OK.
 *    - RET_*:  Enable ota flag Failed.
 */
RET_TYPE ota_commit(bool_t reboot);

/**
 * @brief Get ota anc data buffer in ota zone
 *
 * @param cb  Callback for anc data process
 *
 * @return
 *    - RET_OK: register anc data process callback OK
 *    - RET_*:  register anc data process callback Failed.
 */
RET_TYPE ota_register_anc_data_process_cb(ota_anc_data_process_cb cb);

/**
 * @brief Process original anc data and ota anc data, then rewrite to anc oem zone
 *
 * @note Pleace call this API when boot reason is BOOT_REASON_SOFT_REASON_OTA
 *
 * @return
 *    - RET_OK: process ota data OK.
 *    - RET_*:  process ota data Failed.
 */
RET_TYPE ota_anc_data_process(void);

/**
 * @brief Recover anc data , rewrite to anc oem zone
 *
 * @note Pleace call this API when ota_anc_data_process failed or oem_data_anc_load get anc data invalid
 *
 * @return
 *    - RET_OK: Recover anc data OK.
 *    - RET_*:  Recover anc data Failed.
 */
RET_TYPE ota_anc_data_recover(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OTA
 */

/**
 * @}
 * addtogroup LIB
 */

#endif /* LIB_OTA_H */
