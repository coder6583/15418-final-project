#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

#define LINK_IN 0
#define LINK_OUT 1

// enable SPI slave before the master sends the clock

// @param link_id - specifies if the SPI module is input or output
void sys_spi_init(uint8_t link_id);

void sys_spi_transmit(uint8_t *tx_data, uint32_t len);

void sys_spi_receive(uint8_t *rx_data, uint32_t len);

#endif /* _SPI_H_ */
