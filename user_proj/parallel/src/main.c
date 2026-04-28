#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <sleep.h>
#include <network.h>
#include <string.h>
#include <tinimpi.h>
#include <stdio.h>

void thread_entry() {
  rank_t one = 1;
  rank_t two = 2;
  rank_t rank = one;
  tag_t t = 16;

  if (rank == one) {
    char *msg = "nayeon pop pop!";
    uint16_t len = strlen(msg);
    tinimpi_send(two, t, (uint8_t*)msg, len);    
  } else if (rank == two) {
    char buf[256];
    uint16_t out_len;
    tinimpi_recv(one, t, (uint8_t*) buf, 256, &out_len);
    printf("Recieved message over tiniMPI: %s\n", buf); 
  }
  while (1);
}
int main(UNUSED int argc, UNUSED char const *argv[]) {
   thread_init(1, 256, NULL, 0);
   thread_create(&thread_entry, 0, 1, 1, NULL);
   scheduler_start(1000); // just the default thread;
   while(1);
}
