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

  
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  net_init(NODE_ONE);
  thread_init(1, 256, NULL, 0);
  thread_create(&thread_entry, 0, 1, 1, NULL);
  scheduler_start(1000); // just the default thread;
  while(1);
}
