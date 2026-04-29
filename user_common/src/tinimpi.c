#include <tinimpi.h>
#include <network.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <topology.h>
#include <stdbool.h>
#include <sleep.h>

#define DEBUG_MPI 1

#if DEBUG_MPI
#define MPI_DEBUG(...) printf(__VA_ARGS__)
#else
#define MPI_DEBUG(...)
#endif

// blocking: send data, wait for response
void tinimpi_send(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len) {
  uint8_t header_data[] = { tag };
  // send SYN
  MPI_DEBUG("sending SYN to rank %d,tag=%d\n", dest, tag);
  send_packet(dest, header_data, 1, SYN);
  packet_t p;
  // wait for ACK
  MPI_DEBUG("waiting for ACK from %d...\n", dest);
  while (1) {
    p = get_packet();
    if (p.src == dest && p.opcode == ACK) {
      MPI_DEBUG("recieved ACK from %d!\n", dest);
      break;
    }
    // should still be able to recieve packets and update internal buffer
  }
  // now, send data
  MPI_DEBUG("Sending data to %d!\n", dest);
  send_packet(dest, buf, len, DATA);
}

// blocking: wait for response
void tinimpi_recv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len) {
  packet_t p;

  // wait for src to initiate
  MPI_DEBUG("waiting for SYN from %d...\n", src);
  while (1) {
    p = get_packet();
    if (p.src == src && p.opcode == SYN && p.payload[0] == tag) {
      MPI_DEBUG("recieved SYN from %d!\n", src);
      MPI_DEBUG("--> tag is [%d](%c)\n", tag, (char)(tag+'0'));
      break;
    }
  }

  // send ACK
  MPI_DEBUG("sending ACK to rank %d...\n", src);
  send_packet(src, NULL, 0, ACK);

  // wait for src to send data
  while (1) {
    p = get_packet();
    if (p.src == src && p.opcode == DATA) {
      MPI_DEBUG("recieved data from %d!\n", src);
      break;
    }
  }
  
  // place result in buffer
  if (p.len > buf_capacity) 
    *out_len = buf_capacity;
  else
    *out_len = p.len;

  memcpy(buf, p.payload, *out_len);
  buf[*out_len] = '\0';
}

void print_checklist(const bool *c, const int len) {
  printf("[CHECKLIST]\n");
  for (int i = 0; i < len; i++) {
    if (c[i])
      printf("%d: [x]\n", i);
    else
      printf("%d: [ ]\n", i);
  }
}

void tinimpi_barrier() {
  addr_t me = get_addr();
  // create the checklist
  bool checklist[TOPOLOGY]; 

  // initialize it
  for(int i = 0; i < TOPOLOGY; i++) checklist[i] = false;

  // this node reached the barrier, so we automatically check ourselves
  checklist[me] = true;

  // if we are not the first to reach the barrier, don't wait and send right away.
  send_packet(BROADCAST, NULL, 0, BARRIER);

  while (1) {
    packet_t p = get_packet();
    // wait for other nodes to broadcast themselves
    if (p.dest == BROADCAST && p.opcode == BARRIER && !checklist[p.src]) {
      checklist[p.src] = true;
      send_packet(BROADCAST, NULL, 0, BARRIER);
    }

    // check if everyone has responded
    bool done = true;
    for (int i = 0; i < TOPOLOGY; i++) {
      if (!checklist[i]) {
        done = false;
        break;
      }
    }
    if (done) {
 //     print_checklist(checklist, TOPOLOGY);
      sleep(10);
      send_packet(BROADCAST, NULL, 0, BARRIER);
      break;
    }
  }
}

/* allgather
* assume: sendbuf has valid `chunk_size_bytes`,
* and recvbuf has (chunk_size)*TOPOLOGY valid bytes
* */
void tinimpi_allgather(uint8_t *sendbuf, uint8_t *recvbuf, uint8_t chunk_size) {
  addr_t me = get_addr();
  bool checklist[TOPOLOGY]; 
  // initialize it
  for(int i = 0; i < TOPOLOGY; i++) checklist[i] = false;

  // this node reached the barrier, so we automatically check ourselves
  checklist[me] = true;

  
  send_packet(BROADCAST, sendbuf, chunk_size, BARRIER);

  while (1) {
    packet_t p = get_packet();

    if (p.dest == BROADCAST && p.opcode == BARRIER 
        && !checklist[p.src] && p.len == chunk_size) {
      checklist[p.src] = true;
      memcpy(recvbuf+(p.src*chunk_size), p.payload, chunk_size);
      sleep(10);
      send_packet(BROADCAST, sendbuf, chunk_size, BARRIER);
    }

    bool done = true;
    for (int i = 0; i < TOPOLOGY; i++) {
      if (!checklist[i]) {
        done = false;
        break;
      }
    }
    if (done) {
      sleep(10);
      send_packet(BROADCAST, sendbuf, chunk_size, BARRIER);
      break;
    }
 
  }
}
 
