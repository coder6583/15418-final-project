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
#include <matmul.h>

int A[N][N] = {
  {0, 0, 0, 0, 12, 7, 23, 4},
  {0, 0, 0, 0, 5, 18, 9, 31},
  {0, 0, 0, 0, 27, 2, 14, 11},
  {0, 0, 0, 0, 8, 25, 6, 19},
  {0, 0, 0, 0, 16, 10, 29, 3},
  {0, 0, 0, 0, 21, 13, 1, 24},
  {0, 0, 0, 0, 9, 30, 17, 5},
  {0, 0, 0, 0, 28, 15, 8, 22},
};

uint8_t v[N] = {
  0,
  0,
  0,
  0,
  -10,
  -12,
  -14,
  -16
};

 
void thread_entry() {
 tag_t t = 16;

  char buf[128];
  uint16_t out_len;
  tinimpi_recv(NODE_ONE, t, (uint8_t*) buf, 128, &out_len);
  printf("Recieved message over tiniMPI: %s\n", buf); 
  printf("Entering barrier....\n");
  tinimpi_barrier();
  printf("Exiting barrier!\n");

  printf("calling allgather\n");
  print_vector(v, N);
  tinimpi_allgather(v+3, v, 4);
  print_vector(v, N);
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
