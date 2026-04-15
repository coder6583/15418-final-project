#include <buf.h>
#include <stdint.h>

#define BUFSIZE 2048

char buf_tx[BUFSIZE];
char buf_rx[BUFSIZE];

uint64_t buf_head_rx = 0;
uint64_t buf_tail_rx = 0;
uint64_t buf_head_tx = 0;
uint64_t buf_tail_tx = 0;
/* TX Buffer */

static char tx_full = 0;
static char tx_empty = 1;
static char rx_full = 0;
static char rx_empty = 1;
// add a char to the TX buffer
void txbuf_add(char c) {
    uint64_t next_head = (buf_head_tx + 1) % (BUFSIZE);

    if (next_head == buf_tail_tx) {
	    tx_full = 1;
    } else
	    tx_full = 0;
	
    tx_empty = 0;
    buf_tx[buf_head_tx] = c;
    buf_head_tx = next_head;
    return;
}

// remove a char from the TX buffer
char txbuf_rem() {
    char c = buf_tx[buf_tail_tx];
    uint64_t next_tail = (buf_tail_tx + 1) % BUFSIZE;
    if (next_tail == buf_head_tx) {
        tx_empty = 1;
    } else {
	tx_empty = 0;
    }

    tx_full = 0;
    buf_tail_tx = next_tail;
    return c;
}

// is the TX buffer empty?
char txbuf_empty() {
    return tx_empty;
}

// is the TX buffer full?
char txbuf_full() {
	return tx_full;
}

/* RX Buffer */

// add a char to the RX buffer
void rxbuf_add(char c) {
    uint64_t next_head = (buf_head_rx + 1) % (BUFSIZE);

    if (next_head == buf_tail_rx) {
	    rx_full = 1;
    } else
	    rx_full = 0;

    rx_empty = 0;
    buf_rx[buf_head_rx] = c;
    buf_head_rx = next_head;
    return;
}

// remove a char from the RX buffer
char rxbuf_rem() {
    char c = buf_rx[buf_tail_rx];
    uint64_t next_tail = (buf_tail_rx + 1) % BUFSIZE;
    if (next_tail == buf_head_rx) {
	    rx_empty = 1;
    } else {
	rx_empty = 0;
    }

    rx_full = 0;
    buf_tail_rx = next_tail;
    return c;
}

//  is the RX buffer empty?
char rxbuf_empty() {
    return rx_empty;
}

// is the RX buffer full?
char rxbuf_full() {
    return rx_full;
}
