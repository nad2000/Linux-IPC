#include "rtm.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static void sigusr1_handler(int signal);
static void sigquit_handler(int signal);
static void quit_client();
static int data_socket;

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  int ret;

  signal(SIGUSR1, sigusr1_handler);

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
  signal(SIGQUIT, sigquit_handler);

  // Read dumped routeing table
  // sync_msg_t msg;
  pid_t pid;
  char op_code;
  route_t route;

  for (;;) {
    ret = read(data_socket, &op_code, sizeof(op_code));
    if (ret == -1) {
      perror("read op_code");
      break;
    }
    if (op_code != 'Q' && op_code != 'F') {
      memset(&route, 0, sizeof(route_t));
      ret = read(data_socket, &route, sizeof(route_t));
      if (ret == -1) {
        perror("read op_code");
        break;
      }
    }
    switch (op_code) {
    case 'A':
      arp_table_print();
      break;
    case 'C':
      routing_table_routes_add(&route, "");
      break;
    case 'U':
      routing_table_routes_update(&route, "");
      break;
    case 'D':
      // destination is the index in the table
      if (is_all_digitsn(route.destination, sizeof(route.destination))) {
        int idx = atoi(route.destination);
        routing_table_routes_delete_by_idx(idx, false);
      } else
        routing_table_routes_delete(&route, false);
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
  quit_client();
}

static void clean_client_data() {
  close_arp_shm();
  close(data_socket);
  fflush(stdout);
  fflush(stderr);
}

static void quit_client() {
  clean_client_data();
  exit(EXIT_SUCCESS);
}

static void sigusr1_handler(int signal) { routing_table_flush(false); }
static void sigquit_handler(int signal) {

  const char op_code = 'Q';
  int ret = write(data_socket, &op_code, sizeof(char));
  if (ret == -1) {
    perror("failed to signal server about the exit");
    clean_client_data();
    exit(EXIT_FAILURE);
  }
  quit_client();
}
