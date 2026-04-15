#include <exti.h>
#include <gpio.h>
#include <printk.h>
#include <rcc.h>
#include <nvic.h>


/** @brief System Config register map. */
struct syscfg
{
    volatile uint32_t memrmp;  /**< 00 Memory Remap */
    volatile uint32_t pmc;     /**< 04 Peripheral mode configuration */
    volatile uint32_t exti[4]; /**< 08-14 External interrupt configuration */
    volatile uint32_t cmpcr;   /**< 20 Compensation cell control */
};

#define SYSCFG_BASE (struct syscfg *)0x40013800

#define BITS_PER_EXTI 4

#define RCC_APB2_SYSCFG_EN (1 << 14)
#define NVIC_EXTI_BASE 6
#define NVIC_EXTI9_5 23
#define NVIC_EXTI15_10 40

// Rising edge -> edge = 0b01
// Falling edge -> edge = 0b10
// Rising-Falling edge -> edge = 0b11
void enable_exti(gpio_port port, uint32_t channel, uint32_t edge) {
    struct exti* e = EXTI_BASE;
    struct rcc_reg_map *rcc = RCC_BASE;
    struct syscfg* s = SYSCFG_BASE;
    uint32_t syscfg_exti = channel / 4;

    rcc -> apb2_enr |= RCC_APB2_SYSCFG_EN;

    uint32_t exti_port = 0;
    switch (port) {
    	case GPIO_A: 
	    exti_port = 0;
	    break;
	case GPIO_B:
	    exti_port = 1;
	    break;
	case GPIO_C:
	    exti_port = 2;
	    break;
    }
    s -> exti[syscfg_exti] |= exti_port << ((channel % 4) * 4);
    if (edge & 0x1) {
        e -> rtsr |= (1 << channel);
    }
    if (edge & 0x2) {
    	e -> ftsr |= (1 << channel);
    }
    if (channel <= 4) {
    	e -> imr |= (1 << channel);	
	nvic_irq(NVIC_EXTI_BASE + channel, IRQ_ENABLE);
    } else if (5 <= channel && channel <= 9) {
    	e -> imr |= (1 << channel);
	nvic_irq(NVIC_EXTI9_5, IRQ_ENABLE);
    } else {
    	e -> imr |= (1 << channel);
	nvic_irq(NVIC_EXTI15_10, IRQ_ENABLE);
    }
    return;
}

void disable_exti(uint32_t channel) {
    struct exti* e = EXTI_BASE;
    struct syscfg* s = SYSCFG_BASE;

    uint32_t syscfg_exti = channel / 4;

    s -> exti[syscfg_exti] &= ~(0xF << ((channel % 4) * 4));
    e -> rtsr &= ~(1 << channel);
    e -> ftsr &= ~(1 << channel);
    e -> imr &= ~(1 << channel);
    return;
}

void exti_clear_pending_bit(uint32_t channel) {
    struct exti* e = EXTI_BASE;
    e -> pr |= (1 << channel);
    return;
}

// Left ENCB
void exti0_irq_handler(void) {
    exti_clear_pending_bit(0);
}

// Left ENCA
uint8_t led_on = 0;
void exti1_irq_handler(void) {
    if (led_on) {
        gpio_set(GPIO_A, 4);
	led_on = 0;
    } else {
        gpio_clr(GPIO_A, 4);
	led_on = 1;
    }
    exti_clear_pending_bit(1);
}

void exti2_irq_handler(void) {
    exti_clear_pending_bit(2);
}

void exti3_irq_handler(void) {
    exti_clear_pending_bit(3);
}

void exti4_irq_handler(void) {
    exti_clear_pending_bit(4);
}

// Right ENCA ENCB
void exti9_5_irq_handler(void) {
    for (uint32_t i = 5; i <= 9; i++) {
    	exti_clear_pending_bit(i);
    }
}

void exti15_10_irq_handler(void) {
    for (uint32_t i = 10; i <= 15; i++) {
        exti_clear_pending_bit(i);
    }
}
