#include <network.h>
#include <packet.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// hard-coded address
static addr_t addr = 1;
// initialize everything we need here
void net_init(addr_t a) {
  spi_init(LINK_IN);
  spi_init(LINK_OUT);
  addr = a;
  printf("addr is %d\n", addr);
}

inline uint8_t packet_size(packet_t p) {
  return 5 + p.len;
}

// block until we recieve a packet, forwarding
// irrelevant packets as we go
packet_t get_packet() {
  packet_t p;
  uint8_t buf[sizeof(packet_t)];
  while (1) {
    printf("reciving next bytes\n");
    spi_receive(buf, _NETWORK_HEADER_SIZE);
    printf("done\n");
    p.__start = buf[0];
    p.src = buf[1];
    p.dest = buf[2];
    p.ttl = buf[3];
    p.len = buf[4];
    p.opcode = buf[5];
    print_packet(p);
    if (p.dest == addr) { 
      // packet is for us
      spi_receive(buf, p.len);
      memcpy(p.payload, buf, p.len);
      return p;
    } else {
      // packet is not for us, forward if possible;
      // first checked if ttl is invalid range somehow
      if (0 < p.ttl && p.ttl <= _NETWORK_TTL_INIT) {
        printf("forwarding packet\n");
        buf[3]--;
        spi_transmit(buf, _NETWORK_HEADER_SIZE+p.len);
      }
      // ttl = 0 means packet dropped
    }
  }
  
}

// send `len` bytes, from `src`, to `dest`
void send_packet(addr_t dest, uint8_t *data, uint8_t len, opcode_t op) {
  uint8_t buf[_NETWORK_MAX_PACKET_SIZE];

  buf[0] = _NETWORK_START_BYTE;
  buf[1] = addr;
  buf[2] = dest;
  buf[3] = _NETWORK_TTL_INIT;
  buf[4] = len;
  buf[5] = op;
  spi_transmit(buf, _NETWORK_HEADER_SIZE);
  if (len > 0)
  spi_transmit(data, len);
}

// print a packet in a clean way
void print_packet(packet_t p) {
  printf("=== PACKET RECIEVED ===\n");
  printf("---> src: %d, dest: %d\n", p.src, p.dest);
  printf("---> TTL: %d, data: %d bytes\n", p.ttl, p.len);
  printf("---> opcode: %d\n", p.opcode);
  printf("---> Data: %s\n", p.payload);
}



