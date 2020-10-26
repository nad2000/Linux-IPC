#include "rtm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

routing_table_t routing_table;
arp_table_t arp_table;
const int arp_table_size = sizeof(arp_table);

int create_arp_table() {

  int shm_fd;

  /*Create a shared memory object in kernel space. If shared memory already
   * exists it will truncate it to zero bytes again*/
  shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDWR | O_TRUNC, 0660);

  if (shm_fd < 0) {
    printf("failure on shm_open on shm_fd, errcode = %d\n", errno);
    return -1;
  }

  if (ftruncate(shm_fd, sizeof(arp_table)) == -1) {
    printf("Error on ftruncate to allocate size of shared memory region\n");
    return -1;
  }

  /*Now map the shared memory in kernel space into process's Virtual address
   * space*/
  void *shm_reg = mmap(
      NULL, // let the kernel chose to return the base address of shm memory
      sizeof(arp_table), // sizeof the shared memory to map to virtual address
                         // space of the process
      PROT_READ | PROT_WRITE, // shared memory is Read and Writable
      MAP_SHARED, // shared memory is accessible by different processes
      shm_fd,     // file descriptor of the shared memory
      0); // offset from the base address of the physical/shared memory to be
          // mapped

  /* shm_reg is the address in process's Virtual address space, just like any
   * other address. The Linux paging mechanism maps this address to starting
   * address of the shared memory region in kernel space. Any operation
   * performed by process on shm_reg address is actually the operation
   * performed in shared memory which resides in kernel*/
  memset(shm_reg, 0, sizeof(arp_table));
  munmap(shm_reg, sizeof(arp_table));
  /*Reader process will not be able to read shm if writer unlink
   * the name below*/
  // shm_unlink(mmap_key);
  close(shm_fd);
  return sizeof(arp_table);
}

void routing_table_init() {
  memset(&routing_table, 0, sizeof(routing_table));
  memset(&arp_table, 0, sizeof(arp_table));
  create_arp_table();
}

int routing_table_lookup_route(char destination[DESTINATION_SIZE], char mask) {
  for (int i = 0; i < routing_table.route_count; i++)
    if (strcmp(routing_table.route[i].destination, destination) == 0 &&
        routing_table.route[i].mask == mask)
      return i;
  return -1;
}

int read_arp_table() {

  int shm_fd = 0, rc = 0;

  shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDONLY, 0660);

  if (shm_fd < 0) {
    printf("failure on shm_open on shm_fd, error code = %d", errno);
    return -1;
  }

  void *shm_reg = mmap(NULL, arp_table_size, PROT_READ, MAP_SHARED, shm_fd, 0);

  if (shm_reg == MAP_FAILED) {
    printf("Error on mapping\n");
    return -1;
  }

  memcpy(&arp_table.mac_count, shm_reg, sizeof(int));
  memcpy(arp_table.mac, shm_reg + sizeof(int),
         sizeof(char[18]) * arp_table.mac_count);
  rc = munmap(shm_reg, arp_table_size);

  if (rc < 0) {
    printf("munmap failed\n");
    return -1;
  }
  close(shm_fd);
  return sizeof(char[18]) * arp_table.mac_count + sizeof(int);
}

int routing_table_print() {
  read_arp_table();
  for (int i = 0; i < routing_table.route_count; i++)
    printf("%d: %s/%d %s %s %s\n", i, routing_table.route[i].destination,
           (int)routing_table.route[i].mask, routing_table.route[i].gateway,
           arp_table.mac[i], routing_table.route[i].oif);

  return routing_table.route_count;
}

int dump_rounting_table(int fd) {

  sync_msg_t msg;

  for (int i = 0; i < routing_table.route_count; i++) {
    msg.op_code = CREATE;
    msg.route = routing_table.route[i];
    int ret = write(fd, &msg, sizeof(sync_msg_t));
    if (ret == -1) {
      perror("failed to sync a route");
      exit(EXIT_FAILURE);
    }
  }
  return routing_table.route_count;
}

int inline routing_table_routes_lookup(route_t *route) {
  for (int i = 0; i < routing_table.route_count; i++)
    if (strcmp(routing_table.route[i].destination, route->destination) == 0 &&
        routing_table.route[i].mask == route->mask)
      return i;
  return -1;
}

int inline routing_table_routes_add(route_t *route, char mac[18]) {
  if (routing_table.route_count >= MAX_ROUTES) {
    perror("the routing table is full");
    return -1;
  }
  int position = routing_table_routes_lookup(route);
  if (position != -1) {
    perror("the routing table already has this route");
    return -1;
  }
  position = routing_table.route_count;
  strncpy(routing_table.route[position].destination, route->destination,
          DESTINATION_SIZE - 1);
  routing_table.route[position].mask = route->mask;
  strncpy(routing_table.route[position].gateway, route->gateway,
          GATEWAY_SIZE - 1);
  strncpy(routing_table.route[position].oif, route->oif, OIF_SIZE - 1);
  add_mac(mac);
  routing_table.route_count++;
  fprintf(stderr, "CREATED: %s/%d %s %s\n", route->destination, route->mask,
          route->gateway, route->oif);

  return position;
}

int inline routing_table_routes_update(route_t *route, char mac[18]) {
  int position = routing_table_routes_lookup(route);
  if (position > -1) {
    strncpy(routing_table.route[position].gateway, route->gateway,
            GATEWAY_SIZE - 1);
    strncpy(routing_table.route[position].oif, route->oif, OIF_SIZE - 1);
    fprintf(stderr, "UPDATED: %s/%d %s %s to %s %s\n", route->destination,
            route->mask, routing_table.route[position].gateway,
            routing_table.route[position].oif, route->gateway, route->oif);
    return position;
  }
  perror("the routing table does not have this route");
  return -1;
}

int inline routing_table_routes_delete(route_t *route) {
  int position = routing_table_routes_lookup(route);
  if (position > -1) {
    /* memmove(routing_table.route + position * sizeof(route_t),
            routing_table.route + (position + 1) * sizeof(route_t),
            (routing_table.route_count - position - 1) * sizeof(route_t)); */
    fprintf(stderr, "DELETED: %s/%d %s %s\n",
            routing_table.route[position].destination,
            routing_table.route[position].mask,
            routing_table.route[position].gateway,
            routing_table.route[position].oif);
    for (int i = position + 1; i < routing_table.route_count; i++)
      routing_table.route[i - 1] = routing_table.route[i];
    routing_table.route_count--;
    return routing_table.route_count;
  }
  perror("the routing table does not have this route");
  return -1;
}

int add_mac(char *mac) {

  memcpy(arp_table.mac[arp_table.mac_count++], mac, 18);

  int shm_fd;

  /*Create a shared memory object in kernel space. If shared memory already
   * exists it will truncate it to zero bytes again*/
  // shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDWR | O_TRUNC, 0660);
  shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDWR, 0660);

  if (shm_fd < 0) {
    printf("failure on shm_open on shm_fd, errcode = %d\n", errno);
    return -1;
  }

  /* if (ftruncate(shm_fd, arp_table_size) == -1) { */
  /*   printf("Error on ftruncate to allocate size of shared memory region\n");
   */
  /*   return -1; */
  /* } */

  /*Now map the shared memory in kernel space into process's Virtual address
   * space*/
  void *shm_reg = mmap(
      NULL, // let the kernel chose to return the base address of shm memory
      arp_table_size, // sizeof the shared memory to map to virtual address
                      // space of the process
      PROT_READ | PROT_WRITE, // shared memory is Read and Writable
      MAP_SHARED, // shared memory is accessible by different processes
      shm_fd,     // file descriptor of the shared memory
      0); // offset from the base address of the physical/shared memory to be
          // mapped

  /* shm_reg is the address in process's Virtual address space, just like any
   * other address. The Linux paging mechanism maps this address to starting
   * address of the shared memory region in kernel space. Any operation
   * performed by process on shm_reg address is actually the operation
   * performed in shared memory which resides in kernel*/
  // memset(shm_reg, 0, arp_table_size);
  // memcpy(&arp_table.mac_count, shm_reg, sizeof(int));
  // memcpy(arp_table.mac, shm_reg, sizeof(char[18]) * arp_table.mac_count);

  memcpy(shm_reg, &arp_table,
         sizeof(char[18]) * arp_table.mac_count + sizeof(int));
  munmap(shm_reg, arp_table_size);
  /*Reader process will not be able to read shm if writer unlink
   * the name below*/
  // shm_unlink(mmap_key);
  close(shm_fd);
  return arp_table_size;
}

int arp_table_print() {
  int res = read_arp_table();
  if (res == -1)
    exit(res);
  for (int i = 0; i < arp_table.mac_count; i++) {
    printf("%d: %s\n", i, arp_table.mac[i]);
  }
  return 0;
}
