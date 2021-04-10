#ifndef FILE_RTM_SEEN
#define FILE_RTM_SEEN

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ROUTING_TABLE_FILENAME "rtm.data"
#define SOCKET_NAME "/tmp/DemoSocket"
#define ARP_TABLE_KEY "/arp_table"
#define BUFFER_SIZE 128

#define MAX_CLIENT_SUPPORTED 64
#define MAX_ROUTES 128

#define DESTINATION_SIZE 16
#define GATEWAY_SIZE 16
#define OIF_SIZE 32

typedef enum {
  ARP = 'A',
  CREATE = 'C',
  DELETE = 'D',
  LIST = 'L',
  QUIT = 'Q',
  UPDATE = 'U'
} OPCODE;

typedef struct _route {
  char destination[16];
  char mask;
  char gateway[16];
  char oif[32];
} route_t;

typedef struct _sync_msg {
  OPCODE op_code;
  route_t route;
} sync_msg_t;

typedef struct _routinge_table {
  int route_count;
  route_t route[MAX_ROUTES];
} routing_table_t;

typedef struct _arp_table {
  int mac_count;
  char mac[MAX_ROUTES][18];
} arp_table_t;

void routing_table_init();

/* int routing_table_lookup_route(char destination[16], char mask); */
/* int routing_table_add_route(char destination[16], char mask, char
 * gateway[16], */
/*                             char oif[32]); */
/* int routing_table_update_route(char destination[16], char mask, */
/*                                char gateway[16], char oif[32]); */
/* int routing_table_delete_route(char destination[16], char mask); */
char parse_route(char *buffer, route_t *route, char mac[18]);
char read_route(int fd, route_t *route, char mac[18]);
int routing_table_print();
int routing_table_store();
int routing_table_load();
int arp_table_print();
int add_mac(char *mac);
int delete_mac(int entry_idx);
int dump_rounting_table(int fd);
int routing_table_routes_lookup(route_t *route_t);
int routing_table_routes_add(route_t *route, char mac[18]);
int routing_table_routes_update(route_t *route, char mac[18]);
int routing_table_routes_delete(route_t *route, bool include_mac);
int open_arp_table_ro();
void close_arp_shm();

extern bool debug;
extern char *routing_table_filename;
extern int shm_fd;
extern void *shm_reg;

#endif /* !FILE_RTM_SEEN */
