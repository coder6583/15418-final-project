/* TX Buffer Implementation */
void txbuf_add(char c);
char txbuf_rem();
char txbuf_empty();
char txbuf_full();

/* RX Buffer Implementation */
void rxbuf_add(char c);
char rxbuf_rem();
char rxbuf_empty();
char rxbuf_full();


