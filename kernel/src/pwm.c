#include <printk.h>
#include <pwm.h>
#include <rcc.h>
#include <unistd.h>
#include <gpio.h>

/** @brief TIM1 register map */
struct tim1 {
    volatile uint32_t cr1;     /**< 00 Control Register 1 */
    volatile uint32_t cr2;     /**< 04 Control Register 2 */
    volatile uint32_t smcr;    /**< 08 Slave Mode Control */
    volatile uint32_t dier;    /**<*< 0C DMA/Interrupt Enable */
    volatile uint32_t sr;      /**< 10 Status Register */
    volatile uint32_t egr;     /**< 14 Event Generation */
    volatile uint32_t ccmr[2]; /**< 18-1C Capture/Compare Mode */
    volatile uint32_t ccer;    /**< 20 Capture/Compare Enable */
    volatile uint32_t cnt;     /**< 24 Counter Register */
    volatile uint32_t psc;     /**< 28 Prescaler Register */
    volatile uint32_t arr;     /**< 2C Auto-Reload Register */
    volatile uint32_t rcr;     /**< 30 Repetition Counter Register */
    volatile uint32_t ccr[4];  /**< 34-40 Capture/Compare */
    volatile uint32_t bdtr;    /**< 44 Break and Dead-Time Register */
    volatile uint32_t dcr;     /**< 48 DMA Control Register */
    volatile uint32_t dmar;    /**< 4C DMA address for full transfer Register */
};

/** @brief TIM2-5 register map */
struct tim2_5 {
    volatile uint32_t cr1;        /**< 00 Control Register 1 */
    volatile uint32_t cr2;        /**< 04 Control Register 2 */
    volatile uint32_t smcr;       /**< 08 Slave Mode Control */
    volatile uint32_t dier;       /**< 0C DMA/Interrupt Enable */
    volatile uint32_t sr;         /**< 10 Status Register */
    volatile uint32_t egr;        /**< 14 Event Generation */
    volatile uint32_t ccmr[2];    /**< 18-1C Capture/Compare Mode */
    volatile uint32_t ccer;       /**< 20 Capture/Compare Enable */
    volatile uint32_t cnt;        /**< 24 Counter Register */
    volatile uint32_t psc;        /**< 28 Prescaler Register */
    volatile uint32_t arr;        /**< 2C Auto-Reload Register */
    volatile uint32_t reserved_1; /**< 30 */
    volatile uint32_t ccr[4];     /**< 34-40 Capture/Compare */
    volatile uint32_t reserved_2; /**< 44 */
    volatile uint32_t dcr;        /**< 48 DMA Control Register */
    volatile uint32_t dmar; /**< 4C DMA address for full transfer Register */
    volatile uint32_t or ;  /**< 50 Option Register */
};

uint8_t IS_COMP = 0;

static uint32_t const timer_en[] = {0, 1, 1, (1 << 1), (1 << 2), (1 << 3)};
uint32_t const timer_ccmr[] = {0, 0x0068, 0x6800, 0x0068, 0x6800};
uint32_t const timer_ccxe[] = {0, (1 << 0), (1 << 4), (1 << 8), (1 << 12)};

#define TIM1_BASE (struct tim1 *) 0x40010000
#define TIM1_PSC 15
#define TIM1_UG (1 << 0)
#define TIM1_BDTR_MOE (1 << 15)
#define TIM1_CNT_EN (1 << 0)
#define TIM1_ARPE (1 << 7)

struct tim2_5* const timer_base_pwm[] = {(void *)0x0,   // N/A - Don't fill out
                                     (void *)0x0,   // N/A - Don't fill out
                                     (void *)0x40000000,
                                     (void *)0x40000400,
                                     (void *)0x40000800,
                                     (void *)0x40000C00};

void timer_start_pwm(uint32_t period, uint32_t duty_cycle, uint32_t timer, uint32_t channel) {
    struct rcc_reg_map *rcc = RCC_BASE;

    if (timer == 1) {
    	struct tim1 *t = TIM1_BASE;
	rcc -> apb2_enr = timer_en[timer];
       	t -> psc = TIM1_PSC;
        t -> arr = period - 1;
	t -> egr |= TIM1_UG;

	if (channel > 2) {
	    t -> ccmr[1] |= timer_ccmr[channel];
	} else {
	    t -> ccmr[0] |= timer_ccmr[channel];
	}

	t -> ccer |= timer_ccxe[channel];
	t -> bdtr |= TIM1_BDTR_MOE;
	t -> ccr[channel - 1] = duty_cycle;
	t -> cr1 |= TIM1_ARPE;
	t -> cr1 |= TIM1_CNT_EN;
    } else {
    	struct tim2_5 *t = timer_base_pwm[timer];
	rcc -> apb1_enr |= timer_en[timer];
        t -> psc = TIM1_PSC;
        t -> arr = period - 1;
	t -> egr |= TIM1_UG;

	if (channel > 2) {
	    t -> ccmr[1] |= timer_ccmr[channel];
	} else {
	    t -> ccmr[0] |= timer_ccmr[channel];
	}

	t -> ccer |= timer_ccxe[channel];
	t -> ccr[channel - 1] = duty_cycle;
	t -> cr1 |= TIM1_CNT_EN;
	t -> cr1 |= TIM1_ARPE;
    }

    return;
}

void timer_disable_pwm(uint32_t timer, uint32_t channel) {
    if (timer == 1) {
    	struct tim1 *t = TIM1_BASE;
	t -> ccer &= ~timer_ccxe[channel];
    } else {
    	struct tim2_5 *t = timer_base_pwm[timer];
	t -> ccer &= ~timer_ccxe[channel];
    }
    return;
}

void timer_set_duty_cycle(uint32_t timer, uint32_t channel, uint32_t duty_cycle) {
    if (timer == 1) {
    	struct tim1 *t = TIM1_BASE;
	t -> ccr[channel - 1] = duty_cycle;
    } else {
    	struct tim2_5 *t = timer_base_pwm[timer];
	t -> ccr[channel - 1] = duty_cycle;
    }
    return;
}

