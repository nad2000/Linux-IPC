#ifndef FILE_COMMON_SEEN
#define FILE_COMMON_SEEN

#define SERVER_PORT 2000
#define errExit(msg, ...)                                                      \
  do {                                                                         \
    perror(msg "\n", ##__VA_ARGS__);                                           \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
#define errExit(msg)                                                           \
  do {                                                                         \
    perror(msg "\n");                                                          \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#endif /* !FILE_COMMON_SEEN */
