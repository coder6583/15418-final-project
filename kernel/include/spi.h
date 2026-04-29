#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

#define LINK_IN 0
#define LINK_OUT 1

#define SPI_PACKET_SIZE 71

void sys_spi_init(uint8_t link_id);

void sys_spi_transmit(uint8_t *tx_data, uint32_t len);

void sys_spi_receive(uint8_t *rx_data, uint32_t len);

int sys_spi_rx_ready(void);

int sys_spi_rx_dequeue(uint8_t *buf, uint32_t len);

#endif /* _SPI_H_ */
