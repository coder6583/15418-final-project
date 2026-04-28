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

const rank_t rank = one;

void thread_entry() {
  tag_t t = 16;

  char *msg = "nayeon pop pop!";
  char buf[256];
// tinimpi_send(two, t, (uint8_t*)msg, len);    
  for (int i = 0; i < 10; i++) {
    snprintf(buf, 256, "%s part %d!", msg, i);
    uint16_t len = strlen(buf);
    tinimpi_send(two, t, (uint8_t*)buf, len);
    sleep(1000);
  }
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
   net_init(rank);
   thread_init(1, 256, NULL, 0);
   thread_create(&thread_entry, 0, 1, 1, NULL);
   scheduler_start(1000); // just the default thread;
   while(1);
}
