#include <tinimpi.h>
#include <network.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define DEBUG_MPI 0

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

void tinimpi_barrier() {
  rank_t checklist[]
}
