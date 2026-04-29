#include <tinimpi.h>
#include <network.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <topology.h>
#include <stdbool.h>
#include <sleep.h>
#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>

#define MAX_PENDING 4
static volatile recv_req_t pending[MAX_PENDING];

#define DEBUG_MPI 1

#if DEBUG_MPI
#define MPI_DEBUG(...) printf(__VA_ARGS__)
#else
#define MPI_DEBUG(...)
#endif

// blocking: send data, wait for response
void tinimpi_send(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len) {
  uint8_t header_data[] = { tag, (len >> 8) & 0xFF, len & 0xFF };
  // send SYN
  MPI_DEBUG("sending SYN to rank %d,tag=%d\n", dest, tag);
  send_packet(dest, header_data, 3, SYN, 0);
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

  for (uint16_t offset = 0; offset < len; offset += _NETWORK_MAX_PAYLOAD_SIZE) {
    uint16_t payload_len = len - offset;
    if (payload_len > _NETWORK_MAX_PAYLOAD_SIZE) {
      payload_len = _NETWORK_MAX_PAYLOAD_SIZE;
    }
    send_packet(dest, buf + offset, payload_len, DATA, offset / _NETWORK_MAX_PAYLOAD_SIZE);
  }
}

// blocking: wait for response
void tinimpi_recv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len) {
  packet_t p;

  uint16_t expected_len = 0;

  // wait for src to initiate
  MPI_DEBUG("waiting for SYN from %d...\n", src);
  while (1) {
    p = get_packet();
    if (p.src == src && p.opcode == SYN && p.payload[0] == tag) {
      expected_len += p.payload[1] << 8;
      expected_len += p.payload[2];
      MPI_DEBUG("recieved SYN from %d!\n", src);
      MPI_DEBUG("--> tag is [%d](%c)\n", tag, (char)(tag+'0'));
      break;
    }
  }
  MPI_DEBUG("expecting data length of %d...", expected_len);

  // send ACK
  MPI_DEBUG("sending ACK to rank %d...\n", src);
  send_packet(src, NULL, 0, ACK, 0);

  *out_len = 0;

  // wait for src to send data
  while (1) {
    p = get_packet();
    if (p.src == src && p.opcode == DATA) {
      MPI_DEBUG("recieved data from %d of length %d!\n", src, p.len);
      *out_len += p.len;
      memcpy(buf + (p.seq * _NETWORK_MAX_PAYLOAD_SIZE), p.payload, p.len);
      MPI_DEBUG("p.seq(%d) == (%d)\n", p.seq, (expected_len / _NETWORK_MAX_PAYLOAD_SIZE));
      if (p.seq == (expected_len / _NETWORK_MAX_PAYLOAD_SIZE)) {
        break;
      }
    }
  }
  
  // // place result in buffer
  if (*out_len > buf_capacity) 
    *out_len = buf_capacity;

  // memcpy(buf, p.payload, *out_len);
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
  send_packet(BROADCAST, NULL, 0, BARRIER, 0);

  while (1) {
    packet_t p = get_packet();
    // wait for other nodes to broadcast themselves
    if (p.dest == BROADCAST && p.opcode == BARRIER && !checklist[p.src]) {
      checklist[p.src] = true;
      send_packet(BROADCAST, NULL, 0, BARRIER, 0);
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
      send_packet(BROADCAST, NULL, 0, BARRIER, 0);
      break;
    }
  }
}

void tinimpi_thread() {
  while(1) {
    if (spi_rx_ready()) {
      printf("received smth\n");
      uint8_t buf[72];
      spi_rx_dequeue(buf, 71);
      packet_t p = build_packet(buf);
      print_packet(p);
      
      // if (p.src == src && p.opcode == SYN && p.payload[0])
    }
    spi_progress_tx();
    wait_until_next_period();
  }
}
