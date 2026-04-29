/**
 * @file uart.c
 *
 * @brief uart  
 *
 * @date 11/1/2025      
 *
 * @author Josef Macera <jmacera@andrew.cmu.edu>
 */

#include <stdint.h>
#include <unistd.h>
#include <rcc.h>
#include <uart.h>
#include <gpio.h>
#include <uart_polling.h>
#include <nvic.h>
#include <buf.h>
#include <arm.h>
#define UNUSED __attribute__((unused))



/** @brief The UART register map. */
struct uart_reg_map {
    volatile uint32_t SR;   /**< Status Register */
    volatile uint32_t DR;   /**<  Data Register */
    volatile uint32_t BRR;  /**<  Baud Rate Register */
    volatile uint32_t CR1;  /**<  Control Register 1 */
    volatile uint32_t CR2;  /**<  Control Register 2 */
    volatile uint32_t CR3;  /**<  Control Register 3 */
    volatile uint32_t GTPR; /**<  Guard Time and Prescaler Register */
};

/** @brief Base address for UART2 */
#define UART2_BASE  (struct uart_reg_map *) 0x40004400

/** @brief Enable  Bits for UART Config register */
#define UART_EN (1 << 13)
#define UART_TX_EN (1 << 3)
#define UART_RX_EN (1 << 2)

/** @brief Enable Bit for UART Clock */
#define UART_CLOCK_EN (1 << 17)

/** @brief UART Peripheral Clock speed */
#define UART_CLOCK_SPEED 16000000 // 16MHz 

/** @brief UART Status Register bits */
#define UART_STATUS_DATA_EMPTY 7
#define UART_STATUS_DATA_READY 5

#define UART_TXIE_EN (1 << 7)
#define UART_RXIE_EN (1 << 5)

#define UART_NVIC 38 

// init uart
void uart_init(int baud){
    nvic_irq(UART_NVIC, IRQ_ENABLE);
    struct rcc_reg_map *rcc = RCC_BASE;
    struct uart_reg_map *uart = UART2_BASE;

    rcc->apb1_enr |= UART_CLOCK_EN;

    gpio_init(GPIO_A, 2, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7); // TX
    gpio_init(GPIO_A, 3, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT7); // RX
    
    uart->BRR = baud;

    uart->CR1 |= UART_TX_EN;
    uart->CR1 |= UART_RX_EN;
    uart->CR1 |= UART_EN; // enable uart2

    uart->CR1 |= UART_RXIE_EN;
    uart->CR1 &= ~UART_TXIE_EN;
    return;
}

// put a byte onto the uart buffer
int uart_put_byte(char c){
    struct uart_reg_map *uart = UART2_BASE;
    int state = save_interrupt_state_and_disable();
    char tx_ready = !txbuf_full();
    if (tx_ready) {
        txbuf_add(c);
        uart->CR1 |= UART_TXIE_EN;
        restore_interrupt_state(state);
        return 0;
    } else {
        uart->CR1 |= UART_TXIE_EN;
        restore_interrupt_state(state);
        return -1;
    }
}

// get a byte from the uart buffer
int uart_get_byte(char *c){
    struct uart_reg_map *uart = UART2_BASE;
    uart->CR1 &= ~UART_RXIE_EN; // disable txie
    if (rxbuf_empty()) {
        uart->CR1 |= UART_RXIE_EN;
        return -1;
    }
    *c = rxbuf_rem();
    uart->CR1 |= UART_RXIE_EN;
    return 0;	
}

// tx handler when uart sends an interrupt`
void uart_irq_tx_handler() {
    struct uart_reg_map *uart = UART2_BASE;
    int state = save_interrupt_state_and_disable();
    char c = txbuf_rem();
    uart->DR = c;
    char empty = txbuf_empty();

    restore_interrupt_state(state);
    if (!empty) {
        uart->CR1 |= UART_TXIE_EN; // enable txie
    } else {
        uart->CR1 &= ~UART_TXIE_EN; // enable txie
    }
}

// rx handler when uart sends an interrupt
void uart_irq_rx_handler() {
    struct uart_reg_map *uart = UART2_BASE;
    uart->CR1 &= ~UART_RXIE_EN; // disable txie
    char c = uart->DR;
    rxbuf_add(c);
    if (!rxbuf_full())
        uart->CR1 |= UART_RXIE_EN; // turn it on
    else
        uart->CR1 &= ~(UART_RXIE_EN); // turn it off
}

// flush uart
void uart_flush(){
    struct uart_reg_map *uart = UART2_BASE;
    int state = save_interrupt_state_and_disable();
    while (!txbuf_empty()) {
    	while(((uart->SR >> UART_STATUS_DATA_EMPTY) & 1) == 0);
        char c = txbuf_rem();
        uart->DR = c;
    }
    restore_interrupt_state(state);
}
// uart irq handler
void uart_irq_handler(){
    struct uart_reg_map *uart = UART2_BASE;

    int state = save_interrupt_state_and_disable();
//    char tx_ready = !txbuf_empty();
    restore_interrupt_state(state);

    if (((uart->SR >> UART_STATUS_DATA_EMPTY) & 1))
        uart_irq_tx_handler();

    if (((uart->SR >> UART_STATUS_DATA_READY) & 1) && !rxbuf_full())
        uart_irq_rx_handler();
    nvic_clear_pending(UART_NVIC);

}

