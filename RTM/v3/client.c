#include "rtm.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  int ret;
  int data_socket;

  /* Create data socket. */

  data_socket = socket(AF_UNIX, SOCK_STREAM, 0);

  if (data_socket == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /*
   * For portability clear the whole structure, since some
   * implementations have additional (nonstandard) fields in
   * the structure.
   * */

  memset(&addr, 0, sizeof(struct sockaddr_un));

  /* Connect socket to socket address */

  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

  ret = connect(data_socket, (const struct sockaddr *)&addr,
                sizeof(struct sockaddr_un));

  if (ret == -1) {
    fprintf(stderr, "The server is down.\n");
    exit(EXIT_FAILURE);
  }
  open_arp_table_ro();

  // Read dumped routeing table
  sync_msg_t msg;
  pid_t pid;

  for (;;) {
    memset(&msg, 0, sizeof(sync_msg_t));
    ret = read(data_socket, &msg, sizeof(sync_msg_t));
    if (ret == -1) {
      perror("read");
      break;
    }
    switch (msg.op_code) {
    case 'A':
      arp_table_print();
      break;
    case 'C':
      routing_table_routes_add(&msg.route, "");
      break;
    case 'U':
      routing_table_routes_update(&msg.route, "");
      break;
    case 'D':
      // destination is the index in the table
      if (is_all_digitsn(msg.route.destination,
                         sizeof(msg.route.destination))) {
        int idx = atoi(msg.route.destination);
        routing_table_routes_delete_by_idx(idx, false);
      } else
        routing_table_routes_delete(&msg.route, false);
      break;
    case 'L':
      routing_table_print();
      break;
    case 'Q':
      fprintf(stderr, "Server has quit...");
      goto QUIT;
    case 'F':
      // F(inish) the flushing of the routing page and send PID back to the
      // server
      pid = getpid();
      int ret = write(data_socket, &pid, sizeof(pid_t));
      if (ret == -1) {
        perror("failed to send PID");
        exit(EXIT_FAILURE);
      }
      break;
    }
  };

QUIT:

  close_arp_shm();
  close(data_socket);
  fflush(stdout);
  fflush(stderr);
  exit(EXIT_SUCCESS);
}
