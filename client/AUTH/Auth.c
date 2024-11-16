#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "auth.h"

// Path to the mock server response file
#define MOCK_SERVER_RESPONSE_FILE "mock_server_response.json"

// Reads the content of a file into memory
static char *read_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file); // Get file size
    rewind(file);

    char *content = malloc(length + 1); // Allocate memory for file content
    if (!content) {
        fclose(file);
        return NULL;
    }

    fread(content, 1, length, file);
    content[length] = '\0'; // Null-terminate the string

    fclose(file);
    return content;
}

// Extracts the value of a specific key from a JSON-like string
static char *parse_json_key(const char *json, const char *key) {
    char *start = strstr(json, key);
    if (!start) return NULL;

    start = strchr(start, ':');
    if (!start) return NULL;

    start = strchr(start, '"');
    if (!start) return NULL;

    char *end = strchr(start + 1, '"');
    if (!end) return NULL;

    size_t len = end - start - 1; // Compute length of the value
    char *value = malloc(len + 1);
    strncpy(value, start + 1, len);
    value[len] = '\0'; // Null-terminate the value

    return value;
}

// Simulates login by reading a mock server response
auth_status_t auth_login(const char *username, const char *password, auth_credentials_t **credentials) {
    char *response = read_file(MOCK_SERVER_RESPONSE_FILE); // Read mock server response
    if (!response) {
        return AUTH_ERROR;
    }

    char *token = parse_json_key(response, "token"); // Extract token from response
    if (!token) {
        free(response);
        return AUTH_ERROR;
    }

    *credentials = malloc(sizeof(auth_credentials_t));
    (*credentials)->username = strdup(username);  // Store username
    (*credentials)->token = token;               // Store token
    (*credentials)->token_expire = time(NULL) + 24 * 60 * 60; // Set token expiration (24 hours)

    free(response); // Free response memory
    return AUTH_SUCCESS;
}

// Simulates logout
void auth_logout(auth_credentials_t *credentials) {
    if (!credentials) return;

    printf("User %s logged out.\n", credentials->username);
}

// Checks if the token is still valid
bool auth_is_token_valid(const auth_credentials_t *credentials) {
    return credentials && time(NULL) < credentials->token_expire;
}

// Frees memory allocated for credentials
void auth_free_credentials(auth_credentials_t *credentials) {
    if (credentials) {
        free(credentials->username);
        free(credentials->token);
        free(credentials);
    }
}
