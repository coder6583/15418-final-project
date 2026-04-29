#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <sleep.h>
#include <network.h>
#include <string.h>
#include <tinimpi.h>
#include <stdio.h>
#include <stdint.h>
#include <topology.h>

void mpi_thread() {
  while(1) {
    if (spi_rx_ready()) {
      printf("received smth\n");
      uint8_t buf[72];
      spi_rx_dequeue(buf, 71);
      packet_t p = build_packet(buf);
      print_packet(p);
    }
    wait_until_next_period();
  }
}

void computation() {
  tag_t t = 16;

  sleep(5000);

  char *msg = "nayeon pop pop! 1 2 3 4 5 6 7, you make me feel like eleven! baby, i'm just trying to play it cool";
  uint16_t len = strlen(msg);
  tinimpi_send(NODE_TWO, t, (uint8_t*)msg, len);
  
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  net_init(NODE_ONE);
  thread_init(1, 256, &computation, 0);
  thread_create(&mpi_thread, 0, 1, 1, NULL);
  scheduler_start(1000); // just the default thread;
  while(1);
}
