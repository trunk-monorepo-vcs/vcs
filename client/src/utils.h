#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>

typedef struct {
    char *path;
    int length;
} RelativePath;

void log(char *msg_type, char *name);
void log_buf(char *msg_type, char *name, int size, char *buf);

RelativePath *getRelativePath(char *path)