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

typedef enum {
    TREQ_SEND_WAIT_ACK = 0,
    TREQ_RECV_WAIT_SYN,
    TREQ_RECV_WAIT_DATA,
    TREQ_DONE,
} treq_state_t;

typedef struct {
    treq_state_t  state;
    rank_t        peer;
    tag_t         tag;
    uint8_t      *buf;
    uint16_t      len;          /* send: total bytes; recv: buf_capacity */
    uint16_t     *out_len;      /* recv only */
    uint16_t      expected_len; /* recv: filled from SYN */
} tinimpi_req_t;

void tinimpi_init(rank_t *rank, void (*idle_func)());
void tinimpi_send(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len);
void tinimpi_recv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len);
void tinimpi_send2(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len);
void tinimpi_recv2(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len);
void tinimpi_isend(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len, tinimpi_req_t *req);
void tinimpi_irecv(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len, tinimpi_req_t *req);
void tinimpi_wait(tinimpi_req_t *req);
void tinimpi_barrier();
void tinimpi_barrier2();

void tinimpi_thread();

#endif
