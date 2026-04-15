#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>

#define LINK_IN 0
#define LINK_OUT 1

void spi_init(uint8_t link_id);

void spi_transmit(uint8_t *tx_data, uint32_t len);

void spi_receive(uint8_t *rx_data, uint32_t len);

#endif 