#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>

#define PATH_MAX 4096
#define PORT 12345

int connection; // Simulating the connection for the test

// Mock server to simulate read and write operations
int mock_server() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error creating socket");
        return -1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        return -1;
    }

    // Listen for connections
    listen(server_sock, 5);

    printf("Mock server listening on port %d...\n", PORT);

    // Accept incoming connection
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("Error accepting connection");
        return -1;
    }

    connection = client_sock; // Store connection for use in client-side functions

    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int received = recv(client_sock, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            perror("Error receiving data or connection closed");
            break;
        }

        printf("Received request: %s\n", buffer);

        // Mock response for "READ" request
        if (strncmp(buffer, "READ", 4) == 0) {
            // Example response: "Hello from mock server"
            const char *mock_response = "Hello from mock server";
            send(client_sock, mock_response, strlen(mock_response), 0);
        }
        // Mock response for "WRITE" request
        else if (strncmp(buffer, "WRITE", 5) == 0) {
            const char *mock_ack = "ACK";
            send(client_sock, mock_ack, strlen(mock_ack), 0);
        }
    }

    close(client_sock);
    close(server_sock);

    return 0;
}

// Simulate pxfs_read
static int pxfs_read(const char *path,
                     char *buf,
                     size_t size,
                     off_t off) {
    // Send read request to server
    char request[PATH_MAX + 100];
    snprintf(request, sizeof(request), "READ %s %zu %ld", path, size, off);
    if (send(connection, request, strlen(request), 0) < 0) {
        return -1;
    }

    // Receive data from server
    ssize_t bytes_read = recv(connection, buf, size, 0);
    if (bytes_read < 0) {
        return -1;
    }

    // Log the read operation
    printf("READ operation: %s\n", buf);
    return bytes_read;
}

// Simulate pxfs_write
static int pxfs_write(const char *path,
                      const char *buf,
                      size_t size,
                      off_t off) {
    // Send write request to server
    char request[PATH_MAX + 100];
    snprintf(request, sizeof(request), "WRITE %s %zu %ld", path, size, off);
    if (send(connection, request, strlen(request), 0) < 0) {
        return -1;
    }

    // Send the actual data
    ssize_t bytes_written = send(connection, buf, size, 0);
    if (bytes_written < 0) {
        return -1;
    }

    // Wait for server acknowledgment
    char ack[10];
    if (recv(connection, ack, sizeof(ack), 0) < 0) {
        return -1;
    }

    // Log the write operation
    printf("WRITE operation: %s\n", buf);
    return bytes_written;
}

// Main function to test both read and write functions
int main() {
    // Start the mock server in a separate process or thread
    if (fork() == 0) {
        mock_server();
        exit(0); // End the child process after the server starts
    }

    // Allow some time for the mock server to start
    for (int i = 0; i < 5; i++) {
        sleep(1);
        connection = socket(AF_INET, SOCK_STREAM, 0);
        if (connection < 0) continue;

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(connection, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            printf("Connected to server\n");
            break;
        }
        close(connection);
    }


    // Test pxfs_read
    char read_buffer[1024];
    const char *read_path = "/example/path";
    size_t read_size = 256;
    off_t read_offset = 0;

    int bytes_read = pxfs_read(read_path, read_buffer, read_size, read_offset);
    if (bytes_read >= 0) {
        printf("Read successful, %d bytes: %s\n", bytes_read, read_buffer);
    } else {
        printf("Read failed with error code %d\n", bytes_read);
    }

    // Test pxfs_write
    const char *write_path = "/example/path";
    const char *write_data = "Sample data to write to server.";
    size_t write_size = strlen(write_data);
    off_t write_offset = 0;

    int bytes_written = pxfs_write(write_path, write_data, write_size, write_offset);
    if (bytes_written >= 0) {
        printf("Write successful, %d bytes: %s\n", bytes_written, write_data);
    } else {
        printf("Write failed with error code %d\n", bytes_written);
    }

    return 0;
}
