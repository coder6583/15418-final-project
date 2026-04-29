#include <stdint.h>
#include <spi.h>
#include <rcc.h>
#include <gpio.h>
#include <nvic.h>
#include <arm.h>

/** @brief The SPI register map. */
struct spi_reg_map {
    volatile uint32_t CR1;     /**< Control Register 1 */
    volatile uint32_t CR2;     /**< Control Register 2 */
    volatile uint32_t SR;      /**< Status Register */
    volatile uint32_t DR;      /**< Data Register */
    volatile uint32_t CRCPR;   /**< CRC Polynomial Register */
    volatile uint32_t RXCRCR;  /**< SPI RX CRC Register */
    volatile uint32_t TXCRCR;  /**< SPI TX CRC Register */
    volatile uint32_t I2SCFGR; /**< I2S Configuration Register */
    volatile uint32_t I2SPR;   /**< I2S Prescaler Register */
};

#define SPI1_BASE (struct spi_reg_map *) 0x40013000
#define SPI2_BASE (struct spi_reg_map *) 0x40003800
#define SPI3_BASE (struct spi_reg_map *) 0x40003C00
#define SPI4_BASE (struct spi_reg_map *) 0x40013400

#define SPI_MASTER (1 << 2) // CR1
#define SPI_SLAVE  (0 << 2) // CR1

#define SPI_BR_DIV2   (0 << 3) // CR1
#define SPI_BR_DIV4   (1 << 3) // CR1
#define SPI_BR_DIV8   (2 << 3) // CR1
#define SPI_BR_DIV16  (3 << 3) // CR1
#define SPI_BR_DIV32  (4 << 3) // CR1
#define SPI_BR_DIV64  (5 << 3) // CR1
#define SPI_BR_DIV128 (6 << 3) // CR1
#define SPI_BR_DIV256 (7 << 3) // CR1

#define SPI_CPOL_LOW  (0 << 1) // CR1
#define SPI_CPOL_HIGH (1 << 1) // CR1
#define SPI_CPHA_FIRST  (0)    // CR1
#define SPI_CPHA_SECOND (1)    // CR1

#define SPI_DFF_8BIT  (0 << 11) // CR1
#define SPI_DFF_16BIT (1 << 11) // CR1

#define SPI_LSBFIRST (1 << 7) // CR1
#define SPI_MSBFIRST (0 << 7) // CR1

#define SPI_SSM (1 << 9) // CR1
#define SPI_SSI (1 << 8) // CR1

#define SPI_RXONLY    (1 << 10) // CR1
#define SPI_TXONLY    (0 << 10) // CR1
#define SPI_BIDIMODE_UNI (1 << 15) // CR1

#define SPI_FRF (0 << 4) // CR2

#define SPI_CR2_RXNEIE (1 << 6) // CR2
#define SPI_CR2_TXEIE  (1 << 7) // CR2

#define SPI_EN (1 << 6) // CR1

#define RCC_SPI1_CLOCK_EN (1 << 12) // APB2_ENR
#define RCC_SPI2_CLOCK_EN (1 << 14) // APB1_ENR
#define RCC_SPI3_CLOCK_EN (1 << 15) // APB1_ENR
#define RCC_SPI4_CLOCK_EN (1 << 13) // APB2_ENR

#define SPI_SR_TXE  (1 << 1)
#define SPI_SR_RXNE (1 << 0)
#define SPI_SR_BSY  (1 << 7)

#define SPI2_IRQ 36
#define SPI3_IRQ 51

/* ── ring buffers ─────────────────────────────────────────────────────────── */

#define SPI_BUFSIZE 512

static uint8_t spi_txbuf[SPI_BUFSIZE];
static uint8_t spi_rxbuf[SPI_BUFSIZE];

static volatile uint32_t spi_tx_head = 0, spi_tx_tail = 0;
static volatile uint32_t spi_rx_head = 0, spi_rx_tail = 0;

static volatile uint32_t spi_rx_remaining = 0;
static volatile uint8_t  spi_tx_done = 1;
static volatile uint8_t  spi_rx_done = 1;

/* ── init ─────────────────────────────────────────────────────────────────── */

void sys_spi_init(uint8_t link_id) {
  struct rcc_reg_map *rcc = RCC_BASE;

  if (link_id == LINK_IN) {
    // SPI2 slave (receive side)
    struct spi_reg_map *spi = SPI2_BASE;
    rcc->apb1_enr |= RCC_SPI2_CLOCK_EN;

    gpio_init(GPIO_B, 10, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // SCK
    gpio_init(GPIO_B, 15, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // MOSI
    gpio_init(GPIO_B, 14, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // MISO
    gpio_init(GPIO_B, 12, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // NSS
    gpio_init(GPIO_B, 13, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT0); // READY
    gpio_set(GPIO_B, 13); // deassert READY

    spi->CR1 |= SPI_DFF_8BIT;
    spi->CR1 |= SPI_CPOL_LOW;
    spi->CR1 |= SPI_CPHA_FIRST;
    spi->CR1 |= SPI_MSBFIRST;
    spi->CR1 &= ~SPI_SSM;   // hardware NSS
    spi->CR2 |= SPI_FRF;
    spi->CR1 |= SPI_SLAVE;
    spi->CR1 |= SPI_BR_DIV256;

    spi->CR1 |= SPI_EN;

    // flush any stale data before enabling the interrupt
    while (spi->SR & SPI_SR_RXNE) {
        volatile uint8_t dummy = *((volatile uint8_t *)&spi->DR);
        (void)dummy;
    }

    spi->CR2 |= SPI_CR2_RXNEIE;
    nvic_irq(SPI2_IRQ, IRQ_ENABLE);

  } else {
    // SPI3 master (transmit side)
    struct spi_reg_map *spi = SPI3_BASE;
    rcc->apb1_enr |= RCC_SPI3_CLOCK_EN;

    gpio_init(GPIO_B, 3, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6); // SCK
    gpio_init(GPIO_B, 5, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6); // MOSI
    gpio_init(GPIO_B, 4, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6); // MISO
    gpio_init(GPIO_A, 10, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT0); // NSS
    gpio_set(GPIO_A, 10); // deassert NSS
    gpio_init(GPIO_C, 7, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT0); // READY in

    spi->CR1 |= SPI_DFF_8BIT;
    spi->CR1 |= SPI_BR_DIV256;
    spi->CR1 |= SPI_CPOL_LOW;
    spi->CR1 |= SPI_CPHA_FIRST;
    spi->CR1 |= SPI_MSBFIRST;
    spi->CR1 |= SPI_SSM;    // software NSS
    spi->CR1 |= SPI_SSI;
    spi->CR2 |= SPI_FRF;
    spi->CR1 |= SPI_MASTER;

    // TXEIE is left off until sys_spi_transmit is called
    nvic_irq(SPI3_IRQ, IRQ_ENABLE);

    spi->CR1 |= SPI_EN;
  }
}

/* ── ISR: SPI2 receive ────────────────────────────────────────────────────── */

void spi2_irq_handler(void) {
  struct spi_reg_map *spi = SPI2_BASE;

  if (spi->SR & SPI_SR_RXNE) {
    uint8_t byte = *((volatile uint8_t *)&spi->DR);

    uint32_t next = (spi_rx_head + 1) % SPI_BUFSIZE;
    if (next != spi_rx_tail) {
      spi_rxbuf[spi_rx_head] = byte;
      spi_rx_head = next;
    }

    if (spi_rx_remaining > 0 && --spi_rx_remaining == 0)
      spi_rx_done = 1;
  }

  nvic_clear_pending(SPI2_IRQ);
}

/* ── ISR: SPI3 transmit ───────────────────────────────────────────────────── */

void spi3_irq_handler(void) {
  struct spi_reg_map *spi = SPI3_BASE;

  if (spi->SR & SPI_SR_TXE) {
    if (spi_tx_tail != spi_tx_head) {
      *((volatile uint8_t *)&spi->DR) = spi_txbuf[spi_tx_tail];
      spi_tx_tail = (spi_tx_tail + 1) % SPI_BUFSIZE;
    } else {
      // buffer drained — disable TXEIE and signal caller
      spi->CR2 &= ~SPI_CR2_TXEIE;
      spi_tx_done = 1;
    }
  }

  nvic_clear_pending(SPI3_IRQ);
}

/* ── public API ───────────────────────────────────────────────────────────── */

void sys_spi_transmit(uint8_t *tx_data, uint32_t len) {
  struct spi_reg_map *spi = SPI3_BASE;

  // load tx ring buffer with interrupts off to avoid a partial-load race
  int state = save_interrupt_state_and_disable();
  spi_tx_head = 0;
  spi_tx_tail = 0;
  uint32_t count = len < SPI_BUFSIZE ? len : SPI_BUFSIZE;
  for (uint32_t i = 0; i < count; i++)
    spi_txbuf[i] = tx_data[i];
  spi_tx_head = count;
  spi_tx_done = 0;
  restore_interrupt_state(state);

  // assert CS, then wait for slave READY
  gpio_clr(GPIO_A, 10);
  while (gpio_read(GPIO_C, 7) == 1);

  // enabling TXEIE fires the first TXE interrupt immediately (TXE is already set)
  spi->CR2 |= SPI_CR2_TXEIE;

  // wait for ISR to drain the buffer (spi_tx_done set when last byte written to DR)
  while (!spi_tx_done);

  // wait for the last byte to finish shifting out
  while (spi->SR & SPI_SR_BSY);

  gpio_set(GPIO_A, 10);
}

void sys_spi_receive(uint8_t *rx_data, uint32_t len) {
  struct spi_reg_map *spi = SPI2_BASE;

  int state = save_interrupt_state_and_disable();
  // drain any byte sitting in hardware DR into the ring buffer — if the ISR
  // hadn't run yet for this byte, discarding it would make 'already' too low
  // and spi_rx_remaining would be set 1 too high, causing a permanent stall
  while (spi->SR & SPI_SR_RXNE) {
    uint8_t b = *((volatile uint8_t *)&spi->DR);
    uint32_t next = (spi_rx_head + 1) % SPI_BUFSIZE;
    if (next != spi_rx_tail) {
      spi_rxbuf[spi_rx_head] = b;
      spi_rx_head = next;
    }
  }
  uint32_t already = (spi_rx_head - spi_rx_tail) % SPI_BUFSIZE;
  spi_rx_remaining = (already >= len) ? 0 : len - already;
  spi_rx_done = (spi_rx_remaining == 0) ? 1 : 0;
  restore_interrupt_state(state);

  // signal master we are ready
  gpio_clr(GPIO_B, 13);
  // wait for master to assert NSS
  while (gpio_read(GPIO_B, 12) == 1);

  // ISR fills spi_rxbuf as bytes arrive; wait for all expected bytes
  while (!spi_rx_done);

  // wait for master to deassert NSS before releasing READY
  while (gpio_read(GPIO_B, 12) == 0);
  gpio_set(GPIO_B, 13);

  // copy from ring buffer to caller's buffer
  state = save_interrupt_state_and_disable();
  for (uint32_t i = 0; i < len; i++) {
    rx_data[i] = spi_rxbuf[spi_rx_tail];
    spi_rx_tail = (spi_rx_tail + 1) % SPI_BUFSIZE;
  }
  restore_interrupt_state(state);
}
