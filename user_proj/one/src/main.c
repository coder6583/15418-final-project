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


void thread_entry() {
  tag_t t = 16;

  char *msg = "nayeon pop pop! 1 2 3 4 5 6 7, you make me feel like eleven! baby, i'm just trying to play it cool";
  uint16_t len = strlen(msg);
  tinimpi_send(NODE_TWO, t, (uint8_t*)msg, len);
  printf("haha! i will be entering sleep for 5 seconds!\n");
  sleep(5000);
  printf("now, i enter barrier!\n");
  tinimpi_barrier();
  printf("Exiting barrier!\n");
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  net_init(NODE_ONE);
  thread_init(1, 256, NULL, 0);
  thread_create(&thread_entry, 0, 1, 1, NULL);
  scheduler_start(1000); // just the default thread;
  while(1);
}
