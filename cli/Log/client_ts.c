#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logger.h"

// Function to connect to the server
int connectToServer(const char *ip, const int port) {
    int connection = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (connection == -1) {
        log_error("Cannot create socket");
        return -1;
    }

    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr)); // Clear the sockaddr structure

    sockaddr.sin_family = AF_INET; // Set the address family to IPv4
    sockaddr.sin_port = port == 0 ? htons(88) : htons(port); // Set the port
    sockaddr.sin_addr.s_addr = inet_addr(ip); // Convert IP address from text to binary form

    // Attempt to connect to the server
    if (connect(connection, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        log_error("Connection failed");
        close(connection);
        return -1;
    }

    log_message("Successfully connected to server");
    return connection; // Return the connection file descriptor
}

// Function to send a request and handle the response
char *sendReqAndHandleResp(const int connection, const char *buffer, const int buffsize) {
    // Send data to the server
    if (send(connection, buffer, buffsize, 0) == -1) {
        log_error("Sending packet troubles");
        return NULL;
    }

    // Receive the size of the incoming data
    int64_t packageSize;
    if (recv(connection, &packageSize, sizeof(packageSize), 0) != sizeof(packageSize)) {
        log_error("Troubles in packSize reading");
        return NULL;
    }

    // Allocate memory for the response
    char *responseBuffer = malloc(packageSize);
    if (responseBuffer == NULL) {
        log_error("NULL pointer");
        return NULL;    
    }

    // Receive the actual data
    if (recv(connection, responseBuffer, packageSize, 0) < packageSize) {
        log_error("Cannot recv to buffer");
        free(responseBuffer);
        return NULL;
    }

    log_message("Data received successfully");
    return responseBuffer;
}

//Here I just want the feedback from the terminal(test)
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    // Connect to the server
    int connection = connectToServer(ip, port);
    if (connection == -1) {
        exit(EXIT_FAILURE);
    }

    // Example request to send to the server
    const char *request = "Hello, Server!";
    char *response = sendReqAndHandleResp(connection, request, strlen(request));
    
    if (response != NULL) {
        printf("Response from server: %s\n", response);
        free(response);
    }

    close(connection);
    return 0;
}