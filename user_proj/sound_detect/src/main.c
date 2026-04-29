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

rank_t rank;

rank_t get_next_rank() {
  return (rank + 1) % 2;
}

rank_t get_prev_rank() {
  if (rank == 0) {
    return 1;
  }
  return rank - 1;
}

void computation() {
  tag_t t = 16;

  sleep(5000);

  char *msg = "nayeon pop pop! 1 2 3 4 5 6 7, you make me feel like eleven! baby, i'm just trying to play it cool";
  uint16_t len = strlen(msg);
  char recv[127];
  uint16_t out_len;
  tinimpi_send2(get_next_rank(), t, (uint8_t*)msg, len);
  tinimpi_recv2(get_prev_rank(), t, (uint8_t*)recv, 127, &out_len);
  recv[out_len] = '\0';
  printf("Recieved message over tiniMPI: %s\n", recv);
  
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  tinimpi_init(&rank, &computation);
  while(1);
}
