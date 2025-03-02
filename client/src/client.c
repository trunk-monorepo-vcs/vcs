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

#define PACKAGE_SIZE 1024


int connectToServer(const char *ip, const int port) {
    int connection = socket(AF_INET, SOCK_STREAM, 6);
    if (connection == -1) {
        perror("Cannot create socket");
        return -1;
    }

    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip == NULL ? "127.0.0.1" : ip, &sockaddr.sin_addr) != 1) {
        perror("Gotta some troubles");
    } else {
        if (connect(connection, (struct sockaddr*)&sockaddr, sizeof(sockaddr) == -1)) {
            perror("Gotta some troubles :( with connection");
            printf("ip: %s ; port: %d\n", ip, port);
        } else {
            printf("Connect with succes!!\n");
            return connection;
        }
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

    // // Recieve data
    // int64_t packageSize;
    // if (recv(connection, &packageSize, 8, 0) != 8) {
    //     perror("Troubles in packSize reading");
    //     return NULL;
    // }
    uint64_t packageSize = PACKAGE_SIZE;
    uint64_t readedBytes, lastIndex = 0;
    char *recvBuffer = malloc(packageSize);
    if (recvBuffer == NULL) {
        perror("NULL pointer");
        return NULL;
    }
    do {
        readedBytes = recv(connection, (char *)((uint64_t)recvBuffer + lastIndex), packageSize, 0);
        lastIndex += readedBytes;
        if (readedBytes == packageSize) {
            recvBuffer = realloc(recvBuffer, packageSize * 2);
            if (recvBuffer == NULL) {
                perror("NULL pointer");
                return NULL;
            }
        }
    } while (readedBytes);

    // if (recv(connection, recvBuffer, packageSize, 0) < packageSize) {
    //     perror("Cannot recv to buffer");
    //     free(recvBuffer);
    //     return NULL;
    // }

    return recvBuffer;
}