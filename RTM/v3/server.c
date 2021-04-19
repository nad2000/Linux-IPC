#include "rtm.h"

#include <ctype.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void sigquit_handler(int signal);
static char *initial_datat_filename = NULL;

/*An array of File descriptors which the server process
 * is maintaining in order to talk with the connected clients.
 * Master skt FD is also a member of this array*/
int monitored_fd_set[MAX_CLIENT_SUPPORTED];

pid_t client_pids[MAX_CLIENT_SUPPORTED] = {-1};

/*Remove all the FDs, if any, from the the array*/
static void intitiaze_monitor_fd_set() {

  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++)
    monitored_fd_set[i] = -1;
}

/*Add a new FD to the monitored_fd_set array*/
static int add_to_monitored_fd_set(int skt_fd) {

  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {

    if (monitored_fd_set[i] != -1)
      continue;
    monitored_fd_set[i] = skt_fd;
    break;
  }
  return i;
}

/*Remove the FD from monitored_fd_set array*/
static void remove_from_monitored_fd_set(int skt_fd) {

  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {

    if (monitored_fd_set[i] != skt_fd)
      continue;

    monitored_fd_set[i] = -1;
    break;
  }
}

/* Clone all the FDs in monitored_fd_set array into
 * fd_set Data structure*/
static void refresh_fd_set(fd_set *fd_set_ptr) {

  FD_ZERO(fd_set_ptr);
  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] != -1) {
      FD_SET(monitored_fd_set[i], fd_set_ptr);
    }
  }
}

/*Get the numerical max value among all FDs which server
 * is monitoring*/
static int get_max_fd() {

  int i = 0;
  int max = -1;

  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] > max)
      max = monitored_fd_set[i];
  }

  return max;
}

int quit = false;
void rl_cb(char *line) {

  sync_msg_t msg;
  route_t route;

  char op_code;
  char mac[18];

  if (NULL == line) {
    quit = true;
    return;
  }

  op_code = parse_route(line, &route, mac);

  switch (op_code) {

  case 'A':
    arp_table_print();
    break;
  case 'F':
    routing_table_flush(true);
    // Signal all clients to flush their routing tables
    for (int i = 1; i < MAX_CLIENT_SUPPORTED; i++) {
      if (client_pids[i] > 0)
        kill(client_pids[i], SIGUSR1);
    }
    break;
  case 'S':
  case 'W':
    routing_table_store();
    break;
  case 'H':
    printf("c[reate] - create an entry\n"
           "d[elete] - delete an entry\n"
           "f[lush] - flush the routing table\n"
           "h[elp] - help\n"
           "l[ist] - list all entries\n"
           "p[rint] - print the list of all client PIDs\n"
           "s[ave]/w[rite] - save the routing page into the storage file\n"
           "u[date] - update an entry\n");
    break;
  case 'P':
    printf("#\tPID\n");
    for (int i = 0; i < MAX_CLIENT_SUPPORTED; i++)
      if (client_pids[i] > 0)
        printf("%d\t%d\n", i, client_pids[i]);
    break;
  case 'L':
  case 'Q':
    if (op_code == 'Q')
      quit = true;
    else
      routing_table_print();
  case 'C':
  case 'U':
  case 'D':
    if (op_code == 'C')
      routing_table_routes_add(&route, mac);
    else if (op_code == 'U')
      routing_table_routes_update(&route, mac);
    else if (op_code == 'D') {
      if (is_all_digitsn(route.destination, sizeof(route.destination))) {
        int idx = atoi(route.destination);
        routing_table_routes_delete_by_idx(idx, true);
      } else
        routing_table_routes_delete(&route, true);
    }
    // sync the entry with all the clients:
    msg.op_code = op_code;
    if (op_code == 'L' || op_code == 'Q')
      memset(&msg.route, 0, sizeof(route_t));
    else
      msg.route = route;

    for (int i = 2; i < MAX_CLIENT_SUPPORTED; i++) {

      if (monitored_fd_set[i] == -1)
        break;
      int ret = write(monitored_fd_set[i], &msg, sizeof(sync_msg_t));
      if (ret == -1) {
        perror("failed to sync a route");
        exit(EXIT_FAILURE);
      } else if (debug)
        fprintf(stderr, "Synced route %s/%d %s %s %s to %d\n",
                route.destination, route.mask, route.gateway, mac, route.oif,
                monitored_fd_set[i]);
    }
  }

  if (strlen(line) > 0)
    add_history(line);

  // printf("You typed:\n%s\n", line);
  free(line);
}

int main(int argc, char *argv[]) {

  struct sockaddr_un name;

#if 0
    struct sockaddr_un {
        sa_family_t sun_family;               /* AF_UNIX */
        char        sun_path[108];            /* pathname */
    };
#endif

  int ret;
  int connection_socket;
  int data_socket;
  // int result;
  fd_set readfds;
  int comm_socket_fd, i;
  int c;

  while ((c = getopt(argc, argv, "dhf:i:")) != -1)
    // printf("%c %s\n", c, optarg);
    switch (c) {
    case 'd':
      debug = true;
      break;
    case 'f':
      routing_table_filename = optarg;
      break;
    case 'i':
      initial_datat_filename = optarg;
      break;
    case 'h':
      printf("Usage: %s [-d] [-h] [-f FILE] [-i FILE]\n\n"
             "\t-d - show debuging info\n"
             "\t-f FILE - the file with the stored routes (default: '%s')\n"
             "\t-i FILE - initial data file with the stored commands\n"
             "\t-h - show this help\n",
             argv[0], ROUTING_TABLE_FILENAME);
      return 0;
    case '?':
      if (optopt == 'f' || optopt == 'i')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    }

  routing_table_init();
  if (initial_datat_filename != NULL)
    routing_table_load(initial_datat_filename);
  routing_table_load(NULL);

  intitiaze_monitor_fd_set();
  add_to_monitored_fd_set(0);

  rl_callback_handler_install("# ", (rl_vcpfunc_t *)&rl_cb);

  /*In case the program exited inadvertently on the last run,
   *remove the socket.
   **/

  unlink(SOCKET_NAME);

  /* Create Master socket. */

  /*SOCK_DGRAM for Datagram based communication*/
  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_socket == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (debug)
    fprintf(stderr, "Master socket created\n");

  /*initialize*/
  memset(&name, 0, sizeof(struct sockaddr_un));

  /*Specify the socket Cridentials*/
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

  /* Bind socket to socket name.*/
  /* Purpose of bind() system call is that application() dictate the
   * underlying operating system the criteria of recieving the data. Here,
   * bind() system call is telling the OS that if sender process sends the
   * data destined to socket "/tmp/DemoSocket", then such data needs to be
   * delivered to this server process (the server process)*/
  ret = bind(connection_socket, (const struct sockaddr *)&name,
             sizeof(struct sockaddr_un));

  if (ret == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (debug)
    fprintf(stderr, "bind() call succeed\n");
  /*
   * Prepare for accepting connections. The backlog size is set
   * to 20. So while one request is being processed other requests
   * can be waiting.
   * */

  ret = listen(connection_socket, 20);
  if (ret == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  /*Add master socket to Monitored set of FDs*/
  add_to_monitored_fd_set(connection_socket);

  signal(SIGQUIT, sigquit_handler);

  /* This is the main loop for handling connections. */
  /*All Server process usually runs 24 x 7. Good Servers should always up
   * and running and shold never go down. Have you ever seen Facebook Or
   * Google page failed to load ??*/
  for (;;) {
    refresh_fd_set(&readfds); /*Copy the entire monitored FDs to readfds*/
    /* Wait for incoming connection. */
    if (debug)
      fprintf(stderr, "Waiting on select() sys call\n");

    /* Call the select system call, server process blocks here.
     * Linux OS keeps this process blocked untill the connection initiation
     * request Or data requests arrives on any of the file Drscriptors in the
     * 'readfds' set*/

    select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);

    if (FD_ISSET(connection_socket, &readfds)) {

      /*Data arrives on Master socket only when new client connects with the
       * server (that is, 'connect' call is invoked on client side)*/
      if (debug)
        fprintf(stderr,
                "New connection recieved recvd, accept the connection\n");

      data_socket = accept(connection_socket, NULL, NULL);

      if (data_socket == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      if (debug)
        fprintf(stderr, "Connection accepted from client. Data socket: %d\n",
                data_socket);

      int client_index = add_to_monitored_fd_set(data_socket);
      dump_rounting_table(data_socket);

      pid_t pid = 0;
      ret = read(data_socket, &pid, sizeof(pid_t));
      if (ret == -1) {
        perror("failed to recieve PID");
        exit(EXIT_FAILURE);
      }
      fprintf(stderr, "Client PID=%d connected...\n", pid);
      client_pids[client_index] = pid;
    } else if (FD_ISSET(0, &readfds)) {

      rl_callback_read_char();
      if (quit) {
        routing_table_store();
        close_arp_shm();
        exit(0);
      }

    } else /* Data srrives on some other client FD*/
    {
      char op_code;
      /*Find the client which has send us the data request*/
      i = 1, comm_socket_fd = -1;
      for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] > 0 &&
            FD_ISSET(monitored_fd_set[i], &readfds)) {
          comm_socket_fd = monitored_fd_set[i];

          /* Wait for next data packet. */
          /*Server is blocked here. Waiting for the data to arrive from client
           * 'read' is a blocking system call*/
          if (debug)
            fprintf(stderr, "Waiting for data from the client\n");
          ret = read(comm_socket_fd, &op_code, sizeof(op_code));
          if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
          }

          switch (op_code) {
          case 'Q':
	    fprintf(stderr, "Client PID=%d quit...\n", client_pids[i]);
            client_pids[i] = -1;
            monitored_fd_set[i] = -1;
            break;
          default:
            fprintf(stderr, "Unknown OP code: '%c'\n", op_code);
          }
        }
      }
    }
  } /*go to select() and block*/

  /*close the master socket*/
  close(connection_socket);
  remove_from_monitored_fd_set(connection_socket);
  if (debug)
    fprintf(stderr, "connection closed..\n");

  /* Server should release resources before getting terminated.
   * Unlink the socket. */

  unlink(SOCKET_NAME);
  exit(EXIT_SUCCESS);
}

static void sigquit_handler(int signal) {
  quit = true;
  routing_table_store();
  close_arp_shm();
  exit(0);
}
