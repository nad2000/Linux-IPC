#include "rtm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

routing_table_t routing_table;
arp_table_t arp_table;
const int arp_table_size = sizeof(arp_table);
bool debug = false;
char *routing_table_filename = ROUTING_TABLE_FILENAME;

int shm_fd;
void *shm_reg;

bool inline is_all_digitsn(char data[], int n) {
  bool all_digits = true;
  for (int i = 0; i < n; i++) {
    if (!data[i])
      break;
    if (data[i] < '0' || data[i] > '9') {
      all_digits = false;
      break;
    }
  }
  return all_digits;
}

int create_arp_table() {

  memset(&arp_table, 0, arp_table_size);

  /*Create a shared memory object in kernel space. If shared memory already
   * exists it will truncate it to zero bytes again*/
  shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDWR | O_TRUNC, 0660);
  if (shm_fd < 0) {
    printf("failure on shm_open on shm_fd, errcode = %d\n", errno);
    return -1;
  }

  if (ftruncate(shm_fd, arp_table_size) == -1) {
    printf("Error on ftruncate to allocate size of shared memory region\n");
    return -1;
  }

  /*Now map the shared memory in kernel space into process's Virtual address
   * space*/
  shm_reg = mmap(
      NULL, // let the kernel chose to return the base address of shm memory
      arp_table_size, // sizeof the shared memory to map to virtual address
                      // space of the process
      PROT_READ | PROT_WRITE, // shared memory is Read and Writable
      MAP_SHARED, // shared memory is accessible by different processes
      shm_fd,     // file descriptor of the shared memory
      0); // offset from the base address of the physical/shared memory to be
          // mapped

  memset(shm_reg, 0, arp_table_size);
  // munmap(shm_reg, arp_table_size);
  // close(shm_fd);
  // shm_unlink(ARP_TABLE_KEY);
  return arp_table_size;
}

void close_arp_shm() {
  munmap(shm_reg, arp_table_size);
  close(shm_fd);
  shm_unlink(ARP_TABLE_KEY);
}

void routing_table_init() {
  memset(&routing_table, 0, sizeof(routing_table));
  create_arp_table();
}

int routing_table_lookup_route(char destination[DESTINATION_SIZE], char mask) {
  for (int i = 0; i < routing_table.route_count; i++)
    if (strcmp(routing_table.route[i].destination, destination) == 0 &&
        routing_table.route[i].mask == mask)
      return i;
  return -1;
}

int open_arp_table_ro() {
  shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDONLY, 0660);
  if (shm_fd < 0) {
    printf("failure on shm_open on shm_fd, error code = %d", errno);
    return -1;
  }
  shm_reg = mmap(NULL, arp_table_size, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (shm_reg == MAP_FAILED) {
    printf("Error on mapping\n");
    return -1;
  }
  return 0;
}

int read_arp_table() {

  // int rc = 0;

  // shm_fd = shm_open(ARP_TABLE_KEY, O_CREAT | O_RDONLY, 0660);

  memcpy(&arp_table, shm_reg, arp_table_size);
  // memcpy(&arp_table.mac_count, shm_reg, sizeof(int));
  // memcpy(arp_table.mac, shm_reg + sizeof(int),
  //        sizeof(char[18]) * arp_table.mac_count);
  // rc = munmap(shm_reg, arp_table_size);
  // if (rc < 0) {
  //   printf("munmap failed\n");
  //   return -1;
  // }
  // close(shm_fd);
  return sizeof(char[18]) * arp_table.mac_count + sizeof(int);
}

int routing_table_print() {
  read_arp_table();
  printf("#\tDEST\tMASK\tGW\tMAC\tOIF\n");
  for (int i = 0; i < routing_table.route_count; i++)
    printf("%d\t%s\t%d\t%s\t%s\t%s\n", i, routing_table.route[i].destination,
           (int)routing_table.route[i].mask, routing_table.route[i].gateway,
           arp_table.mac[i], routing_table.route[i].oif);
  return routing_table.route_count;
}

int dump_rounting_table(int fd) {

  sync_msg_t msg;
  int ret;

  for (int i = 0; i < routing_table.route_count; i++) {
    msg.op_code = 'C';
    msg.route = routing_table.route[i];
    ret = write(fd, &msg, sizeof(sync_msg_t));
    if (ret == -1) {
      perror("failed to sync a route");
      exit(EXIT_FAILURE);
    }
  }
  memset(&msg, 0, sizeof(sync_msg_t));
  msg.op_code = 'F';
  ret = write(fd, &msg, sizeof(sync_msg_t));
  if (ret == -1) {
    perror("failed to send 'Finish Flushing'");
    exit(EXIT_FAILURE);
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

  if (mac != NULL && strlen(mac) > 0) {
    add_mac(mac);
    if (debug)
      fprintf(stderr, "CREATED: %s/%d %s %s %s\n", route->destination,
              route->mask, route->gateway, mac, route->oif);
  } else {
    if (debug)
      fprintf(stderr, "CREATED: %s/%d %s %s\n", route->destination, route->mask,
              route->gateway, route->oif);
  }
  routing_table.route_count++;

  return position;
}

int inline routing_table_routes_update(route_t *route, char mac[18]) {
  int position = routing_table_routes_lookup(route);
  if (position > -1) {
    strncpy(routing_table.route[position].gateway, route->gateway,
            GATEWAY_SIZE - 1);
    strncpy(routing_table.route[position].oif, route->oif, OIF_SIZE - 1);
    if (debug)
      fprintf(stderr, "UPDATED: %s/%d %s %s to %s %s\n", route->destination,
              route->mask, routing_table.route[position].gateway,
              routing_table.route[position].oif, route->gateway, route->oif);
    return position;
  }
  perror("the routing table does not have this route");
  return -1;
}

int inline routing_table_routes_delete(route_t *route, bool include_mac) {
  int position = routing_table_routes_lookup(route);
  if (position > -1) {
    return routing_table_routes_delete_by_idx(position, include_mac);
  }
  perror("the routing table does not have this route");
  return -1;
}

int inline routing_table_routes_delete_by_idx(int position, bool include_mac) {
  if (debug)
    fprintf(stderr, "DELETED: %s/%d %s %s %s\n",
            routing_table.route[position].destination,
            routing_table.route[position].mask,
            routing_table.route[position].gateway, arp_table.mac[position],
            routing_table.route[position].oif);
  memmove(routing_table.route + position * sizeof(route_t),
          routing_table.route + (position + 1) * sizeof(route_t),
          (routing_table.route_count - position - 1) * sizeof(route_t));
  for (int i = position + 1; i < routing_table.route_count; i++)
    routing_table.route[i - 1] = routing_table.route[i];
  routing_table.route_count--;
  if (include_mac)
    delete_mac(position);
  return routing_table.route_count;
}

int add_mac(char *mac) {
  memcpy(arp_table.mac[arp_table.mac_count++], mac, 18);
  memcpy(shm_reg, &arp_table, arp_table_size);
  return arp_table.mac_count;
}

int delete_mac(int entry_idx) {

  if (entry_idx < arp_table.mac_count - 1) {
    printf("there is no ARP table entry with index: %d", entry_idx);
    return -1;
  }

  arp_table.mac_count--;
  if (entry_idx < arp_table.mac_count - 1)
    memcpy(arp_table.mac + entry_idx * (sizeof(char[18])),
           arp_table.mac + (entry_idx + 1) * (sizeof(char[18])),
           sizeof(char[18]) * (arp_table.mac_count + entry_idx));
  memset(&arp_table.mac[arp_table.mac_count], 0, sizeof(char[18]));

  memcpy(shm_reg, &arp_table,
         sizeof(char[18]) * arp_table.mac_count + sizeof(int));
  return arp_table.mac_count;
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

char parse_route(char *buffer, route_t *route, char mac[18]) {
  char *token = strtok(buffer, " ");
  // if (!token)
  //   return (char)0;
  // token = strtok(buffer, " ");
  if (!token)
    return (char)0;
  char op_code = toupper(token[0]);

  if (op_code == 'C' || op_code == 'U' || op_code == 'D') {
    char ip_address[20];

    // route_t route;
    token = strtok(NULL, " ");
    if (!token) {
      fprintf(stderr, "ERROR: Incorrect input '%s'", buffer);
      return (char)0;
    }
    strncpy(ip_address, token, DESTINATION_SIZE);

    if (op_code == 'C' || op_code == 'U') {
      token = strtok(NULL, " ");
      if (!token) {
        fprintf(stderr, "ERROR: Incorrect input '%s'", buffer);
        return (char)0;
      }
      strncpy(route->gateway, token, GATEWAY_SIZE);
      token = strtok(NULL, " ");
      if (!token) {
        fprintf(stderr, "ERROR: Incorrect input '%s'", buffer);
        return (char)0;
      }
      strncpy(mac, token, 17);
      token = strtok(NULL, "\n");
      if (!token) {
        fprintf(stderr, "ERROR: Incorrect input '%s'", buffer);
        return (char)0;
      }
      strncpy(route->oif, token, OIF_SIZE);
    }

    token = strtok(ip_address, "/");
    if (!token) {
      fprintf(stderr, "ERROR: Incorrect input '%s'", buffer);
      return (char)0;
    }
    strncpy(route->destination, token, DESTINATION_SIZE);

    token = strtok(NULL, "/");
    route->mask = token ? (char)atoi(token) : (char)255;
  }
  return op_code;
}

char read_route(FILE *fp, route_t *route, char mac[18]) {
  char buffer[BUFFER_SIZE];
  char *ret;

  memset(buffer, 0, BUFFER_SIZE);
  ret = fgets(buffer, BUFFER_SIZE, fp);
  if (ret == NULL)
    return (char)0;
  // if (debug) {
  if (fileno(fp) == 0)
    fprintf(stderr, "Input read from console : %s\n", buffer);
  else
    fprintf(stderr, "Input read from %d : %s\n", fileno(fp), buffer);
  // }
  return parse_route(buffer, route, mac);
}

int routing_table_load(const char *filename) {
  if (filename == NULL)
    filename = routing_table_filename;
  if (access(filename, R_OK) != -1) {

    route_t route;
    char op_code;
    char mac[18];

    FILE *fp = fopen(routing_table_filename, "ro");
    while ((op_code = read_route(fp, &route, mac)) == 'C')
      routing_table_routes_add(&route, mac);
    return fclose(fp);
  }
  return -1;
}

int routing_table_store() {

  FILE *fp = fopen(routing_table_filename, "w");
  for (int i = 0; i < routing_table.route_count; i++) {
    fprintf(fp, "C %s/%d %s %s %s\n", routing_table.route[i].destination,
            routing_table.route[i].mask, routing_table.route[i].gateway,
            arp_table.mac[i], routing_table.route[i].oif);
  }
  return fclose(fp);
}

int inline routing_table_flush(bool include_mac) {
  if (include_mac) {
    memset(&arp_table, 0, arp_table_size);
    memset(shm_reg, 0, arp_table_size);
  }
  memset(&routing_table, 0, sizeof(routing_table));
  fprintf(stderr, "*** Routing table flushed...\n");
  return 0;
}
