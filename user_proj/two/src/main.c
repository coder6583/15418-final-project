#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <sleep.h>
#include <network.h>
#include <string.h>
#include <tinimpi.h>
#include <stdio.h>
#include <topology.h>

const rank_t rank = two;
 
void thread_entry() {
 tag_t t = 16;

  char buf[256];
  uint16_t out_len;
// tinimpi_send(two, t, (uint8_t*)msg, len);    
  while (1) {
    tinimpi_recv(one, t, (uint8_t*) buf, 256, &out_len);
    printf("Recieved message over tiniMPI: %s\n", buf); 
  }
    
//  packet_t p = get_packet();
//  char *msg = "kazuha pop pop!";
//  uint16_t len = strlen(msg);
//  send_packet(one, (uint8_t*)msg, len, SYN);
 while (1);
}
int main(UNUSED int argc, UNUSED char const *argv[]) {
   net_init(rank);
   thread_init(1, 256, NULL, 0);
   thread_create(&thread_entry, 0, 1, 1, NULL);
   scheduler_start(1000); // just the default thread;
   while(1);
}
