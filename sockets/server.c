#include "common.h"
#include <errno.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

char buffer[1024];

void setup_tcp_server_communication() {
  // Step 1: Initialization
  int master_socket_tcp_fd = 0, sent_recv_bytes = 0, addr_len = 0, opt = 1;

  // client specific communication socket FD:
  int comm_socket_fd;

  // FD set on which select polls:
  fd_set readfds;

  // Server and client info:
  struct sockaddr_in server_addr, client_addr;

  // Step 2: TCP master socket creation:
  if ((master_socket_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    errExit("Socket creation failed");

  // Step 3: Specify server info
  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_port = SERVER_PORT;
  server_addr.sin_addr.s_addr = INADDR_ANY; // OR address range to listen on

  const addr_len = sizeof(struct sockaddr);
  if (bind(master_socket_tcp_fd, (struct sockaddr *)&server_addr, addr_len) ==
      -1)
    errExit("Socket bind failed");

  //
  // Step 4: Tell OS to maintain  the queue of max length to Queue incoming
  // client connections.
  if (listen(master_socket_tcp_fd, 5) < 0)
    errExit("Listen filed");

  for (;;) {
    // Step 5: Initialize and fill readfds:
    FD_ZERO(&readfds); // clears (removes all the file descriptors from) set.
    FD_SET(master_socket_tcp_fd,
           &readfds); // add the master socket file descriptor.

    // Step 6: Wait for client connections
    fprintf(stderr, "Block on SELECT System call...\n");
    select(master_socket_tcp_fd + 1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(master_socket_tcp_fd, &readfds)) {
      fprintf(
          stderr,
          "New connection received, accept the connection.\n"
          "Client and Server completes TCP 3 way handshake at this point.\n");
      // Step 7: accept() returns a new temporary FD...
      comm_socket_fd = accept(master_socket_tcp_fd,
                              (struct sockaddr *)&client_addr, &addr_len);
      if (comm_socket_fd < 0)
        errExit("accept error: errno = '%d'", errno);

      fprintf(stderr, "Connection accepted from client: %s:%u\n",
              inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      for (;;) {
        fprintf(stderr, "Server ready to service client msgs.\n");
        memset(buffer, 0, sizeof(buffer));
      }
    }
  }
}

int main(int argc, char **argv) {
  setup_tcp_server_communication();
  return 0;
}
