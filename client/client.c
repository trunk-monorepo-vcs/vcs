#include <sys/socket.h>

int connectToServer(char *buffer, int buffSize) {
    int connection = socket(AF_INET6, SOCK_STREAM, 0);
    /* -> -> -> */
}