#ifndef _TINIMPI_H
#define _TINIMPI_H
#include <stdint.h>
#include <network.h>

typedef uint8_t rank_t;
typedef uint8_t tag_t;

typedef struct {
    volatile int active;
    rank_t src;       // BROADCAST means match any source
    uint8_t opcode;
    tag_t tag;        // only checked for SYN
    packet_t result;
    volatile int done;
} recv_req_t;

void tinimpi_init(rank_t *rank, void (*idle_func)());
void tinimpi_send(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len);
void tinimpi_recv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len);
void tinimpi_send2(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len);
void tinimpi_recv2(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len);
void tinimpi_barrier();
void tinimpi_barrier2();
void tinimpi_bcast(uint8_t *buf, uint16_t len, int root);

void tinimpi_thread();

#endif
