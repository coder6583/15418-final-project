#ifndef _EXTI_H_
#define _EXTI_H_

#include <gpio.h>
#include <unistd.h>

#define RISING_EDGE         1
#define FALLING_EDGE        2
#define RISING_FALLING_EDGE 3
#define EXTI_BASE (struct exti *)0x40013C00

/** @brief EXTI register map. */
struct exti
{
    volatile uint32_t imr;   /**< 00 Interrupt Mask Register */
    volatile uint32_t emr;   /**< 04 Event Mask Register */
    volatile uint32_t rtsr;  /**< 08 Rising trigger Selection */
    volatile uint32_t ftsr;  /**< 0C Falling trigger Selection */
    volatile uint32_t swier; /**< 10 Software Interrupt Event Register */
    volatile uint32_t pr;    /**< 14 Pending Register */
};

/**
 * @brief Enable an external interrupt
 *
 * @param sel     - The GPIO port for the external interrupt
 * @param channel - The GPIO channel for the external interrupt
 * @param edge    - The ege triggering the interrupt. Must be one of
 * RISING_EDGE, FALLING_EDGE, RISING_FALLING_EDGE
 */
void enable_exti(gpio_port port, uint32_t channel, uint32_t edge);

/**
 * @brief Disable the exti
 *
 * @param channel - The channel to disable
 */
void disable_exti(uint32_t channel);

/**
 * @brief Clear the pending bit for the exti
 *
 * @param channel - The channel of interest
 */
void exti_clear_pending_bit(uint32_t channel);

#endif /* _EXTI_H_ */
