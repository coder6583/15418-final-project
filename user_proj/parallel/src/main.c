#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <network.h>
#include <string.h>
#include <tinimpi.h>
#include <stdio.h>

int main(UNUSED int argc, UNUSED char const *argv[]) {
  rank_t one = 1;
  rank_t two = 2;
  (void) one, two;
  net_init(one);
  // net_init(two);


  // transmit
  tag_t t = 16;
  char *msg = "nayeon pop pop!";
  uint16_t len = strlen(msg);
  while (1) {
    tinimpi_send(two, t, (uint8_t*)msg, len);
    for (int i = 0; i < 100000; i++);
  }

  // receive
  char buf[256];
  uint16_t out_len;
  tinimpi_recv(two, t, (uint8_t *)buf, 256, &out_len);
  printf("message: %s\n", buf);
  while (1);
}
