#include "rtm.h"
#include <stdio.h>

int main() {
  // char destination[16];
  // char mask;
  // char gateway[16];
  // char oif[32];
  routing_table_init();
  route_t r;
  char mac[18];
  for (int i = 0; i < MAX_ROUTES; i++) {
    sprintf(r.destination, "%d.%d.%d.%d", i % 3 + 1, i % 5 + 7, i % 7 + 11,
            i % 11 + 13);
    sprintf(r.gateway, "%d.%d.%d.%d", i % 7 + 11, i % 11 + 13, i % 5 + 7,
            i % 3 + 1);
    sprintf(r.oif, "Ethernet%d", i % 5 + 1);
    r.mask = (char)((i % 4 + 1) * 8);

    sprintf(mac, "%02d:%02d:%02d:%02d:%02d:%02d", i % 3, i % 4, i % 5, i % 7,
            i % 11, i % 13);
    routing_table_routes_add(&r, mac);
  }
  routing_table_routes_add(
      &((route_t){"2.9.12.19", (char)32, "1.1.1.1", "UPDATED123"}),
      "ab:12:34:56:78:11");
  routing_table_routes_add(
      &((route_t){"1.8.17.14", (char)32, "2.2.2.2", "UPDATED222"}),
      "ab:12:34:56:78:22");
  routing_table_routes_add(
      &((route_t){"2.9.11.15", (char)32, "3.3.3.3", "UPDATED333"}),
      "ab:12:34:56:78:33");
  routing_table_routes_add(
      &((route_t){"2.11.16.16", (char)8, "4.4.4.4", "UPDATE4444"}),
      "ab:12:34:56:78:44");

  routing_table_routes_delete(&((route_t){"2.9.12.19", (char)32, "", ""}));
  routing_table_routes_delete(&((route_t){"1.8.17.14", (char)32, "", ""}));
  routing_table_routes_delete(&((route_t){"2.9.11.15", (char)32, "", ""}));
  routing_table_print();

  return 0;
}
