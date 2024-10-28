#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <dirent.h>


#include <fuse.h>

/* Returns correct socketfd on success or -1 otherwise */
int connectToServer(const char *ip, const int port);
/* Returns not null char *data recieved from server or NULL on error */
char *sendReqAndHandleResp(const int connection, const char *buffer, const int buffsize);
