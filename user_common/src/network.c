#include <network.h>
#include <packet.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <tinimpi.h>

// hard-coded address
static addr_t addr = 0;
// initialize everything we need here
void net_init(addr_t a) {
  spi_init(LINK_IN);
  spi_init(LINK_OUT);
  addr = a;
}

inline uint8_t packet_size(packet_t p) {
  return 5 + p.len;
}

packet_t build_packet(uint8_t buf[_NETWORK_MAX_PACKET_SIZE + 1]) {
  packet_t p;
  p.__start = buf[0];
  p.src = buf[1];
  p.ttl = buf[3];
  p.dest = buf[2];
  p.len = buf[4];
  p.opcode = buf[5];
  p.seq = buf[6];
  memcpy(p.payload, buf+_NETWORK_HEADER_SIZE, p.len);
  p.payload[p.len] = '\0';
  return p;
}

// block until we recieve a packet, forwarding
// irrelevant packets as we go
packet_t get_packet() {
  packet_t p;
  uint8_t buf[_NETWORK_MAX_PACKET_SIZE];
  while (1) {
    spi_receive(buf, _NETWORK_MAX_PACKET_SIZE);
    p.__start = buf[0];
    p.src = buf[1];
    p.ttl = buf[3];
    p.dest = buf[2];
    p.len = buf[4];
    p.opcode = buf[5];
    p.seq = buf[6];
    memcpy(p.payload, buf+_NETWORK_HEADER_SIZE, p.len);
    p.payload[p.len] = '\0';
//    print_packet(p);
    if (p.dest == addr) { 
      return p;
    } else if (p.dest == BROADCAST && p.src == addr) {
      return p;
    } else {
      // packet is not for us, forward if possible;
      // first checked if ttl is invalid range somehow
      if (0 < p.ttl && p.ttl <= _NETWORK_TTL_INIT) {
        buf[3]--;
        spi_transmit(buf, _NETWORK_HEADER_SIZE+p.len);
      }
      if (p.dest == BROADCAST) {
        return p;
      }
      // ttl = 0 means packet dropped
    }
  }  
}

// send `len` bytes, from `src`, to `dest`
void send_packet(addr_t dest, uint8_t *data, uint8_t len, opcode_t op, uint8_t seq) {
  uint8_t buf[_NETWORK_MAX_PACKET_SIZE];

  buf[0] = _NETWORK_START_BYTE;
  buf[1] = addr;
  buf[2] = dest;
  buf[3] = _NETWORK_TTL_INIT;
  buf[4] = len;
  buf[5] = op;
  buf[6] = seq;
  if (data != NULL) memcpy(buf+_NETWORK_HEADER_SIZE, data, len);
  spi_transmit(buf, _NETWORK_MAX_PACKET_SIZE);
}

// print a packet in a clean way
void print_packet(packet_t p) {
  printf("=== PACKET RECIEVED ===\n");
  printf("---> src: %d, dest: %d\n", p.src, p.dest);
  printf("---> TTL: %d, data: %d bytes\n", p.ttl, p.len);
  switch (p.opcode) {
    case SYN: 
      printf("---> opcode: SYN\n");
      break;
    case ACK: 
      printf("---> opcode: ACK\n");
      break;
    case DATA: 
      printf("---> opcode: DATA\n");
      break;
    case BARRIER:
      printf("---> opcode: BARRIER\n");
      break;
  }
  printf("---> Data: %s\n", p.payload);
}

addr_t get_addr() { return addr; }

