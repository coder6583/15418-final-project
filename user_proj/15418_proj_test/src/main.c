#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <stdio.h>

int main(UNUSED int argc, UNUSED char const *argv[]) {
  spi_init(LINK_IN);
  spi_init(LINK_OUT);


  // transmission
  char *s = "hello";

  while (1) {
    spi_transmit((uint8_t*)s, 5);
    for (int i = 0; i < 10000; i++);
  }

  // receive
  // char recv[6];
  // recv[5] = '\0';

  // while (1) {
  //   spi_receive((uint8_t*)recv, 5);
  //   printf("received: %s\n", recv);
  // }
}