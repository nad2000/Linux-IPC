#ifndef FILE_COMMON_SEEN
#define FILE_COMMON_SEEN

#define SERVER_PORT 2000
#define SERVER_IP_ADDRESS "127.0.0.1"

#define errExit(...)                                                           \
  do {                                                                         \
    char output[1024];                                                         \
    sprintf(output, __VA_ARGS__);                                              \
    perror(output);                                                            \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
#define errExit0(msg)                                                          \
  do {                                                                         \
    perror(msg "\n");                                                          \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#pragma pack(1)
typedef struct _request {
  unsigned int a, b;
} request_t;

typedef struct _result {
  unsigned int c;
} result_t;
#pragma pack(0)

#endif /* !FILE_COMMON_SEEN */
