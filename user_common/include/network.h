#ifndef _NETWORK_H
#define _NETWORK_H
#include <stdint.h>
#include <stdbool.h>

#define _NETWORK_START_BYTE 0x73
#define _NETWORK_TTL_INIT 16
#define _NETWORK_MAX_PAYLOAD_SIZE 64
#define _NETWORK_HEADER_SIZE 7
#define _NETWORK_MAX_PACKET_SIZE _NETWORK_MAX_PAYLOAD_SIZE + 7
typedef uint8_t addr_t;
typedef uint8_t opcode_t;

/* packet opcode definitions */
#define SYN 0
#define ACK 1
#define DATA 2
#define BARRIER 3

/* special addresses */
#define BROADCAST 255

// type definition for a packet
// header: 7 bytes, payload: 64 bytes
// we don't want padding here
typedef struct __attribute__((packed)) {
  uint8_t __start; // magic number
  addr_t src;   // source address (where is the packet coming from)
  addr_t dest;     // destination address (where is the packet going to)
  uint8_t ttl;     // time-to-live (drop packets that circulate the ring network for too long)
  uint8_t len;     // size of the data in the payload
  opcode_t opcode; // opcode
  uint8_t seq;     // packet index for payloads with over max packet size
  uint8_t payload[_NETWORK_MAX_PAYLOAD_SIZE];
} packet_t;

// initialize the network
void net_init(addr_t a);

// get the next packet from the packet buffer
packet_t get_packet();

// send a packet onto the ring network
void send_packet(addr_t dest, uint8_t *data, uint8_t len, opcode_t op, uint8_t seq);

// print it
void print_packet(packet_t p);

// get my own addr
addr_t get_addr();
#endif
