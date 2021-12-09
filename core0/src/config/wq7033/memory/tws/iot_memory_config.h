#include "iot_memory_origin.h"
#ifdef BUILD_PATCH
#include "rom_memory.h"
#endif
/**
 * DTOP rom total 32K
 */
#define DTOP_ROM_START          DCP_ROM_START
#define DTOP_ROM_LENGTH         DCP_ROM_LENGTH

/**
 * BT rom total 1M (512K*2)
 */
#define BT_ROM_START            BCP_ROM_START
#define BT_ROM_LENGTH           BCP_ROM_LENGTH

/**
 * DTOP iram total 64K (32K*2 + 64K)
 */
#define DTOP_IRAM_START         DCP_IRAM_START

#ifdef BT_USE_DTOP_IRAM
#define DTOP_IRAM_SHARE_TO_BT   0x7000
#define BT_IRAM2_START          (DCP_IRAM_START + DCP_IRAM_LENGTH - SHARE_MEMORY_LENGTH - DTOP_IRAM_SHARE_TO_BT)
#define BT_IRAM2_LENGTH         DTOP_IRAM_SHARE_TO_BT

#define DTOP_IRAM_LENGTH        (DCP_IRAM_LENGTH - SHARE_MEMORY_LENGTH - DTOP_IRAM_SHARE_TO_BT)
#else
#define DTOP_IRAM_LENGTH        (DCP_IRAM_LENGTH - SHARE_MEMORY_LENGTH)
#endif

/**
 * BT iram total 288K (128K + 64K*2 + 32K)
 */
#ifdef BUILD_PATCH
#define BT_IRAM_START           (BCP_IRAM_START + ROM_IRAM_LENGTH)
#define BT_IRAM_LENGTH          (BCP_IRAM_LENGTH - ROM_IRAM_LENGTH)
#else
#define BT_IRAM_START           BCP_IRAM_START
#define BT_IRAM_LENGTH          BCP_IRAM_LENGTH
#endif

/* Share memory */
#define SHARE_MEMORY_LENGTH     (WIC_DEPENDENCE_SCRATCH_LENGTH + GENERIC_TRANSMISSION_LENGTH + RING_ORIGIN_LENGTH + IPC_MESSAGE_LENGTH)
#define SHARE_MEMORY_START      (DCP_IRAM_START + DCP_IRAM_LENGTH - SHARE_MEMORY_LENGTH)

#define AUDIO_IRAM_START         AUD_S0_TCM_DRAM_BLK0_BASEADDR
#define AUDIO_IRAM_LENGTH        0x40000


#define WIC_DEPENDENCE_SCRATCH_START       SHARE_MEMORY_START
#define WIC_DEPENDENCE_SCRATCH_LENGTH      0x40
/* bt share data is in WIC_DEPENDENCE_SCRATCH */
#define BT_SHARE_DATA_LENGTH               0x8
#define BT_SHARE_DATA_START                (SHARE_MEMORY_START + WIC_DEPENDENCE_SCRATCH_LENGTH - BT_SHARE_DATA_LENGTH)
#define DTOP_SHARE_DATA_LENGTH             0x4
#define DTOP_SHARE_DATA_START              (SHARE_MEMORY_START + WIC_DEPENDENCE_SCRATCH_LENGTH - BT_SHARE_DATA_LENGTH - DTOP_SHARE_DATA_LENGTH)

/* Generic transmission */
#define GENERIC_TRANSMISSION_CTRL_SIZE     32
#define GENERIC_TRANSMISSION_DTOP_START    GENERIC_TRANSMISSION_START
/* DTOP can directly use API call, 0 indicates give up to use share memory */
#define GENERIC_TRANSMISSION_DTOP_LENGTH   0x800
#define GENERIC_TRANSMISSION_BT_START      (GENERIC_TRANSMISSION_DTOP_START + GENERIC_TRANSMISSION_DTOP_LENGTH)
/* The buffer head is used for GENERIC_TRANSMISSION_CTRL_SIZE */
#define GENERIC_TRANSMISSION_BT_LENGTH     0x800
#define GENERIC_TRANSMISSION_DSP_START     (GENERIC_TRANSMISSION_BT_START + GENERIC_TRANSMISSION_BT_LENGTH)
#define GENERIC_TRANSMISSION_DSP_LENGTH    0x800

#define GENERIC_TRANSMISSION_START         (WIC_DEPENDENCE_SCRATCH_START + WIC_DEPENDENCE_SCRATCH_LENGTH)
#define GENERIC_TRANSMISSION_LENGTH        (GENERIC_TRANSMISSION_DTOP_LENGTH + GENERIC_TRANSMISSION_BT_LENGTH + GENERIC_TRANSMISSION_DSP_LENGTH)

/**
 * ring for rpc data transport origin definiation
 * At end of th dcp iram
 * AUDIO push to   DTOP 30K
 * BT    push to   DTOP 1K
 * BT    pull from DTOP 1K
 * Total size           32k
 */
#define RING_DFA_START          RING_ORIGIN_START
#define RING_DFA_LENGTH         0x7800
#define RING_DTB_START          (RING_DFA_START + RING_DFA_LENGTH)
#define RING_DTB_LENGTH         0x400
#define RING_DFB_START          (RING_DTB_START + RING_DTB_LENGTH)
#define RING_DFB_LENGTH         0x400

#define RING_ORIGIN_LENGTH      (RING_DFA_LENGTH + RING_DTB_LENGTH + RING_DFB_LENGTH)
#define RING_ORIGIN_START       (GENERIC_TRANSMISSION_DTOP_START + GENERIC_TRANSMISSION_LENGTH)

/* ipc message */
#define IPC_MESSAGE_LENGTH      0x800
#define IPC_MESSAGE_START       (RING_ORIGIN_START + RING_ORIGIN_LENGTH)

/* Flash code memory (512K for DTOP) */
#define DTOP_FLASH_START        (FLASH_DCP_OFFSET + IMAGE_HEADER_LEN)
#define DTOP_FLASH_LENGTH       (FLASH_DCP_LENGTH - IMAGE_HEADER_LEN)
#define DTOP_RAM_START          (DTOP_IRAM_START)
#define DTOP_RAM_LENGTH         (DTOP_IRAM_LENGTH)

/* BT patch TBL */
#ifdef BUILD_PATCH
#define BT_TBL_LENGTH           0x6000
#else
#define BT_TBL_LENGT            0x0
#endif

#define BT_FLASH_START          (FLASH_BCP_OFFSET + IMAGE_HEADER_LEN)
#define BT_FLASH_LENGTH         (FLASH_BCP_LENGTH - IMAGE_HEADER_LEN - BT_TBL_LENGTH)
#define BT_RAM_START            (BT_IRAM_START)
#define BT_RAM_LENGTH           (BT_IRAM_LENGTH)