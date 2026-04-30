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

static recv_req_t pending;

/* Auto-ACK'd SYNs that arrived before tinimpi_recv2 was called */
typedef struct {
    rank_t src;
    tag_t tag;
    uint16_t expected_len;
} buffered_syn_t;

#define SYN_QUEUE_SIZE 4
static buffered_syn_t syn_queue[SYN_QUEUE_SIZE];
static volatile uint8_t syn_head = 0;
static volatile uint8_t syn_tail = 0;

/* General buffer for unexpected ACK/DATA packets */
#define PKTBUF_SIZE 16
static int pktbuf_valid[PKTBUF_SIZE];
static packet_t pktbuf[PKTBUF_SIZE];

void tinimpi_init(rank_t *rank, void (*idle_func)()) {
  pending.active     = 0;
  pending.done       = 0;
  syn_head = syn_tail = 0;
  for (int i = 0; i < PKTBUF_SIZE; i++) pktbuf_valid[i] = 0;
  *rank = get_rank();
  printf("rank %d\n", *rank);
  net_init(*rank);
  thread_init(1, 256, idle_func, 0);
  thread_create(&tinimpi_thread, 0, 1, 1, NULL);
  scheduler_start(1000); // just the default thread;
}

static packet_t wait_for(rank_t src, uint8_t opcode, tag_t tag) {
    /* check unexpected buffer before blocking */
    for (int i = 0; i < PKTBUF_SIZE; i++) {
        if (!pktbuf_valid[i]) continue;
        packet_t *p = &pktbuf[i];
        int src_match = (src == BROADCAST) || (src == p->src);
        int op_match  = (opcode == p->opcode);
        int tag_match = (opcode != SYN) || (tag == p->payload[0]);
        if (src_match && op_match && tag_match) {
            pktbuf_valid[i] = 0;
            return *p;
        }
    }
    /* not buffered — block until tinimpi_thread delivers it */
    pending.src    = src;
    pending.opcode = opcode;
    pending.tag    = tag;
    pending.done   = 0;
    pending.active = 1;
    while (!pending.done);
    pending.active = 0;
    return pending.result;
}

#define DEBUG_MPI 0

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
  while (1) {
    packet_t p;
    if (handle_packet(&p)) {
      int delivered = 0;
      if (pending.active && !pending.done) {
        int src_match = (pending.src == BROADCAST) || (pending.src == p.src);
        int op_match  = (pending.opcode == p.opcode);
        int tag_match = (p.opcode != SYN) || (pending.tag == p.payload[0]);
        if (src_match && op_match && tag_match) {
          pending.result = p;
          pending.done   = 1;
          delivered = 1;
        }
      }
      if (!delivered) {
        uint8_t syn_next = (syn_head + 1) % SYN_QUEUE_SIZE;
        if (p.opcode == SYN && syn_next != syn_tail) {
          /* auto-ACK so the sender can proceed past its wait_for(ACK) */
          send_packet(p.src, NULL, 0, ACK, 0);
          syn_queue[syn_head].src = p.src;
          syn_queue[syn_head].tag = p.payload[0];
          syn_queue[syn_head].expected_len = ((uint16_t)p.payload[1] << 8) | p.payload[2];
          syn_head = syn_next;
        } else {
          /* buffer ACK/DATA that arrived before wait_for was posted */
          for (int i = 0; i < PKTBUF_SIZE; i++) {
            if (!pktbuf_valid[i]) {
              pktbuf[i]       = p;
              pktbuf_valid[i] = 1;
              break;
            }
          }
        }
      }
    }
    spi_progress_tx();
    wait_until_next_period();
  }
}

/* ── pending-slot versions of send/recv ──────────────────────────────────── */

void tinimpi_send2(rank_t dest, tag_t tag, uint8_t *buf, uint16_t len) {
    uint8_t header_data[] = { tag, (len >> 8) & 0xFF, len & 0xFF };
    MPI_DEBUG("sending SYN to rank %d, tag=%d\n", dest, tag);
    send_packet(dest, header_data, 3, SYN, 0);

    MPI_DEBUG("waiting for ACK from %d...\n", dest);
    wait_for(dest, ACK, 0);
    MPI_DEBUG("received ACK from %d!\n", dest);

    MPI_DEBUG("sending data to %d!\n", dest);
    for (uint16_t offset = 0; offset < len; offset += _NETWORK_MAX_PAYLOAD_SIZE) {
        uint16_t chunk = len - offset;
        if (chunk > _NETWORK_MAX_PAYLOAD_SIZE) chunk = _NETWORK_MAX_PAYLOAD_SIZE;
        send_packet(dest, buf + offset, chunk, DATA, offset / _NETWORK_MAX_PAYLOAD_SIZE);
    }
}

void tinimpi_recv2(rank_t src, tag_t tag, uint8_t *buf, uint16_t buf_capacity, uint16_t *out_len) {
    uint16_t expected_len;
    int syn_found = 0;
    for (uint8_t i = syn_tail; i != syn_head; i = (i + 1) % SYN_QUEUE_SIZE) {
        if (syn_queue[i].src == src && syn_queue[i].tag == tag) {
            MPI_DEBUG("recv2: using buffered SYN from %d\n", src);
            expected_len = syn_queue[i].expected_len;
            /* remove this entry by shifting tail forward if it's the oldest,
               otherwise swap with tail and advance tail */
            if (i == syn_tail) {
                syn_tail = (syn_tail + 1) % SYN_QUEUE_SIZE;
            } else {
                syn_queue[i] = syn_queue[syn_tail];
                syn_tail = (syn_tail + 1) % SYN_QUEUE_SIZE;
            }
            syn_found = 1;
            break;
        }
    }
    if (!syn_found) {
        MPI_DEBUG("waiting for SYN from %d...\n", src);
        packet_t p = wait_for(src, SYN, tag);
        expected_len = ((uint16_t)p.payload[1] << 8) | p.payload[2];
        MPI_DEBUG("received SYN from %d, expecting %d bytes\n", src, expected_len);
        MPI_DEBUG("sending ACK to rank %d...\n", src);
        send_packet(src, NULL, 0, ACK, 0);
    }

    *out_len = 0;
    uint8_t last_seq = (expected_len - 1) / _NETWORK_MAX_PAYLOAD_SIZE;
    while (1) {
        packet_t p = wait_for(src, DATA, 0);
        MPI_DEBUG("received data from %d, seq=%d len=%d\n", src, p.seq, p.len);
        memcpy(buf + (p.seq * _NETWORK_MAX_PAYLOAD_SIZE), p.payload, p.len);
        *out_len += p.len;
        if (p.seq == last_seq) break;
    }

    if (*out_len > buf_capacity) *out_len = buf_capacity;
}

void tinimpi_barrier2() {
    addr_t me = get_addr();
    bool checklist[TOPOLOGY];
    for (int i = 0; i < TOPOLOGY; i++) checklist[i] = false;
    checklist[me] = true;
    send_packet(BROADCAST, NULL, 0, BARRIER, 0);

    while (1) {
        packet_t p = wait_for(BROADCAST, BARRIER, 0);
        if (!checklist[p.src]) {
            checklist[p.src] = true;
            send_packet(BROADCAST, NULL, 0, BARRIER, 0);
        }
        bool done = true;
        for (int i = 0; i < TOPOLOGY; i++) {
            if (!checklist[i]) { done = false; break; }
        }
        if (done) {
            sleep(10);
            send_packet(BROADCAST, NULL, 0, BARRIER, 0);
            break;
        }
    }
    MPI_DEBUG("exiting barrier\n");
}
