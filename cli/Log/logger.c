#include <stdio.h>
#include <time.h>
#include "logger.h"

void log_message(const char *msg) {
    FILE *log_file = fopen("client.log", "a+");
    if (log_file != NULL) {
        time_t now = time(NULL);
        fprintf(log_file, "[%s] INFO: %s\n", ctime(&now), msg);
        fclose(log_file);
    }
}

void log_error(const char *msg) {
    FILE *log_file = fopen("client.log", "a+");
    if (log_file != NULL) {
        time_t now = time(NULL);
        fprintf(log_file, "[%s] ERROR: %s\n", ctime(&now), msg);
        fclose(log_file);
    }
}