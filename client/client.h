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

int connectToServer(char *buffer, int buffSize); //should we paste there buffSize???
int getRepoStructure(char *buff, int buffSize); //args?? maybe it will contain `char *buff`
