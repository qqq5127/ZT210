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
#ifndef LIB_CALDATA_H
#define LIB_CALDATA_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup CALDATA
 * @{
 * This section introduces the caldata module's enum, structure, functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define CAL_ANA_DUMMY_LEN        (4)
#define OEM_ANA_MAGIC_NUM        (0x5048595A)
#define DCCODE_CAL_MASK          (0xffff)
#define FTM_CAL_DATA_MASK_BIT(a) (0x00000001LL << (a))
#define FTM_CAL_DATA_MASK_CLR(a) (~(0x00000001LL << (a)))

#define ATE_CALI_VALID_FLAG      (0x55)
#define DEFAULT_DCDC1P8_DIV      (0.25f)
#define DEFAULT_DCDC1P2_DIV      (0.6f)
#define IC25UA_CURR_CODE         (16)
#define XTAL_CURR_CODE           (9)
#define DEFAULT_MV_PER_DEGREEC   (1.937f)
#define DEFAULT_OFFSET_MV_25C    (661.1f)
#define CAL_ANA_CFG_DCCODE_VER   (8)
#define ATE_CAL_CHECK_VER        (8)
#define VOUT_RATIO_LOW_LIMIT     (0.2f)
#define VOUT_RATIO_UP_LIMIT      (0.205f)
#define VOUT_TESTER_LOW_VOL      (3500)
#define VOUT_TESTER_UP_VOL       (4200)

#define CAL_DATA_TOTAL_SIZE 4096
#define CAL_DATA_RF_SIZE    1024
#define CAL_DATA_ANA_SIZE   1024
#define CAL_DATA_DUMMY_SIZE \
    (CAL_DATA_TOTAL_SIZE - CAL_DATA_RF_SIZE - CAL_DATA_RF_SIZE)

/** @defgroup lib_caldata_enum Enum
  * @{
  */
typedef enum _CAL_DATA_ANA_MASK {
    MADC_CAL_DATA                     = 0,
    BG_TRIM_CODE                      = 1,
    IC25UA_TRIM_CODE                  = 2,
    PMM_LDO_TRIM_CODE                 = 3,
    XTAL_TRIM_CODE                    = 4,
    TEMP_CAL_DATA                     = 5,
    CHG_VOUT_TRIM_CODE                = 6,
    CHG_VIN_MEAS_RATIO                = 7,
    CHG_IOUT_RANGE_TRIM_CODE          = 8,
    CHG_IOUT_STEP_TRIM_CODE           = 9,
    CHG_UVP_DEFAULT_VAL               = 10,
    AON_LDO_TRIM_CODE                 = 11,
    DCDC1P8_TRIM_CODE                 = 12,
    DCDC1P2_TRIM_CODE                 = 13,
    DCDC0P8_TRIM_CODE                 = 14,
    DIGIT0P6_TRIM_CODE                = 15,
    METER_LDO_TRIM_CODE               = 16,
    FLASH_LDO_TRIM_CODE               = 17,
    PLL_LDO_TRIM_CODE                 = 18,
    XTAL_LDO_TRIM_CODE                = 19,
    BB_LDO_TRIM_CODE                  = 20,
    LNA_LDO_TRIM_CODE                 = 21,
    BTADC_LDO_TRIM_CODE               = 22,
    LSPK_VCM_TRIM_CODE                = 23,
    RSPK_VCM_TRIM_CODE                = 24,
    LSPK_VREF_TRIM_CODE               = 25,
    RSPK_VREF_TRIM_CODE               = 26,
    LSPK_DC_TRIM_CODE                 = 27,
    RSPK_DC_TRIM_CODE                 = 28,
    MICBIAS_TRIM_CODE                 = 29,
    MIC_VREF_TRIM_CODE                = 30,
    MIC_VCM_TRIM_CODE                 = 31,
    TK_LDO_TRIM_CODE                  = 32,
    TK_LSB_TRIM                       = 33,
    TK_VREF_TRIM_CODE                 = 34,
    TIA_DC_TRIM_CODE                  = 35,
    DIGIT0P56_TRIM_CODE               = 36,
    LSPK_DC_TRIM_CODE_1P4             = 37,
    RSPK_DC_TRIM_CODE_1P4             = 38,
    CURR_IDEL_VAL                     = 39,
    CURR_AUDIO_VAL                    = 40,
    CURR_RF_VAL                       = 41,
    MAX_EFF_DCDC0P8                   = 42,
    MAX_EFF_DCDC1P2                   = 43,
    MAX_EFF_DCDC1P8                   = 44,
    MIC_ADC_BIAS_INT2_CODE            = 45,
    LSPK_VCM_TRIM_CODE_0P7            = 46,
    RSPK_VCM_TRIM_CODE_0P7            = 47,
    MAX_ANA_CAL_DATA_BITS,
} CAL_DATA_ANA_MASK;
/**
  * @}
  */

/** @defgroup lib_caldata_typedef Typedef
  * @{
  */

/**
  * @}
  */
/* pack for the structures in the whole file */
#pragma pack(push)   // save the pack status
#pragma pack(1)      // 1 byte align

/** @defgroup lib_caldata_struct Struct
  * @{
  */

/* analog total ctxt entry */
typedef struct _ana_ctxt {
    //cal flag
    uint64_t temp_cal : 1,
             madc_cal : 1,
             vbg_cal : 1,
             ic25ua_cal : 1,
             pmm_ldo_cal : 1,
             xtal_cal : 1,
             chg_vout_cal : 1,
             chg_vin_cal : 1,
             chg_iout_range_cal : 1,
             chg_iout_step_cal : 1,
             chg_uvp_cal : 1,
             dcdc1P8_cal : 1,
             dcdc1P2_cal : 1,
             dcdc0P8_cal : 1,
             left_spk_dc_cal : 1,
             right_spk_dc_cal : 1,
             lspk_vcm_cal : 1,
             rspk_vcm_cal : 1,
             lspk_vref_cal : 1,
             rspk_vref_cal : 1,
             micbias_cal : 1,
             mic_vref_cal : 1,
             mic_vcm_cal : 1,
             aon_ldo_cal : 1,
             flash_ldo_cal : 1,
             pll_ldo_cal : 1,
             xtal_ldo_cal : 1,
             digital_ldo_cal : 1,
             meter_ldo_cal : 1,
             bb_ldo_cal : 1,
             lna_ldo_cal : 1,
             btadc_ldo_cal : 1,
             tk_ldo_cal : 1,
             tk_lsb_cal : 1,
             tk_vref_cal : 1,
             tia_cal : 1,
             left_spk_dc_cal_1p4 : 1,
             right_spk_dc_cal_1p4 : 1,
             dc0p8_eff_cal : 1,
             dc1p2_eff_cal : 1,
             dc1p8_eff_cal : 1,
             adc_bias_int2_cal : 1,
             lspk_vcm_700mv_cal : 1,
             rspk_vcm_700mv_cal : 1,
             resv : 20;
    //madc
    float offset_mv_25C;
    float mv_per_degreeC;
    float vrefpn_minus700mv;
    float vcm_mv;
    float dcdc_v1p8_div;
    float dcdc_v1p2_div;
    //charger
    float chg_vin_madc_ratio;
    float chg_vout_madc_ratio;
    float chg_vout_v4p0_mv;
    float chg_vout_v4p3_mv;
    float uvp_3p1_mv;
    float chg_iout_dcdc_curr_ma;
    float chg_iout_seg0_code0_ma;
    float chg_iout_seg0_code63_ma;
    float chg_iout_seg1_code0_ma;
    float chg_iout_seg1_code63_ma;
    float chg_iout_seg2_code0_ma;
    float chg_iout_seg2_code63_ma;
    float chg_iout_seg3_code0_ma;
    float chg_iout_seg3_code63_ma;
    //tk
    float tk_lsb;
    int32_t dc_offset_code;
    //pmu
    uint8_t vbg_trim_code;
    int8_t pmu_rsv;
    uint8_t ic25ua_curr_code;
    uint8_t pmm_ldo_code;
    uint8_t xtal_trim_code;
    uint8_t dcdc_v1p8_trim_code;
    uint8_t dcdc_v1p2_trim_code;
    uint8_t dcdc_v0p8_trim_code;
    uint8_t dig_ldo_v0p56_trim_code;
    uint8_t aon_ldo_trim_code;
    uint8_t flash_ldo_trim_code;
    uint8_t pll_ldo_trim_code;
    uint8_t xtal_ldo_trim_code;
    uint8_t digital_ldo_trim_code;
    uint8_t meter_ldo_trim_code;
    uint8_t bb_ldo_trim_code;
    uint8_t lna_ldo_trim_code;
    uint8_t btadc_ldo_trim_code;
    //audio
    uint8_t left_spk_coden;
    uint8_t left_spk_codep;
    uint8_t right_spk_coden;
    uint8_t right_spk_codep;
    uint8_t left_spk_coden_1p4;
    uint8_t left_spk_codep_1p4;
    uint8_t right_spk_coden_1p4;
    uint8_t right_spk_codep_1p4;
    uint8_t lspk_vref_trim_code;
    uint8_t lspk_vcom_trim_code;
    uint8_t lspk_vcom_700mv_trim_code;
    uint8_t rspk_vref_trim_code;
    uint8_t rspk_vcom_trim_code;
    uint8_t rspk_vcom_700mv_trim_code;
    uint8_t micbias_trim_code;
    uint8_t mic_vref_trim_code[9];
    uint8_t mic_vcm_trim_code[4];
    uint8_t mic0_adc_bias_int2_ctrl;
    uint8_t mic1_adc_bias_int2_ctrl;
    uint8_t mic2_adc_bias_int2_ctrl;
    uint8_t mic_vcm_rsv[2];
    //TK
    uint8_t tk_ldo_trim_code;
    uint16_t tk_vref_code;
    //tia
    uint8_t offset_trim_code;
    //charger
    uint8_t chg_vout_trim_code;
    uint8_t chg_iout_pmos_cal;
    uint8_t uvp_3p1_trim_code;
    uint8_t chg_iout_res_cal[4];
    //max efficiency
    uint8_t dc0p8_zccode;
    uint8_t dc1p2_zccode;
    uint8_t dc1p8_zccode;
    int8_t padding[3];
    //audio  dc_offset cal data
    float left_spk_dc_offset;
    float left_spk_dc_offset_1p4;
    float right_spk_dc_offset;
    float right_spk_dc_offset_1p4;
} ana_ctxt_t;

/* charger cal data */
typedef struct _cal_data_charger {
    float chg_vin_madc_ratio;
    float chg_vout_madc_ratio;
    float chg_vout_v4p0_mv;
    float chg_vout_v4p3_mv;
    float uvp_3p1_mv;
    float chg_dcdc_iout_ma;
    float chg_iout_range[4][2];
} cal_data_charger;

/* madc cal data */
typedef struct _cal_data_madc {
    float vrefpn_minus700mv;
    float vcm_mv;
    int32_t dc_offset_code;
} cal_data_madc;

/* temp cal data */
typedef struct _cal_data_temp {
    float offset_mv_25C;
    float mv_per_degreeC;
} cal_data_temp;

typedef struct _cali_cmd_val {
    uint32_t cmd : 8,
             code : 8,
             val : 16;
} cali_cmd_val;

typedef struct _ate_cali_reg {
    cali_cmd_val aon_vdd3p1_mv;
    cali_cmd_val dcdc_v1p8_mv;
    cali_cmd_val dcdc_v1p2_mv;
    cali_cmd_val dcdc_v0p8_mv;
    cali_cmd_val digital_0p6_mv;
    cali_cmd_val digital_0p56_mv;
    cali_cmd_val meter_ldo_mv;
    cali_cmd_val flash_ldo_mv;
    cali_cmd_val pll_ldo_mv;
    cali_cmd_val xtal_ldo_mv;
    cali_cmd_val bb_ldo_mv;
    cali_cmd_val lna_ldo_mv;
    cali_cmd_val btadc_ldo_mv;
    cali_cmd_val ic25ua_curr_na;
    cali_cmd_val xtal_curr_na;
    cali_cmd_val chg_vout_v4p2_mv;
    cali_cmd_val chg_vout_v4p0_mv;
    cali_cmd_val chg_vout_v4p3_mv;
    cali_cmd_val tk_ldo_mv;
    cali_cmd_val idle_curr_ua;
    cali_cmd_val audio_curr_ua;
    cali_cmd_val rf_curr_ua;
    uint8_t padding[40];   //padding to 128bytes
} tester_cali_res;

/* analog config setting */
typedef struct _iot_cal_data_ana_cfg {
    /* magic num */
    uint32_t magic;

    /* hardware version */
    uint16_t hw_ver_major;      //ate program version
    uint8_t hw_ver_minor;      //flash calib version
    uint8_t hw_ver_tester;    //tester program version

    /* mask */
    uint64_t mask;

    //madc
    float offset_mv_25C;
    float mv_per_degreeC;
    float vrefpn_minus700mv;
    float vcm_mv;
    float dcdc_v1p8_div;
    float dcdc_v1p2_div;
    int8_t madc_rsv[1];
    uint8_t ic25ua_curr_code;
    int32_t dc_offset_code;
    uint8_t xtal_trim_code;
    //pmu
    uint8_t vbg_trim_code;
    uint8_t ic25ua_ldo_code;
    uint8_t pmm_ldo_code;
    uint8_t dcdc_v1p8_trim_code;
    uint8_t dcdc_v1p2_trim_code;
    uint8_t dcdc_v0p8_trim_code;
    uint8_t dig_ldo_v0p56_trim_code;
    //tia
    uint8_t offset_trim_code;
    //tk
    uint8_t tk_ldo_trim_code;
    float tk_lsb;
    uint16_t tk_vref_code;
    uint8_t tk_rsv[1];
    //audio
    uint8_t lspk_dc_coden;
    uint8_t lspk_dc_codep;
    uint8_t rspk_dc_coden;
    uint8_t rspk_dc_codep;
    uint8_t lspk_vref_trim_code;
    uint8_t lspk_vcom_trim_code;
    uint8_t rspk_vref_trim_code;
    uint8_t rspk_vcom_trim_code;
    uint8_t micbias_trim_code;
    uint8_t mic_vref_trim_code[4];
    uint8_t lspk_dc_coden_1p4;
    uint8_t lspk_dc_codep_1p4;
    uint8_t rspk_dc_coden_1p4;
    uint8_t rspk_dc_codep_1p4;
    uint8_t mic_vref_rsv[1];
    uint8_t mic_vcm_trim_code[4];
    uint8_t mic0_adc_bias_int2_ctrl;
    uint8_t mic1_adc_bias_int2_ctrl;
    uint8_t mic2_adc_bias_int2_ctrl;
    uint8_t lspk_vcom_700mv_trim_code;
    uint8_t rspk_vcom_700mv_trim_code;
    //pmu tested by platform
    uint8_t aon_ldo_trim_code;
    uint8_t flash_ldo_trim_code;
    uint8_t pll_ldo_trim_code;
    uint8_t xtal_ldo_trim_code;
    uint8_t digital_ldo_trim_code;
    uint8_t meter_ldo_trim_code;
    uint8_t bb_ldo_trim_code;
    uint8_t lna_ldo_trim_code;
    uint8_t btadc_ldo_trim_code;
    //charger
    uint8_t chg_vout_trim_code;
    float chg_vin_madc_ratio;
    float chg_vout_madc_ratio;
    float chg_vout_v4p0_mv;
    float chg_vout_v4p3_mv;
    float uvp_3p1_mv;
    float chg_iout_dcdc_curr_ma;
    float chg_iout_seg0_code0_ma;
    float chg_iout_seg0_code63_ma;
    float chg_iout_seg1_code0_ma;
    float chg_iout_seg1_code63_ma;
    float chg_iout_seg2_code0_ma;
    float chg_iout_seg2_code63_ma;
    float chg_iout_seg3_code0_ma;
    float chg_iout_seg3_code63_ma;
    uint8_t chg_iout_range_trim_code;
    uint8_t chg_iout_pmos_cal;
    uint8_t uvp_3p1_trim_code;
    uint8_t chg_iout_res_cal[4];

    uint8_t rco_pass_flag : 1,
            deep_pass_flag : 1,
            rsv : 6;

    tester_cali_res tester_calib_val;
    uint8_t dc0p8_zccode;
    uint8_t dc1p2_zccode;
    uint8_t dc1p8_zccode;
    float left_spk_dc_offset;
    float left_spk_dc_offset_1p4;
    float right_spk_dc_offset;
    float right_spk_dc_offset_1p4;
    uint8_t reserved[CAL_ANA_DUMMY_LEN];
} cal_data_ana_cfg_t;

typedef struct _cal_ana_data_cfg {
    uint16_t head_ver;
    uint8_t crc;
    cal_data_ana_cfg_t ana_cfg;   //512 offset
} cal_ana_data_cfg_t;

typedef struct _cal_data_t {
    uint8_t rf_data[CAL_DATA_RF_SIZE];
    uint8_t ana_data[CAL_DATA_ANA_SIZE];
    uint8_t dummy_data[CAL_DATA_DUMMY_SIZE];
} cal_data_t;
/**
  * @}
  */

/**
 * @brief  This function is to load cal data;
 * @return cal_data_t cal data's pointer with flash mapping address.
 */
cal_data_t *cal_data_load(void);

/**
 * @brief  This function is to load cal data to ana_ctxt_t
 * @return 0 success; others fail.
 */
uint32_t cal_data_ana_load(void);

/**
 * @brief This function is to get cal data with mask.
 *
 * @param mask is cal data's mask.
 * @return uint32_t cal data.
 */
uint32_t cal_data_trim_code_get(CAL_DATA_ANA_MASK mask);

/**
 * @brief This function is to load chargercal data to struct.
 *
 * @param chg_para is charger paremeter struct.
 */
void cal_data_charger_get(cal_data_charger *chg_para);

/**
 * @brief This function is to load charger cal data to struct.
 *
 * @param madc_para is madc paremeter struct.
 */
void cal_data_madc_get(cal_data_madc *madc_para);

/**
 * @brief This function is to load temp cal data to struct.
 *
 * @param temp_para is temp paremeter struct.
 */
void cal_data_temp_get(cal_data_temp *temp_para);

/**
 * @brief This function is to dump cal data info.
 */
void cal_data_ana_cfg_dump(void);

#pragma pack(pop)   // restore the pack status

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup CALDATA
 */

/**
* @}
 * addtogroup LIB
*/

#endif   //LIB_CALDATA_H
