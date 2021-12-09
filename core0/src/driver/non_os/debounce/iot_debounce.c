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
/* common includes */
#include "types.h"
#include "riscv_cpu.h"
#include "critical_sec.h"

#include "driver_dbglog.h"

#include "iot_irq.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "iot_timer.h"

#ifdef LOW_POWER_ENABLE
#include "power_mgnt.h"
#endif

#include "gpio.h"
#include "intc.h"

/* hw includes */
#include "gpi_debounce.h"

// Just set 0 is enough for now
#define IOT_DEBOUNCE_DEFAULT_COMMON_DIV 0
// This value let tatol divider to 1000, and debounce clk is 1000Hz
#define IOT_DEBOUNCE_DEFAULT_DIV 31
#define IOT_DEBOUNCE_INT_NUMBER  5
#define IOT_DEBOUNCE_GPIO_CH     (GPI_DEB_CH_MAX * GPI_DEB_MAX)
#define IOT_DEBOUNCE_GPI0_DEB_RST_DIV 1279

#define IOT_DEBOUNCE_MID_INT_SAVE_BIT 3
#define CHANNEL_FOR_DEBOUNCE_RESET 7
#define GPIO_SRC_FOR_DEBOUNCE_RESET 4
#define HARD_RESET_CHANNEL GPI_DEB_CH_3
#define DEB_RESET_RECONFIG_TIME_S 1000000
#define DEB_RESET_RECONFIG_DELAY_MS 80

typedef struct {
    iot_debounce_callback cb[IOT_DEBOUNCE_GPIO_CH];
    iot_irq_t irq[IOT_DEBOUNCE_INT_NUMBER];
    uint16_t gpio[IOT_DEBOUNCE_GPIO_CH];
    /**
     * bit 0: IO INT
     * bit 1: PRESS INT
     * bit 2: PRESS MID INT
     * bit 3: press mid store
     */
    uint8_t int_en[IOT_DEBOUNCE_GPIO_CH];
    uint8_t in_use;
} iot_debounce_state_t;

static uint32_t iot_debounce_isr_handler(uint32_t vector, uint32_t data);

static iot_debounce_state_t iot_debounce_info;

static uint8_t iot_debounce_reset_time = 0;
static uint8_t iot_debounce_cycle_timer = 0;
static bool_t iot_debounce_cycle_timer_active = false;
static uint8_t iot_debounce_clr_counter_flag = 1;
static uint32_t iot_debounce_reconfig_need_one_second = 0;
static uint32_t iot_debounce_reconfig_need_two_second = 0;

static void iot_debounce_interrupt_init(uint8_t ch)
{
    if (ch < GPI_DEB_CH_MAX) {
        // dig
        if (iot_debounce_info.irq[ch] == 0) {
            iot_debounce_info.irq[0] = iot_irq_create(GPIO_DEB_INT0 + ch, 0, iot_debounce_isr_handler);
            iot_irq_unmask(iot_debounce_info.irq[ch]);
        }
    } else {
        // pmm
        iot_debounce_info.irq[4] = iot_irq_create(PMM_GPIO_DEB_INT_ALL, 4, iot_debounce_isr_handler);
    }
}

static uint8_t iot_debounce_get_gpio_ch(uint16_t gpio)
{
    cpu_critical_enter();
    if (gpio == GPIO_AON_03) {
        // use channel 3 for pmm debounce reset
        cpu_critical_exit();
        return CHANNEL_FOR_DEBOUNCE_RESET;
    }
    // TK PAD as GPI always use pmm debounce
    if ((gpio >= GPIO_67 && gpio <= GPIO_70) || gpio >= GPIO_AON_START) {
        // Use pmm debounce
        for (uint8_t i = GPI_DEB_CH_MAX; i < IOT_DEBOUNCE_GPIO_CH; i++) {
            if (!(iot_debounce_info.in_use & BIT(i))) {
                iot_debounce_info.in_use |= (uint8_t)BIT(i);
                // channel 4-7 for pmm debounce
                cpu_critical_exit();
                return i;
            }
        }
    } else if (gpio < GPIO_AON_START) {
        for (uint8_t i = 1; i < GPI_DEB_CH_MAX; i++) {
            if (!(iot_debounce_info.in_use & BIT(i))) {
                iot_debounce_info.in_use |= (uint8_t)BIT(i);
                cpu_critical_exit();
                return i;
            }
        }
    } else {
        cpu_critical_exit();
        assert(false);
    }

    cpu_critical_exit();
    return IOT_DEBOUNCE_GPIO_CH;
}

static uint8_t iot_debounce_get_src(GPIO_ID gpio)
{
    if (gpio == GPIO_AON_03)
    {
        return GPIO_SRC_FOR_DEBOUNCE_RESET;
    }
    if ((gpio >= GPIO_67 && gpio <= GPIO_70)) {
        // tk use as gpi are map to pmm debounce src 30 - 33
        return (gpio - GPIO_67) + 30;
    } else if (gpio >= GPIO_AON_START) {
        // pmm io map to pmm debounce src 0-3
        return gpio - GPIO_AON_START;
    } else {
        // all others use dig deboucne
        return gpio;
    }
}

static uint8_t iot_debounce_cycle_timer_handler(iot_addrword_t data)
{
    UNUSED(data);
    //reset the debounce tidemark
    iot_debounce_set_time(CHANNEL_FOR_DEBOUNCE_RESET, iot_debounce_reset_time);
#ifdef LOW_POWER_ENABLE
    power_mgnt_set_sleep_vote(POWER_SLEEP_DEB);
#endif
    iot_debounce_cycle_timer_active = false;
    return 0;
}

void iot_debounce_init(void)
{
    gpi_debounce_init(GPI_DEB_DIG);
    for (uint8_t i = GPI_DEB_CH_0; i <= GPI_DEB_CH_3; i++) {
        iot_debounce_info.gpio[i] = 0xFF;
        gpi_debounce_disable(GPI_DEB_DIG, (GPI_DEB_CH)i);
    }

    gpi_debounce_init(GPI_DEB_PMM);
    // The channel_3 is used for deb reset, don't need to disable
    for (uint8_t j = GPI_DEB_CH_0; j <= GPI_DEB_CH_2; j++) {
        iot_debounce_info.gpio[j + GPI_DEB_CH_MAX] = 0xFF;
        gpi_debounce_disable(GPI_DEB_PMM, (GPI_DEB_CH)j);
    }

    // Set main divider
    iot_debounce_set_main_divider(IOT_DEBOUNCE_DEFAULT_COMMON_DIV);
}

void iot_debounce_deinit(void)
{
    for (uint8_t i = 0; i < GPI_DEB_CH_MAX; i++) {
        if (iot_debounce_info.irq[i]) {
            iot_irq_mask(iot_debounce_info.irq[i]);
            iot_irq_delete(iot_debounce_info.irq[i]);
        }
    }

    gpi_debounce_deinit(GPI_DEB_DIG);
}

void iot_debounce_set_main_divider(uint8_t div)
{
    gpi_debounce_set_common_clock_div(GPI_DEB_DIG, div);
    gpi_debounce_set_common_clock_div(GPI_DEB_PMM, div);
}

void iot_debounce_set_interrupt(uint8_t ch, const iot_debounce_int_cfg_t *cfg)
{
    assert(ch < IOT_DEBOUNCE_GPIO_CH);

    if (!cfg) {
        return;
    }

    GPI_DEB deb_ctrl = (ch >= GPI_DEB_CH_MAX) ? GPI_DEB_PMM : GPI_DEB_DIG;
    uint8_t hch = (ch >= GPI_DEB_CH_MAX) ? ch - GPI_DEB_CH_MAX : ch;

    iot_debounce_info.cb[ch] = cfg->cb;

    cpu_critical_enter();
    if (cfg->int_io_en) {
        iot_debounce_info.int_en[ch] |= BIT(IOT_DEBOUNCE_INT_IO);
        gpi_debounce_int_enable(deb_ctrl, (GPI_DEB_CH)hch, GPI_DEB_INT_IO);
    }

    if (cfg->int_press_en) {
        iot_debounce_info.int_en[ch] |= BIT(IOT_DEBOUNCE_INT_PRESS);
        gpi_debounce_int_enable(deb_ctrl, (GPI_DEB_CH)hch, GPI_DEB_INT_PRESS);
    }

    if (cfg->int_press_mid_en) {
        iot_debounce_info.int_en[ch] |= BIT(IOT_DEBOUNCE_INT_PRESS_MID);
        gpi_debounce_int_enable(deb_ctrl, (GPI_DEB_CH)hch, GPI_DEB_INT_PRESS_MID);
    }
    cpu_critical_exit();

    iot_debounce_interrupt_init(ch);
}

uint8_t iot_debounce_gpio(uint16_t gpio, uint8_t time_ms, IOT_DEBOUNCE_MODE mode,
                          const iot_debounce_int_cfg_t *cfg)
{
    uint8_t ch = iot_debounce_get_gpio_ch(gpio);
    uint8_t hch_src = (ch >= GPI_DEB_CH_MAX) ? ch - GPI_DEB_CH_MAX : ch;
    GPI_DEB deb_ctrl = (ch >= GPI_DEB_CH_MAX) ? GPI_DEB_PMM : GPI_DEB_DIG;
    GPI_DEB_CH hch = (GPI_DEB_CH)hch_src;

    cpu_critical_enter();
    iot_debounce_info.gpio[ch] = gpio;
    cpu_critical_exit();

    gpi_debounce_src_select(deb_ctrl, hch, iot_debounce_get_src((GPIO_ID)gpio));

    // Make the total divider is 1000
    gpi_debounce_set_clock_div(deb_ctrl, hch, IOT_DEBOUNCE_DEFAULT_DIV);
    gpi_debounce_set_threshold(deb_ctrl, hch, time_ms);
    gpi_debounce_out_edge_sel(deb_ctrl, hch, 0);

    switch (mode) {
        case IOT_DEBOUNCE_EDGE_RAISING:
            gpi_debounce_set_in_polarity(deb_ctrl, hch, false);
            gpi_debounce_set_out_polarity(deb_ctrl, hch, false);
            break;
        case IOT_DEBOUNCE_EDGE_FALLING:
            gpi_debounce_set_in_polarity(deb_ctrl, hch, true);
            gpi_debounce_set_out_polarity(deb_ctrl, hch, false);
            break;
        case IOT_DEBOUNCE_MODE_MAX:
            break;
        default:
            break;
    }

    iot_debounce_set_interrupt(ch, cfg);

    gpi_debounce_force_input_low(deb_ctrl, hch, false);
    gpi_debounce_force_input_high(deb_ctrl, hch, false);
    gpi_debounce_reset_enable(deb_ctrl, hch, false);
    gpi_debounce_enable(deb_ctrl, hch);

    if (deb_ctrl == GPI_DEB_PMM) {
        iot_irq_unmask(iot_debounce_info.irq[4]);
    }

    return ch;
}

uint8_t iot_debounce_gpio_disable(uint16_t gpio)
{
    uint8_t ret = RET_NOT_EXIST;
    for (uint32_t ch = 0; ch < IOT_DEBOUNCE_GPIO_CH; ch++) {
        /* one gpio may use several debounce channel */
        if (iot_debounce_info.gpio[ch] == gpio && ((iot_debounce_info.in_use & BIT(ch)) != 0)) {
            GPI_DEB deb_ctrl = (ch >= GPI_DEB_CH_MAX) ? GPI_DEB_PMM : GPI_DEB_DIG;
            GPI_DEB_CH hch = (GPI_DEB_CH)((ch >= GPI_DEB_CH_MAX) ? ch - GPI_DEB_CH_MAX : ch);

            gpi_debounce_disable(deb_ctrl, hch);
            iot_debounce_info.gpio[ch] = 0xFF;
            iot_debounce_info.in_use &= ~BIT(ch);
            ret = RET_OK;
        }
    }
    return ret;
}

void iot_debounce_enable_hard_reset(uint8_t time_40ms)
{
    iot_debounce_reset_time = time_40ms;

    // call hw api to save code size.
    uint8_t ch = iot_debounce_get_gpio_ch(IOT_AONGPIO_03);
    uint8_t hch_src = (ch >= GPI_DEB_CH_MAX) ? ch - GPI_DEB_CH_MAX : ch;
    GPI_DEB deb_ctrl = (ch >= GPI_DEB_CH_MAX) ? GPI_DEB_PMM : GPI_DEB_DIG;
    GPI_DEB_CH hch = (GPI_DEB_CH)hch_src;

    gpi_debounce_src_select(deb_ctrl, hch, iot_debounce_get_src((GPIO_ID)IOT_AONGPIO_03));

    // Make the total divider is 25
    gpi_debounce_set_clock_div(deb_ctrl, hch, IOT_DEBOUNCE_GPI0_DEB_RST_DIV);
    gpi_debounce_set_threshold(deb_ctrl, hch, time_40ms);
    gpi_debounce_out_edge_sel(deb_ctrl, hch, 0);

    // raising
    gpi_debounce_set_in_polarity(deb_ctrl, hch, false);
    gpi_debounce_set_out_polarity(deb_ctrl, hch, false);

    gpi_debounce_force_input_low(deb_ctrl, hch, false);
    gpi_debounce_force_input_high(deb_ctrl, hch, false);
    gpi_debounce_reset_enable(deb_ctrl, hch, true);
    gpi_debounce_enable(deb_ctrl, hch);
}

void iot_debounce_disable_hard_reset(void)
{
    gpi_debounce_set_threshold(GPI_DEB_PMM, HARD_RESET_CHANNEL, 0);
    gpi_debounce_reset_enable(GPI_DEB_PMM, HARD_RESET_CHANNEL, false);
}

void iot_debounce_set_time(uint8_t ch, uint8_t rst_time)
{
    GPI_DEB deb_ctrl = (ch >= GPI_DEB_CH_MAX) ? GPI_DEB_PMM : GPI_DEB_DIG;
    GPI_DEB_CH hch = (GPI_DEB_CH)((ch >= GPI_DEB_CH_MAX) ? ch - GPI_DEB_CH_MAX : ch);

    gpi_debounce_set_threshold(deb_ctrl, hch, rst_time);
}

void iot_debounce_reconfig_hard_reset_time(IOT_DEBOUNCE_CLEAR_TIMER_MODE clear_timer_mode)
{
    bool_t need_config = false;

    if ((iot_debounce_clr_counter_flag == 0) || iot_debounce_cycle_timer_active) {
        return;
    }

    if (iot_debounce_cycle_timer == 0) {
        iot_debounce_cycle_timer = iot_timer_create(iot_debounce_cycle_timer_handler, 0);
    }

    uint32_t current_time = iot_timer_get_time();

    if (clear_timer_mode == IOT_DEBOUNCE_NO_TIMER) {
        need_config = true;
    } else if (clear_timer_mode == IOT_DEBOUNCE_ONE_SECOND) {
        if ((current_time - iot_debounce_reconfig_need_one_second) > DEB_RESET_RECONFIG_TIME_S) {
            iot_debounce_reconfig_need_one_second = current_time;
            need_config = true;
        }
    } else if (clear_timer_mode == IOT_DEBOUNCE_TWO_SECONDS) {
        if ((current_time - iot_debounce_reconfig_need_two_second) > (DEB_RESET_RECONFIG_TIME_S * 2)) {
            iot_debounce_reconfig_need_two_second = current_time;
            need_config = true;
        }
    }

    if (need_config) {
        DBGLOG_DRIVER_DEBUG("[HARD RESET] counter clear.\n");
        gpi_debounce_set_threshold(GPI_DEB_PMM, HARD_RESET_CHANNEL, 0);
        if (iot_debounce_reset_time == 0) {
            iot_debounce_reset_time = IOT_DEBOUNCE_CHARGER_RESET_TIME;
        }

#ifdef LOW_POWER_ENABLE
        power_mgnt_clear_sleep_vote(POWER_SLEEP_DEB);
#endif

        iot_timer_set(iot_debounce_cycle_timer, DEB_RESET_RECONFIG_DELAY_MS * 1000, false);
        iot_timer_start(iot_debounce_cycle_timer);
        iot_debounce_cycle_timer_active = true;
    }
}

void iot_debounce_reconfig_hard_reset_time_with_delay(void)
{
    gpi_debounce_set_threshold(GPI_DEB_PMM, HARD_RESET_CHANNEL, 0);
    // delay 80ms
    iot_timer_delay_us(DEB_RESET_RECONFIG_DELAY_MS * 1000);
    gpi_debounce_set_threshold(GPI_DEB_PMM, HARD_RESET_CHANNEL, 250);
}

uint8_t iot_debounce_get_hard_reset_flag(void)
{
    return gpi_debounce_get_reset_flag(GPI_DEB_PMM, HARD_RESET_CHANNEL);
}

void iot_debounce_clear_hard_reset_flag(void)
{
    // Clear the reset flag
    gpi_debounce_int_clear(GPI_DEB_PMM, HARD_RESET_CHANNEL, GPI_DEB_INT_PRESS);
}

void iot_debounce_set_clr_counter_flag(uint8_t flag)
{
    if (iot_debounce_clr_counter_flag != flag) {
        DBGLOG_DRIVER_INFO("[HARD RESET] set clear counter flag:%d !\n", flag);
    }

    iot_debounce_clr_counter_flag = flag;

    if (flag == 1) {
        iot_debounce_reconfig_hard_reset_time(IOT_DEBOUNCE_NO_TIMER);
    }
}

uint8_t iot_debounce_get_clr_counter_flag(void)
{
    return iot_debounce_clr_counter_flag;
}

static void iot_debounce_int_handler(GPI_DEB deb, GPI_DEB_CH ch, uint8_t int_state)
    IRAM_TEXT(iot_debounce_int_handler);
static void iot_debounce_int_handler(GPI_DEB deb, GPI_DEB_CH ch, uint8_t int_state)
{
    uint8_t chn = deb == GPI_DEB_DIG ? ch : ch + 4;
    uint8_t state = int_state & (BIT(IOT_DEBOUNCE_INT_MAX) - 1);

    switch (state) {
        case 0:
            /* mid = 0, press = 0, io = 0 */
            return;
        case 1:
            /**
             * mid = 0, press = 0, io = 1
             * press too short, io int are self clear, no need clear
             */
            break;
        case 2:
            /**
             * mid = 0, press = 1, io = 0
             * normal press done
             * 1. call press cb
             * 2. restore mid int if necessary
             */
            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS)) {
                if (iot_debounce_info.cb[chn] != NULL) {
                    iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS);
                }

                // Restore mid int
                if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT)) {
                    iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                    iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                    gpi_debounce_int_enable(deb, ch, GPI_DEB_INT_PRESS_MID);
                }
            }
            break;
        case 3:
            /**
             * mid = 0, press = 1, io = 1
             * Could not be here
             */
            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT)) {
                /* mid int saved, it must have been handled */
                if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS)) {
                    if (iot_debounce_info.cb[chn] != NULL) {
                        iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS);
                    }
                }
                // Restore mid int
                iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                gpi_debounce_int_enable(deb, ch, GPI_DEB_INT_PRESS_MID);
            }
            break;
        case 4:
            /**
             * mid = 1, press = 0, io = 0
             * 1. The software additionally cleared mid_int in press int and turned on mid_int enable.
             * Since the counter needs 1 debounce cycle to clear after clearing press int,
             * mid_int will come again.
             *
             * 2.The key is just released when the counter is equal to the tidemark
             *
             * This is a incorrect interrupt, just clear int and drop it.
             */
            break;
        case 5:
            /**
             * mid = 1, press = 0, io = 1
             * normal mid int
             * 1. save mid int and disable it
             * 2. call mid cb
             */
            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS_MID)) {
                // Disable mid int and store int state
                iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                gpi_debounce_int_disable(deb, ch, GPI_DEB_INT_PRESS_MID);
                if (iot_debounce_info.cb[chn] != NULL) {
                    iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS_MID);
                }
            }
            break;
        case 6:
            /**
             * mid = 1, press = 1, io = 0
             * The key has been released. The mid_int interrupt (110 state) has been responded to
             * before, and the cleared mid_int is generated again when the press int is up.
             *
             * 1. If mid int wore handled, clear int and call press cb then
             *    restore mid int if necessary
             * 2. If no mid int wore handled, deal as a short press, drop it.
             */
            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT)) {
                /* mid int saved, it must have been handled */
                if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS)) {
                    if (iot_debounce_info.cb[chn] != NULL) {
                        iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS);
                    }
                }
                // Restore mid int
                iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                gpi_debounce_int_enable(deb, ch, GPI_DEB_INT_PRESS_MID);
            }
            break;
        case 7:
            /**
             * mid = 1, press = 1, io = 1
             * It may a press int left by last shutdown or crash just need deal as a normal mid int
             * and drop the press int
             */
            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT)) {
                /* mid int saved, it must have been handled */
                if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS)) {
                    if (iot_debounce_info.cb[chn] != NULL) {
                        iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS);
                    }
                }

                // Restore mid int
                iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                gpi_debounce_int_enable(deb, ch, GPI_DEB_INT_PRESS_MID);
            }

            if (iot_debounce_info.int_en[chn] & BIT(IOT_DEBOUNCE_INT_PRESS_MID)) {
                // Disable mid int and store int state
                iot_debounce_info.int_en[chn] &= ~BIT(IOT_DEBOUNCE_INT_PRESS_MID);
                iot_debounce_info.int_en[chn] |= BIT(IOT_DEBOUNCE_MID_INT_SAVE_BIT);
                gpi_debounce_int_disable(deb, ch, GPI_DEB_INT_PRESS_MID);
                if (iot_debounce_info.cb[chn] != NULL) {
                    iot_debounce_info.cb[chn](iot_debounce_info.gpio[chn], IOT_DEBOUNCE_INT_PRESS_MID);
                }
            }

            break;
        default:
            return;
    }

    /* Always clear all int */
    gpi_debounce_int_clear_all(deb, (GPI_DEB_CH)ch);
}

static uint32_t iot_debounce_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_debounce_isr_handler);
static uint32_t iot_debounce_isr_handler(uint32_t vector, uint32_t data)
{
    uint32_t ch = data;
    UNUSED(vector);
    if (ch < 4) {
        uint8_t state = gpi_debounce_get_int_state(GPI_DEB_DIG, (GPI_DEB_CH)ch);
        iot_debounce_int_handler(GPI_DEB_DIG, (GPI_DEB_CH)ch, state);
    } else {
        // For pmm debounce
        uint32_t state_all = gpi_debounce_get_int_state_all(GPI_DEB_PMM);
        for (ch = GPI_DEB_CH_0; ch < GPI_DEB_CH_MAX; ch++) {
            uint8_t state = (state_all >> (ch * 8)) & 0xFF;
            if (state) {
                iot_debounce_int_handler(GPI_DEB_PMM, (GPI_DEB_CH)ch, state);
            }
        }
    }

    return 0;
}

// TODO: remove when debug finish
void iot_debounce_sleep_debug(void)
{
    static uint8_t fail_times = 0;

    uint32_t state_all = gpi_debounce_get_int_state_all(GPI_DEB_PMM);
    uint32_t src = intc_get_source(PMM_GPIO_DEB_INT_ALL);
    uint32_t st = intc_get_status(PMM_GPIO_DEB_INT_ALL);
    uint32_t en = intc_get_enable(PMM_GPIO_DEB_INT_ALL);

    DBGLOG_DRIVER_ERROR("debounce pmm int:0x%08x, intc src:%d st:%d en:%d\n", state_all, src, st, en);

    fail_times++;

    if (fail_times > 10) {
        for (uint8_t ch = GPI_DEB_CH_0; ch < GPI_DEB_CH_MAX; ch++) {
            uint8_t state = (state_all >> (ch * 8)) & 0xFF;
            if (state) {
                gpi_debounce_int_clear_all(GPI_DEB_PMM, (GPI_DEB_CH)ch);
            }
        }
    }
}
