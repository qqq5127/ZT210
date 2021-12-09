/****************************************************************************
 *
 * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated int32_to another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 *
 * ****************************************************************************/

#ifndef _LIB_RISCV_PMP_H
#define _LIB_RISCV_PMP_H

struct pmp;

/*!
 * @brief Set of available PMP addressing modes
 */
enum pmp_address_mode {
    /*! @brief Disable the PMP region */
    PMP_OFF = 0,
    /*! @brief Use Top-of-Range mode */
    PMP_TOR = 1,
    /*! @brief Use naturally-aligned 4-byte region mode */
    PMP_NA4 = 2,
    /*! @brief Use naturally-aligned power-of-two mode */
    PMP_NAPOT = 3
};

enum pmp_locked {
    PMP_UNLOCKED = 0,
    PMP_LOCKED = 1
};

/*!
 * @brief Configuration for a PMP region
 */
struct pmp_config {
    /*! @brief Sets whether reads to the PMP region succeed */
    uint32_t R : 1;
    /*! @brief Sets whether writes to the PMP region succeed */
    uint32_t W : 1;
    /*! @brief Sets whether the PMP region is executable */
    uint32_t X : 1;

    /*! @brief Sets the addressing mode of the PMP region */
    enum pmp_address_mode A : 2;

    uint32_t _pad : 2;

    /*! @brief Sets whether the PMP region is locked */
    enum pmp_locked L : 1;
};

/*!
 * @brief A handle for the PMP device
 */
struct pmp {
    /* The minimum granularity of the PMP region. Set by pmp_init */
    uintptr_t _granularity[3];
};

/*!
 * @brief Get the PMP device handle
 */
struct pmp *pmp_get_device(void);

/*!
 * @brief Get the number of pmp regions for the hartid
 */
uint32_t pmp_num_regions(int32_t hartid);

/*!
 * @brief Initialize the PMP
 * @param pmp The PMP device handle to be initialized
 *
 * The PMP initialization routine is optional and may be called as many times
 * as is desired. The effect of the initialization routine is to attempt to set
 * all regions to unlocked and disabled, as well as to clear the X, W, and R
 * bits. Only the pmp configuration of the hart which executes the routine will
 * be affected.
 *
 * If any regions are fused to preset values by the implementation or locked,
 * those PMP regions will silently remain uninitialized.
 */
void pmp_init(struct pmp *pmp);

/*!
 * @brief Configure a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to configure
 * @param config The desired configuration of the PMP region
 * @param address The desired address of the PMP region
 * @return 0 upon success
 */
int32_t pmp_set_region(const struct pmp *pmp, uint32_t region,
                         struct pmp_config config, size_t address);

/*!
 * @brief Get the configuration for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @param config Variable to store the PMP region configuration
 * @param address Variable to store the PMP region address
 * @return 0 if the region is read successfully
 */
int32_t pmp_get_region(const struct pmp *pmp, uint32_t region,
                         struct pmp_config *config, size_t *address);

/*!
 * @brief Lock a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to lock
 * @return 0 if the region is successfully locked
 */
int32_t pmp_lock(const struct pmp *pmp, uint32_t region);

/*!
 * @brief Set the address for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to set
 * @param address The desired address of the PMP region
 * @return 0 if the address is successfully set
 */
int32_t pmp_set_address(const struct pmp *pmp, uint32_t region,
                          size_t address);

/*!
 * @brief Get the address of a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @return The address of the PMP region, or 0 if the region could not be read
 */
size_t pmp_get_address(const struct pmp *pmp, uint32_t region);

/*!
 * @brief Set the addressing mode of a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to set
 * @param mode The PMP addressing mode to set
 * @return 0 if the addressing mode is successfully set
 */
int32_t pmp_set_address_mode(const struct pmp *pmp, uint32_t region,
                               enum pmp_address_mode mode);

/*!
 * @brief Get the addressing mode of a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @return The address mode of the PMP region
 */
enum pmp_address_mode pmp_get_address_mode(const struct pmp *pmp,
                                                       uint32_t region);

/*!
 * @brief Set the executable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to set
 * @param X The desired value of the executable bit
 * @return 0 if the executable bit is successfully set
 */
int32_t pmp_set_executable(const struct pmp *pmp, uint32_t region, int32_t X);

/*!
 * @brief Get the executable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @return the value of the executable bit
 */
int32_t pmp_get_executable(const struct pmp *pmp, uint32_t region);

/*!
 * @brief Set the writable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to set
 * @param W The desired value of the writable bit
 * @return 0 if the writable bit is successfully set
 */
int32_t pmp_set_writeable(const struct pmp *pmp, uint32_t region, int32_t W);

/*!
 * @brief Get the writable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @return the value of the writable bit
 */
int32_t pmp_get_writeable(const struct pmp *pmp, uint32_t region);

/*!
 * @brief Set the readable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to set
 * @param R The desired value of the readable bit
 * @return 0 if the readable bit is successfully set
 */
int32_t pmp_set_readable(const struct pmp *pmp, uint32_t region, int32_t R);

/*!
 * @brief Set the readable bit for a PMP region
 * @param pmp The PMP device handle
 * @param region The PMP region to read
 * @return the value of the readable bit
 */
int32_t pmp_get_readable(const struct pmp *pmp, uint32_t region);

#endif //_LIB_RISCV_PMP_H
