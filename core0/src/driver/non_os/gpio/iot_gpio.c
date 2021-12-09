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
#include "chip_irq_vector.h"

/* hw includes */
#include "gpio.h"
#include "iot_gpio.h"
#include "iot_irq.h"

#include "iot_resource.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

/*lint -esym(754, gpio_status::w) */
/*lint -esym(754, wakeup_ena) */
/*lint -esym(754, reserved) */
typedef struct iot_gpio_info {
    iot_gpio_int_callback cb;
    union gpio_status {
        uint32_t w;
        struct {
            uint32_t use : 1;
            uint32_t dir : 1;
            uint32_t out_value : 1;
            uint32_t int_en : 3;
            uint32_t int_mode : 3;
            uint32_t pull : 2;
            uint32_t wakeup_ena : 1;
            uint32_t reserved : 22;
        } b;
    } st;
} iot_gpio_info_t;

static iot_gpio_info_t iot_gpio_info[GPIO_MAX];
static iot_irq_t iot_gpio_irq;
static iot_irq_t iot_gpio_pmm_irq;

#ifdef LOW_POWER_ENABLE
const uint16_t iot_gpio_restore_list[] = {GPIO_00, GPIO_01, GPIO_02, GPIO_03, GPIO_07,
                                          GPIO_63, GPIO_64, GPIO_66, GPIO_69, GPIO_70,
                                          GPIO_71, GPIO_72, GPIO_73, GPIO_74, GPIO_75, GPIO_76};
static struct pm_operation iot_gpio_pm;
#endif

static uint32_t iot_gpio_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_gpio_isr_handler);
static uint32_t iot_gpio_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

    for (uint16_t i = 0; i < GPIO_MAX; i++) {
        if (gpio_get_int_state(i) != 0) {
            if (iot_gpio_info[i].cb) {
                iot_gpio_info[i].cb();
            }

            gpio_int_clear(i);
        }
    }

    return 0;
}

static void iot_gpio_interrupt_config(void)
{
    gpio_int_disable_all();

    iot_gpio_irq = iot_irq_create(GPIO_INT, 0, iot_gpio_isr_handler);
    iot_irq_unmask(iot_gpio_irq);

    iot_gpio_pmm_irq = iot_irq_create(PMM_GPIO_INT, 0, iot_gpio_isr_handler);
    iot_irq_unmask(iot_gpio_pmm_irq);
}

uint32_t iot_gpio_save(uint32_t data)
{
#ifdef LOW_POWER_ENABLE
    /** Nothing to do here */
#endif
    UNUSED(data);
    return RET_OK;
}

uint32_t iot_gpio_restore(uint32_t data)
{
#ifdef LOW_POWER_ENABLE
    for (uint32_t i = 0; i < ARRAY_SIZE(iot_gpio_restore_list); i++) {
        uint16_t gpio = iot_gpio_restore_list[i];
        if (iot_gpio_info[gpio].st.b.pull != IOT_GPIO_PULL_NONE) {
            gpio_set_pull_mode(gpio, (GPIO_PULL_MODE)iot_gpio_info[gpio].st.b.pull);
        }

        if (iot_gpio_info[gpio].st.b.use) {
            /* opened by software as an normal gpio */
            gpio_release(gpio);
            if (iot_gpio_info[gpio].st.b.dir == IOT_GPIO_DIRECTION_INPUT) {
                // Input
                gpio_open(gpio, GPIO_DIRECTION_INPUT);
                gpio_int_enable(gpio, (GPIO_INT_MODE)iot_gpio_info[gpio].st.b.int_mode);
            } else {
                // Output
                gpio_open(gpio, GPIO_DIRECTION_OUTPUT);
                gpio_write(gpio, iot_gpio_info[gpio].st.b.out_value);
            }
        }
    }
#endif
    UNUSED(data);
    return RET_OK;
}

void iot_gpio_init(void)
{
    gpio_init();
    iot_gpio_interrupt_config();

#ifdef LOW_POWER_ENABLE
    for (uint32_t i = 0; i < ARRAY_SIZE(iot_gpio_restore_list); i++) {
        uint16_t gpio = iot_gpio_restore_list[i];
        if (iot_resource_check_gpio_configured((uint8_t)gpio)) {
            iot_gpio_set_pull_mode(gpio, iot_resource_lookup_pull_mode((uint8_t)gpio));
        } else if (gpio == GPIO_63 || gpio == GPIO_64 || gpio == GPIO_66) {
            /* pmm gpio configured in sbl */
        } else {
            /* other gpio default pull down */
            iot_gpio_set_pull_mode(gpio, IOT_GPIO_PULL_DOWN);
        }
    }

    list_init(&iot_gpio_pm.node);
    iot_gpio_pm.save = iot_gpio_save;
    iot_gpio_pm.restore = iot_gpio_restore;
    iot_dev_pm_register(&iot_gpio_pm);
#endif
}

void iot_gpio_deinit(void)
{
    gpio_int_disable_all();

    iot_irq_mask(iot_gpio_irq);
    iot_irq_delete(iot_gpio_irq);

    iot_irq_mask(iot_gpio_pmm_irq);
    iot_irq_delete(iot_gpio_pmm_irq);

    gpio_deinit();
}

uint8_t iot_gpio_open(uint16_t gpio, IOT_GPIO_DIRECTION dir)
{
    iot_gpio_info[gpio].st.b.use = 1;
    iot_gpio_info[gpio].st.b.out_value = 0;
    iot_gpio_info[gpio].st.b.dir = dir;

    return gpio_open(gpio, (GPIO_DIRECTION)dir);
}

uint8_t iot_gpio_open_as_interrupt(uint16_t gpio, IOT_GPIO_INT_MODE mode, iot_gpio_int_callback cb)
{
    uint32_t mask = cpu_disable_irq();

    iot_gpio_info[gpio].cb = cb;
    iot_gpio_info[gpio].st.b.use = 1;
    iot_gpio_info[gpio].st.b.dir = IOT_GPIO_DIRECTION_INPUT;
    iot_gpio_info[gpio].st.b.int_mode = mode;

    // Set gpio to input mode
    uint8_t ret = gpio_open(gpio, GPIO_DIRECTION_INPUT);
    if (ret != RET_OK) {
        cpu_restore_irq(mask);
        return ret;
    }
    gpio_int_enable(gpio, (GPIO_INT_MODE)mode);

    cpu_restore_irq(mask);

    return ret;
}

void iot_gpio_int_enable(uint16_t gpio)
{
    iot_gpio_info[gpio].st.b.int_en = 1;
    gpio_int_enable(gpio, (GPIO_INT_MODE)iot_gpio_info[gpio].st.b.int_mode);
}

void iot_gpio_int_disable(uint16_t gpio)
{
    iot_gpio_info[gpio].st.b.int_en = 0;
    gpio_int_disable(gpio);
}

void iot_gpio_close(uint16_t gpio)
{
    gpio_close(gpio);
    iot_gpio_info[gpio].st.b.use = 0;
}

void iot_gpio_write(uint16_t gpio, uint8_t value) IRAM_TEXT(iot_gpio_write);
void iot_gpio_write(uint16_t gpio, uint8_t value)
{
    iot_gpio_info[gpio].st.b.out_value = value;
    gpio_write(gpio, value);
}

uint8_t iot_gpio_read(uint16_t gpio)
{
    return gpio_read(gpio);
}

void iot_gpio_toggle(uint16_t gpio)
{
    iot_gpio_info[gpio].st.b.out_value ^= 1;
    gpio_toggle(gpio);
}

void iot_gpio_set_pull_mode(uint16_t gpio, IOT_GPIO_PULL_MODE mode)
{
    iot_gpio_info[gpio].st.b.pull = mode;
    gpio_set_pull_mode(gpio, (GPIO_PULL_MODE)mode);
}

void iot_gpio_get_wakeup_source(uint16_t *gpio, uint8_t *level)
{
    gpio_get_wakeup_source(gpio, level);
}

int8_t iot_gpio_set_drive(uint16_t gpio, IOT_GPIO_DRIVE_MODE drv)
{
    return gpio_set_drive(gpio, (GPIO_DRIVE_MODE)drv);
}
