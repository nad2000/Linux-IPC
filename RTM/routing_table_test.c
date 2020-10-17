#include "rtm.h"
#include <stdio.h>

int main() {
  char destination[16];
  // char mask;
  char gateway[16];
  char oif[32];
  routing_table_init();
  for (int i = 0; i < MAX_ROUTES; i++) {
    sprintf(destination, "%d.%d.%d.%d", i % 3 + 1, i % 5 + 7, i % 7 + 11,
            i % 11 + 13);
    sprintf(gateway, "%d.%d.%d.%d", i % 7 + 11, i % 11 + 13, i % 5 + 7,
            i % 3 + 1);
    sprintf(oif, "Ethernet%d", i % 5 + 1);
    routing_table_add_route(destination, (char)((i % 4 + 1) * 8), gateway, oif);
  }
  routing_table_update_route("2.9.12.19", (char)32, "1.1.1.1", "UPDATED123");
  routing_table_update_route("1.8.17.14", (char)32, "2.2.2.2", "UPDATED222");
  routing_table_update_route("2.9.11.15", (char)32, "3.3.3.3", "UPDATED333");
  routing_table_update_route("2.11.16.16", (char)8, "4.4.4.4", "UPDATE4444");
  routing_table_delete_route("2.9.12.19", (char)32);
  routing_table_delete_route("1.8.17.14", (char)32);
  routing_table_delete_route("2.9.11.15", (char)32);
  routing_table_print();

  return 0;
}
