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
  request_t request;
  result_t result;
  // Step 1: Initialization - Socket handle
  int sockfd = 0, sent_recv_bytes = 0;
  socklen_t addr_len = (socklen_t)sizeof(struct sockaddr);

  struct sockaddr_in dest;

  // Step 2: Specify server information:
  dest.sin_family = AF_INET;
  dest.sin_port = SERVER_PORT;

  struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
  dest.sin_addr = *((struct in_addr *)host->h_addr);

  // Step 3: Create a TCP socket
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  connect(sockfd, (struct sockaddr *)&dest, addr_len);

  // Step 4: get data to be sent to the server
PROMP_USER:

  printf("Enter a: ?\n");
  scanf("%u", &request.a);
  printf("Enter b: ?\n");
  scanf("%u", &request.b);

  // Step 5: send the data to  server
  sent_recv_bytes = sendto(sockfd, &request, sizeof(request_t), 0,
                           (struct sockaddr *)&dest, addr_len);
  fprintf(stderr, "Bytes sent = %d\n", sent_recv_bytes);

  // Step 6: Receive the reply from the server:
  sent_recv_bytes = recvfrom(sockfd, (char *)&result, sizeof(result_t), 0,
                             (struct sockaddr *)&dest, &addr_len);
  fprintf(stderr, "Bytes received = %d\n", sent_recv_bytes);
  if (!sent_recv_bytes)
    exit(0);
  printf("A + B -> C = %u\n", result.c);

  goto PROMP_USER;
}

int main(int argc, char **argv) {
  setup_tcp_communication();
  return 0;
}
