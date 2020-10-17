#include "rtm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

routing_table_t routing_table;

void routing_table_init() { memset(&routing_table, 0, sizeof(routing_table)); }

int routing_table_lookup_route(char destination[DESTINATION_SIZE], char mask) {
  for (int i = 0; i < routing_table.route_count; i++)
    if (strcmp(routing_table.route[i].destination, destination) == 0 &&
        routing_table.route[i].mask == mask)
      return i;
  return -1;
}
int routing_table_add_route(char destination[DESTINATION_SIZE], char mask,
                            char gateway[GATEWAY_SIZE], char oif[OIF_SIZE]) {
  if (routing_table.route_count >= MAX_ROUTES) {
    perror("the routing table is full");
    return -1;
  }
  int position = routing_table_lookup_route(destination, mask);
  if (position != -1) {
    perror("the routing table already has this route");
    return -1;
  }
  position = routing_table.route_count;
  strncpy(routing_table.route[position].destination, destination,
          DESTINATION_SIZE - 1);
  routing_table.route[position].mask = mask;
  strncpy(routing_table.route[position].gateway, gateway, GATEWAY_SIZE - 1);
  strncpy(routing_table.route[position].oif, oif, OIF_SIZE - 1);
  routing_table.route_count++;

  return position;
}

int routing_table_update_route(char destination[DESTINATION_SIZE], char mask,
                               char gateway[GATEWAY_SIZE], char oif[OIF_SIZE]) {
  int position = routing_table_lookup_route(destination, mask);
  if (position > -1) {
    strncpy(routing_table.route[position].gateway, gateway, GATEWAY_SIZE - 1);
    strncpy(routing_table.route[position].oif, oif, OIF_SIZE - 1);
    return position;
  }
  perror("the routing table does not have this route");
  return -1;
}

int routing_table_delete_route(char destination[DESTINATION_SIZE], char mask) {
  int position = routing_table_lookup_route(destination, mask);
  if (position > -1) {
    /* memmove(routing_table.route + position * sizeof(route_t),
            routing_table.route + (position + 1) * sizeof(route_t),
            (routing_table.route_count - position - 1) * sizeof(route_t)); */
    for (int i = position + 1; i < routing_table.route_count; i++)
      routing_table.route[i - 1] = routing_table.route[i];
    routing_table.route_count--;
    return routing_table.route_count;
  }
  perror("the routing table does not have this route");
  return -1;
}
int routing_table_print() {
  for (int i = 0; i < routing_table.route_count; i++)
    printf("%d: %s/%d %s %s\n", i, routing_table.route[i].destination,
           (int)routing_table.route[i].mask, routing_table.route[i].gateway,
           routing_table.route[i].oif);

  return routing_table.route_count;
}

int dump_rounting_table(int fd) {
  int ret = write(fd, buffer, BUFFER_SIZE);
  if (ret == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }
}
