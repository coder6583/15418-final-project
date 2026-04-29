#include <stdint.h>
#include <spi.h>
#include <rcc.h>
#include <gpio.h>
#include <nvic.h>
#include <arm.h>
#include <printk.h>

#define BULLSHIT for(int i = 0; i < 100000; i++);

/* ── SPI register map ────────────────────────────────────────────────────── */

struct spi_reg_map {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
};

#define SPI1_BASE ((struct spi_reg_map *) 0x40013000)
#define SPI2_BASE ((struct spi_reg_map *) 0x40003800)
#define SPI3_BASE ((struct spi_reg_map *) 0x40003C00)

#define SPI_MASTER       (1 << 2)
#define SPI_SLAVE        (0 << 2)

#define SPI_BR_DIV2      (0 << 3)
#define SPI_BR_DIV4      (1 << 3)
#define SPI_BR_DIV8      (2 << 3)
#define SPI_BR_DIV16     (3 << 3)
#define SPI_BR_DIV32     (4 << 3)
#define SPI_BR_DIV64     (5 << 3)
#define SPI_BR_DIV128    (6 << 3)
#define SPI_BR_DIV256    (7 << 3)

#define SPI_CPOL_LOW     (0 << 1)
#define SPI_CPHA_FIRST   (0)
#define SPI_DFF_8BIT     (0 << 11)
#define SPI_MSBFIRST     (0 << 7)
#define SPI_SSM          (1 << 9)
#define SPI_SSI          (1 << 8)
#define SPI_FRF          (0 << 4)
#define SPI_EN           (1 << 6)
#define SPI_CR2_RXDMAEN  (1 << 0)
#define SPI_CR2_TXDMAEN  (1 << 1)

#define RCC_SPI2_CLOCK_EN (1 << 14)
#define RCC_SPI3_CLOCK_EN (1 << 15)

#define SPI_SR_TXE  (1 << 1)
#define SPI_SR_RXNE (1 << 0)
#define SPI_SR_BSY  (1 << 7)

/* ── DMA register map ────────────────────────────────────────────────────── */

struct dma_reg_map {
    volatile uint32_t LISR;   /* 0x00 low interrupt status  */
    volatile uint32_t HISR;   /* 0x04 high interrupt status */
    volatile uint32_t LIFCR;  /* 0x08 low interrupt flag clear  */
    volatile uint32_t HIFCR;  /* 0x0C high interrupt flag clear */
};

struct dma_stream_reg_map {
    volatile uint32_t CR;    /* 0x00 */
    volatile uint32_t NDTR;  /* 0x04 */
    volatile uint32_t PAR;   /* 0x08 */
    volatile uint32_t M0AR;  /* 0x0C */
    volatile uint32_t M1AR;  /* 0x10 */
    volatile uint32_t FCR;   /* 0x14 */
};

#define DMA1_BASE    ((struct dma_reg_map *)    0x40026000)
/* Stream3 base = DMA1 base + 0x58 */
#define DMA1_S3_BASE ((struct dma_stream_reg_map *) 0x40026058)
/* Stream5 base = DMA1 base + 0x88 */
#define DMA1_S5_BASE ((struct dma_stream_reg_map *) 0x40026088)

/* SxCR bits */
#define DMA_CR_EN        (1 << 0)
#define DMA_CR_TCIE      (1 << 4)
#define DMA_CR_DIR_P2M   (0 << 6)
#define DMA_CR_DIR_M2P   (1 << 6)
#define DMA_CR_CIRC      (1 << 8)
#define DMA_CR_MINC      (1 << 10)
#define DMA_CR_PSIZE_BYTE (0 << 11)
#define DMA_CR_MSIZE_BYTE (0 << 13)
#define DMA_CR_PL_HIGH   (2 << 16)
#define DMA_CR_DBM       (1 << 18)
#define DMA_CR_CT        (1 << 19)
#define DMA_CR_CHSEL0    (0 << 25)

/* LISR/LIFCR Stream3 flags (bits 22–27) */
#define DMA_LISR_TCIF3   (1 << 27)
#define DMA_LIFCR_CTCIF3 (1 << 27)

/* HISR/HIFCR Stream5 flags (bits 6–11) */
#define DMA_HISR_TCIF5   (1 << 11)
#define DMA_HIFCR_CTCIF5 (1 << 11)

#define RCC_AHB1_DMA1_EN (1 << 21)

#define DMA1_STREAM3_IRQ 14

/* ── DMA packet queue ────────────────────────────────────────────────────── */

#define SPI_PKT_QUEUE_DEPTH 4

/* DMA double-buffer targets — written only by hardware */
static uint8_t dma_buf[2][SPI_PACKET_SIZE];

/* Software packet queue — ISR produces, consumer dequeues */
static uint8_t pkt_queue[SPI_PKT_QUEUE_DEPTH][SPI_PACKET_SIZE];
static volatile uint8_t pkt_head = 0;  /* ISR write pointer  */
static volatile uint8_t pkt_tail = 0;  /* consumer read pointer */

/* ── DMA TX packet queue ────────────────────────────────────────────────────── */

#define TXQ_SIZE 8

static volatile uint32_t tx_head = 0;
static volatile uint32_t tx_tail = 0;
static tx_item_t tx_queue[TXQ_SIZE];

static volatile spi_tx_state_t spi_tx_state = SPI_TX_IDLE;

/* ── init ─────────────────────────────────────────────────────────────────── */

void sys_spi_init(uint8_t link_id) {
    struct rcc_reg_map *rcc = RCC_BASE;

    if (link_id == LINK_IN) {
        struct spi_reg_map *spi = SPI2_BASE;
        rcc->apb1_enr |= RCC_SPI2_CLOCK_EN;

        gpio_init(GPIO_B, 10, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // SCK
        gpio_init(GPIO_B, 15, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // MOSI
        gpio_init(GPIO_B, 14, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // MISO
        gpio_init(GPIO_B, 12, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5); // NSS

        spi->CR1 |= SPI_DFF_8BIT;
        spi->CR1 |= SPI_CPOL_LOW;
        spi->CR1 |= SPI_CPHA_FIRST;
        spi->CR1 |= SPI_MSBFIRST;
        spi->CR1 &= ~SPI_SSM;
        spi->CR2 |= SPI_FRF;
        spi->CR1 |= SPI_SLAVE;
        spi->CR1 |= SPI_BR_DIV256;
        spi->CR2 |= SPI_CR2_RXDMAEN;
        spi->CR1 |= SPI_EN;

        /* flush any stale hardware data */
        while (spi->SR & SPI_SR_RXNE) {
            volatile uint8_t dummy = *((volatile uint8_t *)&spi->DR);
            (void)dummy;
        }

        /* ── DMA1 Stream3 (SPI2_RX, channel 0) ── */
        rcc->ahb1_enr |= RCC_AHB1_DMA1_EN;

        struct dma_stream_reg_map *s = DMA1_S3_BASE;

        /* disable stream before configuring */
        s->CR &= ~DMA_CR_EN;
        while (s->CR & DMA_CR_EN);

        /* clear stale flags */
        DMA1_BASE->LIFCR = DMA_LIFCR_CTCIF3;

        s->PAR   = (uint32_t)&spi->DR;       /* source: SPI2 data register  */
        s->M0AR  = (uint32_t)dma_buf[0];     /* destination buffer 0        */
        s->M1AR  = (uint32_t)dma_buf[1];     /* destination buffer 1        */
        s->NDTR  = SPI_PACKET_SIZE;
        s->FCR   = 0;                         /* direct mode (no FIFO)       */

        s->CR = DMA_CR_CHSEL0    /* channel 0 = SPI2_RX */
              | DMA_CR_DBM       /* double-buffer, runs forever */
              | DMA_CR_PL_HIGH
              | DMA_CR_MSIZE_BYTE
              | DMA_CR_PSIZE_BYTE
              | DMA_CR_MINC      /* increment memory pointer */
              | DMA_CR_CIRC      /* required for DBM */
              | DMA_CR_DIR_P2M
              | DMA_CR_TCIE;     /* interrupt on each completed buffer */

        nvic_irq(DMA1_STREAM3_IRQ, IRQ_ENABLE);

        s->CR |= DMA_CR_EN;

    } else {
        struct spi_reg_map *spi = SPI3_BASE;
        rcc->apb1_enr |= RCC_SPI3_CLOCK_EN;

        gpio_init(GPIO_B, 3, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);  // SCK
        gpio_init(GPIO_B, 5, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);  // MOSI
        gpio_init(GPIO_B, 4, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);  // MISO
        gpio_init(GPIO_A, 10, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT0); // NSS
        gpio_set(GPIO_A, 10);
        gpio_init(GPIO_C, 7, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT0); // READY in

        spi->CR1 |= SPI_DFF_8BIT;
        spi->CR1 |= SPI_BR_DIV256;
        spi->CR1 |= SPI_CPOL_LOW;
        spi->CR1 |= SPI_CPHA_FIRST;
        spi->CR1 |= SPI_MSBFIRST;
        spi->CR1 |= SPI_SSM;
        spi->CR1 |= SPI_SSI;
        spi->CR2 |= SPI_FRF;
        spi->CR1 |= SPI_MASTER;
        spi->CR1 |= SPI_EN;

        /* ── DMA1 Stream5 (SPI3_TX, channel 0) ── */
        rcc->ahb1_enr |= RCC_AHB1_DMA1_EN;

        struct dma_stream_reg_map *s = DMA1_S5_BASE;

        s->CR &= ~DMA_CR_EN;
        while (s->CR & DMA_CR_EN);

        DMA1_BASE->HIFCR = DMA_HIFCR_CTCIF5;

        s->PAR  = (uint32_t)&spi->DR;  /* destination: SPI3 data register */
        s->FCR  = 0;                   /* direct mode */

        s->CR = DMA_CR_CHSEL0
              | DMA_CR_PL_HIGH
              | DMA_CR_MSIZE_BYTE
              | DMA_CR_PSIZE_BYTE
              | DMA_CR_MINC
              | DMA_CR_DIR_M2P;        /* memory → peripheral, single shot */
    }
}

/* ── DMA ISR: fires when each 71-byte buffer is complete ─────────────────── */

void dma1_stream3_irq_handler(void) {
  printk("handler called\n");
  struct dma_reg_map *dma = DMA1_BASE;
  struct dma_stream_reg_map *s = DMA1_S3_BASE;

  if (!(dma->LISR & DMA_LISR_TCIF3))
    return;

  dma->LIFCR = DMA_LIFCR_CTCIF3;

  /* CT has already toggled to the next target:
      CT=1 means DMA is now filling M1 → M0 (buf[0]) just completed
      CT=0 means DMA is now filling M0 → M1 (buf[1]) just completed */
  uint8_t completed = ((s->CR & DMA_CR_CT) != 0) ? 0 : 1;

  uint8_t next = (pkt_head + 1) % SPI_PKT_QUEUE_DEPTH;
  if (next != pkt_tail) {
    for (int i = 0; i < SPI_PACKET_SIZE; i++)
      pkt_queue[pkt_head][i] = dma_buf[completed][i];
    pkt_head = next;
  }
  /* if queue is full the packet is silently dropped */

  nvic_clear_pending(DMA1_STREAM3_IRQ);
}

/* ── public API ───────────────────────────────────────────────────────────── */

/* Returns 1 if at least one complete packet is waiting. */
int sys_spi_rx_ready(void) {
  return pkt_head != pkt_tail;
}

/* Copies the oldest queued packet into buf (up to len bytes).
   Returns 0 on success, -1 if no packet is available. */
int sys_spi_rx_dequeue(uint8_t *buf, uint32_t len) {
    int state = save_interrupt_state_and_disable();
    if (pkt_head == pkt_tail) {
        restore_interrupt_state(state);
        return -1;
    }
    uint32_t n = len < SPI_PACKET_SIZE ? len : SPI_PACKET_SIZE;
    for (uint32_t i = 0; i < n; i++)
        buf[i] = pkt_queue[pkt_tail][i];
    pkt_tail = (pkt_tail + 1) % SPI_PKT_QUEUE_DEPTH;
    restore_interrupt_state(state);
    return 0;
}

/* Blocking receive — spins until a packet arrives then dequeues it.
   Kept for backwards compatibility with existing get_packet() callers. */
void sys_spi_receive(uint8_t *rx_data, uint32_t len) {
    while (!sys_spi_rx_ready());
    sys_spi_rx_dequeue(rx_data, len);
}

void sys_spi_transmit(uint8_t *tx_data, uint32_t len) {
  while(sys_spi_tx_queue_full());
  sys_spi_tx_queue_push(tx_data, len);
  // struct spi_reg_map *spi = SPI3_BASE;
  // struct dma_stream_reg_map *s = DMA1_S5_BASE;

  // gpio_clr(GPIO_A, 10);
  // // while (gpio_read(GPIO_C, 7) == 1);
  // BULLSHIT

  // DMA1_BASE->HIFCR = DMA_HIFCR_CTCIF5;
  // s->M0AR = (uint32_t)tx_data;
  // s->NDTR = len;
  // spi->CR2 |= SPI_CR2_TXDMAEN;
  // s->CR |= DMA_CR_EN;

  // while (!(DMA1_BASE->HISR & DMA_HISR_TCIF5));
  // DMA1_BASE->HIFCR = DMA_HIFCR_CTCIF5;

  // s->CR &= ~DMA_CR_EN;
  // spi->CR2 &= ~SPI_CR2_TXDMAEN;

  // while (spi->SR & SPI_SR_BSY);

  // gpio_set(GPIO_A, 10);
}

int sys_spi_tx_queue_full(void) {
  uint32_t next = (tx_head + 1) % TXQ_SIZE;

  return next == tx_tail;
}

int sys_spi_tx_queue_push(uint8_t *data, uint32_t len) {
  if (len != SPI_PACKET_SIZE) {
    return -1;
  }

  uint32_t next = (tx_head + 1) % TXQ_SIZE;

  if (next == tx_tail) {
    return -2; // queue full
  }

  for (uint32_t i = 0; i < len; i++) {
    tx_queue[tx_head].data[i] = data[i];
  }
  tx_queue[tx_head].len = len;

  tx_head = next;
  return 0;
}

static void spi_start_tx_dma(uint8_t *data, uint32_t len) {
  struct spi_reg_map *spi = SPI3_BASE;
  struct dma_stream_reg_map *s = DMA1_S5_BASE;

  gpio_clr(GPIO_A, 10); // CS low

  s->CR &= ~DMA_CR_EN;
  while (s->CR & DMA_CR_EN); // okay, short hardware wait

  DMA1_BASE->HIFCR = DMA_HIFCR_CTCIF5;

  s->M0AR = (uint32_t)data;
  s->NDTR = len;

  spi->CR2 |= SPI_CR2_TXDMAEN;
  s->CR |= DMA_CR_EN;

  spi_tx_state = SPI_TX_DMA_ACTIVE;
}

void sys_spi_progress_tx(void) {
  struct spi_reg_map *spi = SPI3_BASE;
  struct dma_stream_reg_map *s = DMA1_S5_BASE;

  switch (spi_tx_state) {
  case SPI_TX_IDLE:
    if (tx_tail == tx_head) {
      return; // queue empty
    }

    spi_start_tx_dma(tx_queue[tx_tail].data, tx_queue[tx_tail].len);
    return;

  case SPI_TX_DMA_ACTIVE:
    if (!(DMA1_BASE->HISR & DMA_HISR_TCIF5)) {
      return; // DMA still feeding SPI
    }

    DMA1_BASE->HIFCR = DMA_HIFCR_CTCIF5;

    s->CR &= ~DMA_CR_EN;
    spi->CR2 &= ~SPI_CR2_TXDMAEN;

    spi_tx_state = SPI_TX_WAIT_BSY;
    return;

  case SPI_TX_WAIT_BSY:
    if (spi->SR & SPI_SR_BSY) {
      return; // last byte still shifting out
    }

    gpio_set(GPIO_A, 10); // CS high

    tx_tail = (tx_tail + 1) % TXQ_SIZE;
    spi_tx_state = SPI_TX_IDLE;
    return;
  }
}
