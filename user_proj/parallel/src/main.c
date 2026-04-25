#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <network.h>
#include <string.h>

int main(UNUSED int argc, UNUSED char const *argv[]) {
  net_init();

  // transmission
  //char *s = "hello";

  // transmit
  while (1) {
    char *s = "Hello, 15-418! stfu";
    send_packet(1, 2, (uint8_t*)s, strlen(s));
    for (int i = 0; i < 10000; i++);
  }

  // receive
/*   while (1) {
    packet_t p = get_packet();
    print_packet(p);
    for (int i = 0; i < 10000; i++);
  } */
}
