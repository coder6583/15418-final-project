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

 
void thread_entry() {
 tag_t t = 16;

  char buf[128];
  uint16_t out_len;
  tinimpi_recv(NODE_ONE, t, (uint8_t*) buf, 128, &out_len);
  printf("Recieved message over tiniMPI: %s\n", buf); 
  printf("Entering barrier....\n");
  tinimpi_barrier();
  printf("Exiting barrier!\n");
// tinimpi_send(two, t, (uint8_t*)msg, len);    
    

//  packet_t p = get_packet();
//  char *msg = "kazuha pop pop!";
//  uint16_t len = strlen(msg);
//  send_packet(one, (uint8_t*)msg, len, SYN);
 while (1);
}
int main(UNUSED int argc, UNUSED char const *argv[]) {
  net_init(NODE_TWO);
  thread_init(1, 256, NULL, 0);
  thread_create(&thread_entry, 0, 1, 1, NULL);
  scheduler_start(1000); // just the default thread;
  thread_entry();
  while(1);
}
