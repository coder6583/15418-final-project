#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

#define LINK_IN 0
#define LINK_OUT 1

// enable SPI slave before the master sends the clock

// @param link_id - specifies if the SPI module is input or output
void sys_spi_init(uint8_t link_id);

#endif /* _SPI_H_ */