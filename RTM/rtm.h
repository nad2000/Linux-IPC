#ifndef FILE_RTM_SEEN
#define FILE_RTM_SEEN

#define SOCKET_NAME "/tmp/DemoSocket"
#define BUFFER_SIZE 128

#define MAX_CLIENT_SUPPORTED 64
#define MAX_ROUTES 128

#define DESTINATION_SIZE 16
#define GATEWAY_SIZE 16
#define OIF_SIZE 32

typedef enum { CREATE = 'C', UPDATE = 'U', DELETE = 'D' } OPCODE;

typedef struct _route {
  char destination[16];
  char mask;
  char gateway[16];
  char oif[32];
} route_t;

typedef struct _sync_msg {
  OPCODE op_code;
  route_t msg_body;
} sync_msg_t;

typedef struct _routinge_table {
  route_t route[MAX_ROUTES];
  int route_count;
} routing_table_t;

void routing_table_init();

int routing_table_lookup_route(char destination[16], char mask);
int routing_table_add_route(char destination[16], char mask, char gateway[16],
                            char oif[32]);
int routing_table_update_route(char destination[16], char mask,
                               char gateway[16], char oif[32]);
int routing_table_delete_route(char destination[16], char mask);
int routing_table_print();

#endif /* !FILE_RTM_SEEN */
