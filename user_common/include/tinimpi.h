#ifndef _TINIMPI_H
#define _TINIMPI_H
#include <stdint.h>

typedef uint8_t rank_t;
typedef uint8_t tag_t;

typedef struct {
    rank_t src;
    uint8_t opcode;      // what opcode we're waiting for (SYN, ACK, DATA)
    tag_t tag;           // only checked for SYN
    uint8_t *buf;        // where to write payload bytes
    uint16_t buf_cap;
    uint16_t *out_len;
    volatile int done;   // mpi_thread sets this when matched
} recv_req_t;

void tinimpi_init(); 
void tinimpi_send(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len);
void tinimpi_recv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len);
void tinimpi_barrier();

void tinimpi_thread();

#endif
