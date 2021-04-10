#include "rtm.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  int i;
  int ret;
  int data_socket;
  char buffer[BUFFER_SIZE];

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
  for (;;) {
    memset(&msg, 0, sizeof(sync_msg_t));
    ret = read(data_socket, &msg, sizeof(sync_msg_t));
    if (ret == -1) {
      perror("read");
      break;
    }
    switch (msg.op_code) {
    case 'C':
      routing_table_routes_add(&msg.route, "");
      break;
    case 'U':
      routing_table_routes_update(&msg.route, "");
      break;
    case 'D':
      routing_table_routes_delete(&msg.route, false);
      break;
    case 'L':
      routing_table_print();
      break;
    }
  };

  /* Send arguments. */
  do {
    printf("Enter number to send to server :\n");
    scanf("%d", &i);
    ret = write(data_socket, &i,
                sizeof(int)); // alternative system calls: sendmsg(), sendto()
    if (ret == -1) {
      perror("write");
      break;
    }
    printf("No of bytes sent = %d, data sent = %d\n", ret, i);
  } while (i);

  /* Request result. */

  memset(buffer, 0, BUFFER_SIZE);
  strncpy(buffer, "RES", strlen("RES"));
  buffer[strlen(buffer)] = '\0';
  printf("buffer = %s\n", buffer);

  ret = write(data_socket, buffer, strlen(buffer));
  if (ret == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  /* Receive result. */
  memset(buffer, 0, BUFFER_SIZE);

  ret = read(data_socket, buffer, BUFFER_SIZE);
  if (ret == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  /* Ensure buffer is 0-terminated. */

  buffer[BUFFER_SIZE - 1] = 0;

  printf("Result = %s\n", buffer);

  /* Close socket. */

  close(data_socket);
  fflush(stdout);

  exit(EXIT_SUCCESS);
}
