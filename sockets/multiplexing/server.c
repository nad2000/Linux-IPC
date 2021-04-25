#include "common.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

char buffer[1024];
bool debug;

#if 0
typedef struct fd_set {
        u_int   fd_count;               /* how many are SET? */
        SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} fd_set;
#endif

int monitored_fd_set[MAX_CLIENT_SUPPORTED];
int fd_count = 0;
int master_socket_tcp_fd = 0;

static void intitiaze_monitor_fd_set();
static int add_to_monitored_fd_set(int fd);
static void remove_from_monitored_fd_set(int fd);
static void refresh_fd_set(fd_set *fd_set_ptr);
static int get_max_fd();

void setup_tcp_server_communication() {
  // Step 1: Initialization
  int sent_recv_bytes = 0;

  // client specific communication socket FD:
  int comm_socket_fd;

  // FD set on which select polls:
  fd_set readfds;
  FD_ZERO(&readfds);

  // Server and client info:
  struct sockaddr_in server_addr, client_addr;

  // Step 2: TCP master socket creation:
  if ((master_socket_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    errExit0("Socket creation failed");

  // set master socket to allow multiple connections ,
  // this is just a good habit, it will work without this
  int opt = true;
  if (setsockopt(master_socket_tcp_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0)
    errExit0("setsockopt");

  // Step 3: Specify server info
  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_port = SERVER_PORT;
  server_addr.sin_addr.s_addr = INADDR_ANY; // OR address range to listen on

  socklen_t addr_len = (socklen_t)sizeof(struct sockaddr);
  if (bind(master_socket_tcp_fd, (struct sockaddr *)&server_addr, addr_len) ==
      -1)
    errExit0("Socket bind failed");

  //
  // Step 4: Tell OS to maintain  the queue of max length to Queue incoming
  // client connections.
  if (listen(master_socket_tcp_fd, 5) < 0)
    errExit0("Listen filed");

  intitiaze_monitor_fd_set();

  for (;;) {

    // Step 5: Initialize and fill readfds:
    refresh_fd_set(&readfds);

    // Step 6: Wait for client connections
    if (debug)
      fprintf(stderr, "Block on SELECT System call...\n");
    select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);

    if (FD_ISSET(master_socket_tcp_fd, &readfds)) {
      if (debug)
        fprintf(
            stderr,
            "New connection received, accept the connection.\n"
            "Client and Server completes TCP 3 way handshake at this point.\n");
      // Step 7: accept() returns a new temporary FD...
      comm_socket_fd = accept(master_socket_tcp_fd,
                              (struct sockaddr *)&client_addr, &addr_len);
      if (comm_socket_fd < 0)
        errExit("accept error: errno = '%d'", errno);

      if (debug)
        fprintf(stderr, "Connection accepted from client: %s:%u\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      add_to_monitored_fd_set(comm_socket_fd);
      // FD_SET(comm_socket_fd, &readfds); // add the master socket file
      // descriptor.
    }
    for (int i = 0; i < fd_count; i++) {

      if (FD_ISSET(monitored_fd_set[i], &readfds))
        comm_socket_fd = monitored_fd_set[i];
      else
        continue;

      memset(buffer, 0, sizeof(buffer));

      // Step 8: service the client request
      sent_recv_bytes = recvfrom(comm_socket_fd, (char *)buffer, sizeof(buffer),
                                 0, (struct sockaddr *)&client_addr, &addr_len);
      if (sent_recv_bytes == 0) {
        close(comm_socket_fd);
        break; // goto step 5
      }
      request_t *request = (request_t *)buffer;

      // Stop 9
      if (!(request->a || request->b)) {
        close(comm_socket_fd);
        remove_from_monitored_fd_set(comm_socket_fd);
        if (debug)
          fprintf(stderr, "Server closes connection with client: %s:%u\n",
                  inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        break; // goto step 5
      }

      result_t result;
      result.c = request->a + request->b;

      // Send the reply to the client
      sent_recv_bytes =
          sendto(comm_socket_fd, (char *)&result, sizeof(result_t), 0,
                 (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

      if (debug)
        fprintf(stderr, "Server sent %d bytes in reply to client\n",
                sent_recv_bytes);
    }
  }
}

int main(int argc, char **argv) {
  char c;

  while ((c = getopt(argc, argv, "dh")) != -1)
    // printf("%c %s\n", c, optarg);
    switch (c) {
    case 'd':
      debug = true;
      break;
    /* case 'f':
      routing_table_filename = optarg;
      break;
    case 'i':
      initial_datat_filename = optarg;
      break; */
    case 'h':
      printf("Usage: %s [-d] [-h]\n\n"
             "\t-d - show debuging info\n"
             "\t-h - show this help\n",
             argv[0]);
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

  setup_tcp_server_communication();
  return 0;
}

/* Remove all the FDs, if any, from the the array */
static void intitiaze_monitor_fd_set() {
  memset(monitored_fd_set, -1, sizeof(int) * MAX_CLIENT_SUPPORTED);
  // for (int i = 0; i < MAX_CLIENT_SUPPORTED; i++)
  //   monitored_fd_set[i] = -1;
}

/* Add a new FD to the monitored_fd_set array */
static int add_to_monitored_fd_set(int fd) {
  monitored_fd_set[fd_count++] = fd;
  return fd_count;
}

/* Remove the FD from monitored_fd_set array */
static void remove_from_monitored_fd_set(int fd) {
  for (int i = 0; i < fd_count; i++) {
    if (monitored_fd_set[i] != fd)
      continue;
    monitored_fd_set[i] = -1;
    fd_count--;
    memmove(&monitored_fd_set + i * sizeof(int),
            &monitored_fd_set + (i + 1) * sizeof(int),
            (fd_count - i) * sizeof(int));
    break;
  }
}

/* Clone all the FDs in monitored_fd_set array into
 * fd_set Data structure*/
static void refresh_fd_set(fd_set *fd_set_ptr) {
  FD_ZERO(fd_set_ptr);
  FD_SET(master_socket_tcp_fd, fd_set_ptr);
  for (int i = 0; i < fd_count; i++) {
    FD_SET(monitored_fd_set[i], fd_set_ptr);
  }
}

/* Get the numerical max value among all FDs which server
 * is monitoring*/
static int get_max_fd() {
  int max = master_socket_tcp_fd > 0 ? master_socket_tcp_fd : -1;
  for (int i = 0; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] > max)
      max = monitored_fd_set[i];
  }
  return max;
}
