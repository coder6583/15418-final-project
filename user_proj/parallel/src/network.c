#include <network.h>
#include <packet.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// initialize everything we need here
void net_init() {
  spi_init(LINK_IN);
  spi_init(LINK_OUT);
}

inline uint8_t packet_size(packet_t p) {
  return 4 + p.len;
}

packet_t get_packet() {
  packet_t p;
  uint8_t buf[sizeof(packet_t)];

  spi_receive(buf, sizeof(packet_t));

  p.__start = buf[0];
  p.src = buf[1];
  p.dest = buf[2];
  p.ttl = buf[3];
  p.len = buf[4];

  memcpy(p.payload, buf+5, p.len);
  
  return p;
}

// send `len` bytes, from `src`, to `dest`
void send_packet(addr_t src, addr_t dest, uint8_t *data, uint8_t len) {
  uint8_t buf[_NETWORK_MAX_PACKET_SIZE];

  buf[0] = _NETWORK_START_BYTE;
  buf[1] = src;
  buf[2] = dest;
  buf[3] = _NETWORK_TTL_INIT;
  buf[4] = len;

  memcpy(buf+5, data, len);

  spi_transmit(buf, 5+len);
}

// print a packet in a clean way
void print_packet(packet_t p) {
  printf("=== PACKET RECIEVED ===\n");
  printf("---> src: %d, dest: %d\n", p.src, p.dest);
  printf("---> TTL: %d, data: %d bytes\n", p.ttl, p.len);
  printf("---> Data: %s\n", p.payload);
}



