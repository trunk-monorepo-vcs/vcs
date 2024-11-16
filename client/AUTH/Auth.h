#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <time.h>

// Authentication result
typedef enum {
    AUTH_SUCCESS = 0,          // Successful login
    AUTH_ERROR = -1            // Failed login
} auth_status_t;

// User credentials structure
typedef struct {
    char *username;            // Username
    char *token;               // Authentication token
    time_t token_expire;       // Token expiration time
} auth_credentials_t;

// Function declarations
auth_status_t auth_login(const char *username, const char *password, auth_credentials_t **credentials);
void auth_logout(auth_credentials_t *credentials);
bool auth_is_token_valid(const auth_credentials_t *credentials);
void auth_free_credentials(auth_credentials_t *credentials);

#endif // AUTH_H
