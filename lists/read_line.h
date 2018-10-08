#include <unistd.h>

int send_msg(int socket, char *message, int length);
int recv_msg(int socket, char *message, int length);
ssize_t readLine(int fd, void *buffer, size_t n);
