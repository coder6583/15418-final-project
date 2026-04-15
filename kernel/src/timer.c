/**
 * @file timer.c
 *
 * @brief Initializes general purpose timer 2-5
 *    Helper functions to change timer registers
 *
 * @date 11/01/2025
 *
 * @author Soma Narita
 */

#include <unistd.h>
#include <stdint.h>
#include <timer.h>
#include <rcc.h>
#include <nvic.h>

#define UNUSED __attribute__((unused))

/** @brief tim2_5 */
struct tim2_5 {
  volatile uint32_t cr1; /**< 00 Control Register 1 */
  volatile uint32_t cr2; /**< 04 Control Register 2 */
  volatile uint32_t smcr; /**< 08 Slave Mode Control */
  volatile uint32_t dier; /**< 0C DMA/Interrupt Enable */
  volatile uint32_t sr; /**< 10 Status Register */
  volatile uint32_t egr; /**< 14 Event Generation */
  volatile uint32_t ccmr[2]; /**< 18-1C Capture/Compare Mode */
  volatile uint32_t ccer; /**< 20 Capture/Compare Enable */
  volatile uint32_t cnt; /**< 24 Counter Register */
  volatile uint32_t psc; /**< 28 Prescaler Register */
  volatile uint32_t arr; /**< 2C Auto-Reload Register */
  volatile uint32_t reserved_1; /**< 30 */
  volatile uint32_t ccr[4]; /**< 34-40 Capture/Compare */
  volatile uint32_t reserved_2; /**< 44 */
  volatile uint32_t dcr; /**< 48 DMA Control Register */
  volatile uint32_t dmar; /**< 4C DMA address for full transfer Register */
  volatile uint32_t or; /**< 50 Option Register */
};


// Constants to set registers for each timer
struct tim2_5* const timer_base[] = {(void *)0x0,   // N/A - Don't fill out
                                     (void *)0x0,   // N/A - Don't fill out
                                     (void *)0x40000000,    // TODO: fill out address for TIMER 2
                                     (void *)0x40000400,    // TODO: fill out address for TIMER 3
                                     (void *)0x40000800,    // TODO: fill out address for TIMER 4
                                     (void *)0x40000C00};   // TODO: fill out address for TIMER 5

static uint32_t const timer_en[] = {0, 0, 1, (1 << 1), (1 << 2), (1 << 3)};
static uint8_t const timer_nvic[] = {0, 0, 28, 29, 30, 50};

#define TIM2_EN (1 << 0)
#define TIM3_EN (1 << 1)
#define TIM4_EN (1 << 2)
#define TIM5_EN (1 << 3)

#define TIM_UIF (1 << 0)
#define TIM_UG (1 << 0)
#define TIM_UIE (1 << 0)
#define TIM_CNT_EN (1 << 0)

#define TIM2_NVIC 28
#define TIM3_NVIC 29
#define TIM4_NVIC 30
#define TIM5_NVIC 50

// Initialize timer with the set prescalar and period
void timer_init(int timer, uint32_t prescalar, uint32_t period) {
    struct tim2_5 *t = timer_base[timer];
    struct rcc_reg_map *rcc = RCC_BASE;

    rcc -> apb1_enr |= timer_en[timer];

    // prescalar is 32 bits while t -> psc is 16 bit register?
    t -> psc = prescalar - 1;
    t -> arr = period - 1;

    // resets the counters
    t -> egr |= TIM_UG;

    // enables update interrupts
    t -> dier |= TIM_UIE;

    // enable counter
    t -> cr1 |= TIM_CNT_EN;

    nvic_irq(timer_nvic[timer], IRQ_ENABLE);
    return;
}

// Disable RCC clock for timer
void timer_disable(int timer) {
    struct rcc_reg_map *rcc = RCC_BASE;
    rcc -> apb1_enr &= ~(timer_en[timer]);
    return;
}

// Clear interrupt, should be called in every interrupt
void timer_clear_interrupt_bit(int timer) {
    struct tim2_5 *t = timer_base[timer];
    t -> sr &= ~(TIM_UIF);
    return;
}

// Set ARR, this changes the period of when the interrupts fire
void timer_set_arr(int timer, uint32_t arr) {
    struct tim2_5 *t = timer_base[timer];
    t -> arr = arr;
    return;
}

// Interrupt handler for timer 2
void tim2_irq_handler(void ) {
    timer_clear_interrupt_bit(2);
}

// Interrupt handler for timer 3
void tim3_irq_handler(void ) {
    timer_clear_interrupt_bit(3);
}

// Interrupt handler for timer4
void tim4_irq_handler(void ) {
    timer_clear_interrupt_bit(4);
}

// Interrupt handler for timer5
void tim5_irq_handler(void ) {
    timer_clear_interrupt_bit(5);
}
