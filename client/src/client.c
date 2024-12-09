#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int connectToServer(const char *ip, const int port) {
    int connection = socket(AF_INET, SOCK_STREAM, 6);
    if (connection == -1) {
        perror("Cannot create socket");
        return -1;
    }

    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = port == 0 ? 88 : htons(port);

    int ipAddr= inet_pton(AF_INET, ip == NULL ? "127.0.0.1" : ip, &sockaddr);


    switch (ipAddr) {
        case 1:
            return connection;
        case 0:
            perror("Invalid address");
            break;
        case -1:
            perror("Invalid AF");
            break;
    }

    close(connection);
    return -1;
}

char *sendReqAndHandleResp(const int connection, const char *dataToSend, const int buffsize) {
    // Send data
    if (send(connection, dataToSend, buffsize, 0) == -1) {
        perror("Sending packet troubles");
        return NULL;
    }

    // Recieve data
    int64_t packageSize;
    if (recv(connection, &packageSize, 8, 0) != 8) {
        perror("Troubles in packSize reading");
        return NULL;
    }

    char *recvBuffer = malloc(packageSize);
    if (recvBuffer == NULL) {
        perror("NULL pointer");
        return NULL;
    }

    if (recv(connection, recvBuffer, packageSize, 0) < packageSize) {
        perror("Cannot recv to buffer");
        free(recvBuffer);
        return NULL;
    }

    return recvBuffer;
}