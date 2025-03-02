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

#include "utils.h"

extern char *mountpoint;

void log(char *msg_type, char *name) {
	FILE *log_file = fopen("pxfs.log", "a+");
	if (log_file != NULL) {
		fprintf(log_file, "%s: %s\n", msg_type, name);
		fclose(log_file);
	}
}

void log_buf(char *msg_type, char *name, int size, char *buf) {
	FILE *log_file = fopen("pxfs.log", "a+");
	if (log_file != NULL) {
		fprintf(log_file, "%s buf %s of size %d in file %s\n", msg_type, buf, size, name);
		fclose(log_file);
	}
}

RelativePath *getRelativePath(char *path) {
    int i, pathlength, j;
    for (i = 0; mountpoint[i] != '\000'; i++) {
        if (path[i] == '\000')
            return NULL;
        if (path[i] != mountpoint[i])
            return NULL;
    }
    for (j = i; path[j] != '\000'; j++);
    pathlength = j;
    
    char *relpath = malloc(pathlength + 3);
    if (relpath == NULL) 
        return NULL;

    relpath[0] = '.';
    relpath[1] = '/';
    for (j = 0; j < pathlength; j++) 
        relpath[j + 2] = path[i + j];
    
    relpath[j + 2] = '\000';

    RelativePath *relativePath = malloc(sizeof(RelativePath));
    if (relativePath == NULL) 
        return NULL;
    relativePath->path = relpath;
    relativePath->length = j + 2 + 1;

    return relativePath;
}