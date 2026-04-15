#include <stdint.h>
#include <spi.h>
#include <rcc.h>
#include <gpio.h>

/** @brief The SPI register map. */
struct spi_reg_map {
    volatile uint32_t CR1;   /**< Control Register 1 */
    volatile uint32_t CR2;   /**< Control Register 2 */
    volatile uint32_t SR;   /**<  Status Register */
    volatile uint32_t DR;  /**<  Data Register */
    volatile uint32_t CRCPR;  /**<  CRC Polynomial Register */
    volatile uint32_t RXCRCR;  /**<  SPI RX CRC Register */
    volatile uint32_t TXCRCR;  /**<  SPI TX CRC Register */
    volatile uint32_t I2SCFGR; /**<  I2S Configuration Register */
    volatile uint32_t I2SPR; /**<  I2S Prescaler Register */
};


#define SPI1_BASE (struct spi_reg_map *) 0x40013000
#define SPI2_BASE (struct spi_reg_map *) 0x40003800
#define SPI3_BASE (struct spi_reg_map *) 0x40003C00
#define SPI4_BASE (struct spi_reg_map *) 0x40013400

#define SPI_MASTER (1 << 2) // CR1
#define SPI_SLAVE (0 << 2) // CR1

#define SPI_BR_DIV2 (0 << 3) // CR1
#define SPI_BR_DIV4 (1 << 3) // CR1
#define SPI_BR_DIV8 (2 << 3) // CR1
#define SPI_BR_DIV16 (3 << 3) // CR1
#define SPI_BR_DIV32 (4 << 3) // CR1
#define SPI_BR_DIV64 (5 << 3) // CR1
#define SPI_BR_DIV128 (6 << 3) // CR1
#define SPI_BR_DIV256 (7 << 3) // CR1

#define SPI_CPOL_LOW (0 << 1) // CR1
#define SPI_CPOL_HIGH (0 << 1) // CR1
#define SPI_CPHA_FIRST (0) // CR1
#define SPI_CPHA_SECOND (1) // CR1

#define SPI_DFF_8BIT (0 << 11) // CR1
#define SPI_DFF_16BIT (1 << 11) // CR1

#define SPI_LSBFIRST (1 << 7) // CR1
#define SPI_MSBFIRST (0 << 7) // CR1

#define SPI_SSM (1 << 9) // CR1
#define SPI_SSI (1 << 8) // CR1

#define SPI_FRF (0 << 4) // CR2

#define SPI_EN (1 << 6) // CR1

#define RCC_SPI1_CLOCK_EN (1 << 12) // APB2_ENR
#define RCC_SPI2_CLOCK_EN (1 << 14) // APB1_ENR
#define RCC_SPI3_CLOCK_EN (1 << 15) // APB1_ENR
#define RCC_SPI4_CLOCK_EN (1 << 13) // APB2_ENR

void sys_spi_init(uint8_t link_id) {
  struct rcc_reg_map *rcc = RCC_BASE;
  
  if (link_id == LINK_IN) {
    // INPUT: SPI2, should be slave
    struct spi_reg_map *spi = SPI2_BASE;
    rcc -> apb1_enr |= RCC_SPI2_CLOCK_EN;

    // initialize SPI2_SCK line
    gpio_init(GPIO_B, 10, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5);
    // initialize SPI2_MOSI line
    gpio_init(GPIO_B, 15, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5);
    // initialize SPI2_MISO line
    gpio_init(GPIO_B, 14, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT5);

    // Set the data frame format
    spi -> CR1 |= SPI_DFF_8BIT;

    // CPOL = 0, CPHA = 0
    spi -> CR1 |= SPI_CPOL_LOW;
    spi -> CR1 |= SPI_CPHA_FIRST;

    // Set MSB first
    spi -> CR1 |= SPI_MSBFIRST;

    // Set software chip select, always HIGH
    spi -> CR1 |= SPI_SSM;
    spi -> CR1 |= SPI_SSI;

    // Select motorola mode
    spi -> CR2 |= SPI_FRF;

    // Set slave mode
    spi -> CR1 |= SPI_SLAVE;

    // Enable SPI
    spi -> CR1 |= SPI_EN;
  } else {
    // OUTPUT: SPI3, should be master
    struct spi_reg_map *spi = SPI3_BASE;
    rcc -> apb1_enr |= RCC_SPI3_CLOCK_EN;

    // initialize SPI3_SCK line
    gpio_init(GPIO_B, 3, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);
    // initialize SPI3_MOSI line
    gpio_init(GPIO_B, 5, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);
    // initialize SPI3_MISO line
    gpio_init(GPIO_B, 4, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT6);

    // Set the data frame format
    spi -> CR1 |= SPI_DFF_8BIT;

    // CPOL = 0, CPHA = 0
    spi -> CR1 |= SPI_CPOL_LOW;
    spi -> CR1 |= SPI_CPHA_FIRST;

    // Set MSB first
    spi -> CR1 |= SPI_MSBFIRST;

    // Set software chip select, always HIGH
    spi -> CR1 |= SPI_SSM;
    spi -> CR1 |= SPI_SSI;

    // Select motorola mode
    spi -> CR2 |= SPI_FRF;

    // Set master mode
    spi -> CR1 |= SPI_MASTER;

    // Enable SPI
    spi -> CR1 |= SPI_EN;
  }
}