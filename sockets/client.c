#include "common.h"
#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

char buffer[1024];

void setup_tcp_communication() {
  // Step 1: Initialization - Socket handle
  int sockfd = 0, sent_recv_bytes = 0;
  socklen_t addr_len = (socklen_t)sizeof(struct sockaddr);

  struct sockaddr_in dest;

  // Step 2: Specify server information:
  dest.sin_family = AF_INET;
  dest.sin_port = SERVER_PORT;

  struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);

  // Step 3: Create a TCP socket
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  connect(sockfd, (struct sockaddr *)&dest, addr_len);

  // Step 4: get data to be sent to the server
PROMP_USER:

  printf("Enter a: ?\n");
}

int main(int argc, char **argv) {
  setup_tcp_communication();
  return 0;
}

