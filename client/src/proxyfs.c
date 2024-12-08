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
#include <stdlib.h>
#include <stddef.h>
#include <fuse.h>

#ifndef AT_EMPTY_PATH
#	define AT_EMPTY_PATH 0x1000
#endif

/* CLIENT-SERVER COMMUNICATION */
#include "client.h"
/* 
	Static var that contains descriptor of socket with server.
	Defines by function from client.h
*/
#include "diff.h"
static int connection;



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

static int pxfs_open(const char *name, struct fuse_file_info *fi)
{
	int fd;

	fd = openat((int)(intptr_t)(fuse_get_context()->private_data),
	            &name[1],
	            fi->flags);
	if (fd < 0)
		return -errno;

	fi->fh = (uint64_t)fd;

	return 0;
}

static int pxfs_create(const char *name,
					   mode_t mode,
					   struct fuse_file_info *fi)
{
	const struct fuse_context *ctx;
	int dirfd, fd, err;

	ctx = fuse_get_context();
	dirfd = (int)(intptr_t)ctx->private_data;

	fd = openat(dirfd, &name[1], fi->flags | O_CREAT | O_EXCL, mode);
	if (fd < 0)
		return -errno;

	if (fchown(fd, ctx->uid, ctx->gid) < 0) {
		err = -errno;
		close(fd);
		return err;
	}

	fi->fh = (uint64_t)fd;

	char initial_content[MAX_FILE_CONTENT_LENGTH] = {0};
	PseudoFile* file = create_pseudo_file(name, initial_content);
	if (file == NULL) {
		close(fd);
		return -ENOMEM;
	}
	// EDIT!!
	log("CREATE", name);

	return 0;
}

static int pxfs_close(const char *name, struct fuse_file_info *fi)
{
	if (close((int)fi->fh))
		return -errno;

	// EDIT
	log("CLOSE", name);

	return 0;
}

#if FUSE_USE_VERSION < 30
static int pxfs_truncate(const char *name, off_t size)
#else
static int pxfs_truncate(const char *name,
                         off_t size,
                         struct fuse_file_info *fi)
#endif
{
	int fd, err = 0;

#if FUSE_USE_VERSION >= 30
	if (fi)
		fd = (int)fi->fh;
	else {
#endif
		fd = openat((int)(intptr_t)(fuse_get_context()->private_data),
		            &name[1],
		            O_WRONLY | O_CREAT);
		if (fd < 0)
			return -errno;
#if FUSE_USE_VERSION >= 30
	}
#endif

	if (ftruncate(fd, size) < 0)
		err = -errno;

#if FUSE_USE_VERSION >= 30
	if (!fi)
		close(fd);
#endif

	return err;
}

static int pxfs_getattr(const char *name,
#if FUSE_USE_VERSION < 30
                        struct stat *stbuf)
#else
                        struct stat *stbuf,
                        struct fuse_file_info *fi)
#endif
{
	int ret;

#if FUSE_USE_VERSION >= 30
	if (fi) {
		// local
		ret = fstat((int)fi->fh, stbuf);
		if (ret < 0)
			return -errno;
	} else {
		char *response = NULL;

		// Prepare the request to get an attribute
		char request[1024];
		//type of request
		request[0] = 3;
		//path length
		int path_length = strlen(name);
		if (path_length + 5 > sizeof(request)) {
			perror("Path length exceeds buffer size");
			return -ENAMETOOLONG;
		}
		uint32_t net_path_length = htonl(path_length);
		memcpy(&request[1], &net_path_length, 4);
		//path
		memcpy(&request[5], name, path_length);

		// Send the request and receive the response
		response = sendReqAndHandleResp(connection, request, path_length + 5);
		if (response == NULL) {
			perror("Error of function sendReqAndHandleResp");
			return -EIO;
		}

		// Process the response and fill the stat structure
		int file_exists = response[0];
		if (file_exists == 0) {
			// File exists, parse the stat structure
			uint32_t struct_size = ntohl(*(uint32_t *)&response[1]);
			// Fill the stat structure with mode, nlink, and size
			stbuf->st_nlink = 1;
			stbuf->st_mode = ntohl(*(uint32_t *)&response[5]);
			stbuf->st_size = ntohl(*(uint32_t *)&response[9]);
		} else if (file_exists == 1) {
			// file does not exist
			uint32_t error_length = ntohl(*(uint32_t *)&response[1]);
			memcpy(stbuf, &response[5], error_length);
			free(response);
			return -ENOENT;
		}

		free(response);
		return 0;
#endif
	ret = fstatat((int)(intptr_t)(fuse_get_context()->private_data),
	              &name[1],
	              stbuf,
	              AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);

	if (ret < 0)
		return -errno;

	return 0;
}

static int pxfs_access(const char *name, int mask)
{
	const char *namep = name;

	if ((name[0] != '/') || (name[1] != '\0'))
		++namep;

	if (faccessat((int)(intptr_t)(fuse_get_context()->private_data),
	              namep,
	              mask,
	              AT_SYMLINK_NOFOLLOW) < 0)
		return -errno;

	return 0;
}

static int pxfs_read(const char *path,
                     char *buf,
                     size_t size,
                     off_t off,
                     struct fuse_file_info *fi)
{
	ssize_t out;

	out = pread((int)fi->fh, buf, size > INT_MAX ? INT_MAX : size, off);
	if (out < 0)
		return -errno;

	// EDTIT
	log_buf("READ", buf, size, path);

	return (int)out;
}

static int pxfs_write(const char *path,
                      const char *buf,
					  size_t size,
					  off_t off,
					  struct fuse_file_info *fi)
{
  		ssize_t out;
		//out = pwrite((int)fi->fh, buf, size > INT_MAX ? INT_MAX : size, off);
		if (out < 0)
			return -errno;

		// Обновление псевдофайла
		PseudoFile* file = find_file(path);
		if (file != NULL) {
			update_pseudo_file(file, buf);
		}
		// EDIT!!!
		log_buf("WRITE", buf, size, path);

		return (int)out;
}

static int pxfs_unlink(const char *name)
{
	if (unlinkat((int)(intptr_t)(fuse_get_context()->private_data),
	             &name[1],
	             0) < 0)
		return -errno;

	return 0;
}

static int pxfs_mkdir(const char *name, mode_t mode)
{
	const struct fuse_context *ctx;
	int dirfd, err;

	ctx = fuse_get_context();
	dirfd = (int)(intptr_t)ctx->private_data;

	if (mkdirat(dirfd, &name[1], mode) < 0)
		return -errno;

	if (fchownat(dirfd,
	             &name[1],
	             ctx->uid,
	             ctx->gid,
	             AT_SYMLINK_NOFOLLOW) < 0) {
		err = -errno;
		unlinkat(dirfd, &name[1], AT_REMOVEDIR);
		return err;
	}

	return 0;
}

static int pxfs_rmdir(const char *name)
{
	if (unlinkat((int)(intptr_t)(fuse_get_context()->private_data),
	             &name[1],
	             AT_REMOVEDIR) < 0)
		return -errno;

	return 0;
}

static int pxfs_opendir(const char *name, struct fuse_file_info *fi)
{
	DIR *dirp;
	int fd, err, dirfd = (int)(intptr_t)(fuse_get_context()->private_data);

	if ((name[0] == '/') && (name[1] == '\0'))
		fd = dup(dirfd);
	else
		fd = openat(dirfd, &name[1], O_DIRECTORY);

	if (fd < 0)
		return -errno;

	dirp = fdopendir(fd);
	if (!dirp) {
		err = -errno;
		close(fd);
		return err;
	}

	fi->fh = (uint64_t)(uintptr_t)dirp;
	return 0;
}

static int pxfs_closedir(const char *name, struct fuse_file_info *fi)
{
	if (closedir((DIR *)(uintptr_t)fi->fh) < 0)
		return -errno;

	return 0;
}

static int pxfs_readdir(const char *path,
                        void *buf,
                        fuse_fill_dir_t filler,
                        off_t offset,
#if FUSE_USE_VERSION < 30
                        struct fuse_file_info *fi)
#else
                        struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags)
#endif
{
    struct stat stbuf;
    struct dirent ent, *pent;
    DIR *dirp;
    int dirfd;

#if FUSE_USE_VERSION >= 30
    if (fi) {
        // local readdir
        dirp = (DIR *)(uintptr_t)fi->fh;
        dirfd = (int)(intptr_t)(fuse_get_context()->private_data);

        if (offset == 0)
            rewinddir(dirp);

        do {
            if (readdir_r(dirp, &ent, &pent) != 0)
                return -errno;

            if (!pent)
                break;

            if (fstatat(dirfd,
                        pent->d_name,
                        &stbuf,
                        AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW) < 0)
                return -errno;

                return -ENOMEM;
        } while (1);
    } else {
        // prepare a request to read the directory
        char request[1024];
        request[0] = 2;
        int path_length = strlen(path);
        memcpy(request + 1, &path_length, 4);
        memcpy(request + 5, path, path_length);

        // send the request and receive the response
        char *response = sendReqAndHandleResp(connection, request, path_length + 5);
        if (response == NULL) {
            perror("Error of function sendReqAndHandleResp");
            return -EIO;
        }

        // process the response and fill the stat structure
        int file_exists = response[0];
        if (file_exists == 0) {
            // file exists
            uint32_t names_count = ntohl(*(uint32_t *)&response[1]);
            int offset = 5;
	    // listing files
            for (uint32_t i = 0; i < names_count; i++) {
                uint8_t name_length = response[offset];
                char name[name_length + 1];
                memcpy(name, &response[offset + 1], name_length);
                name[name_length] = '\0';
                printf("Name: %s\n", name);
                offset += 1 + name_length;
            }
        } else if (file_exists == 1){
            // file does not exist
            uint32_t error_length = ntohl(*(uint32_t *)&response[1]);
            char error_message[error_length + 1];
            memcpy(error_message, &response[5], error_length);
            error_message[error_length] = '\0';
            printf("Error: %s\n", error_message);
        }

        // free the allocated memory
        free(response);

        return 0;
    }
#else
    dirp = (DIR *)(uintptr_t)fi->fh;
    dirfd = (int)(intptr_t)(fuse_get_context()->private_data);

    if (offset == 0)
        rewinddir(dirp);

    do {
        if (readdir_r(dirp, &ent, &pent) != 0)
            return -errno;

        if (!pent)
            break;

        if (fstatat(dirfd,
                    pent->d_name,
                    &stbuf,
                    AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW) < 0)
            return -errno;

#if FUSE_USE_VERSION < 30
        if (filler(buf, pent->d_name, &stbuf, 0) == 1)
#else
        if (filler(buf, pent->d_name, &stbuf, 0, flags) == 1)
#endif
            return -ENOMEM;
    } while (1);
#endif

    return 0;
}

static int pxfs_symlink(const char *to, const char *from)
{
	const struct fuse_context *ctx;
	int dirfd, err;

	ctx = fuse_get_context();
	dirfd = (int)(intptr_t)ctx->private_data;

	if (symlinkat(to, dirfd, &from[1]) < 0)
		return -errno;

	if (fchownat(dirfd,
	             &from[1],
	             ctx->uid,
	             ctx->gid,
	             AT_SYMLINK_NOFOLLOW) < 0) {
		err = -errno;
		unlinkat(dirfd, &from[1], AT_REMOVEDIR);
		return err;
	}

	return 0;
}

static int pxfs_readlink(const char *name, char *buf, size_t size)
{
	ssize_t len;

	len = readlinkat((int)(intptr_t)(fuse_get_context()->private_data),
	                 &name[1],
	                 buf,
	                 size - 1);
	if (len < 0)
		return -errno;

	buf[len] = '\0';
	return 0;
}

static int pxfs_mknod(const char *name, mode_t mode, dev_t dev)
{
	if (mknodat((int)(intptr_t)(fuse_get_context()->private_data),
	            &name[1],
	            mode,
	            dev) < 0)
		return -errno;

	return 0;
}

#if FUSE_USE_VERSION < 30
static int pxfs_chmod(const char *name, mode_t mode)
#else
static int pxfs_chmod(const char *name, mode_t mode, struct fuse_file_info *fi)
#endif
{
	int ret;

#if FUSE_USE_VERSION >= 30
	if (fi)
		ret = fchmod((int)fi->fh, mode);
	else
#endif
		ret = fchmodat((int)(intptr_t)(fuse_get_context()->private_data),
	                   &name[1],
	                   mode,
	                   AT_SYMLINK_NOFOLLOW);
	if (ret < 0)
		return -errno;

	return 0;
}

static int pxfs_chown(const char *name,
                      uid_t uid,
#if FUSE_USE_VERSION < 30
                      gid_t gid)
#else
                      gid_t gid,
                      struct fuse_file_info *fi)
#endif
{
	int ret;

#if FUSE_USE_VERSION >= 30
	if (fi)
		ret = fchown((int)fi->fh, uid, gid);
	else
#endif
		ret = fchownat((int)(intptr_t)(fuse_get_context()->private_data),
		             &name[1],
		             uid,
		             gid,
		             AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);

	if (ret < 0)
		return -errno;

	return 0;
}

static int pxfs_utimens(const char *name,
#if FUSE_USE_VERSION < 30
                        const struct timespec tv[2])
#else
                        const struct timespec tv[2],
                        struct fuse_file_info *fi)
#endif
{
	int ret;

#if FUSE_USE_VERSION >= 30
	if (fi)
		ret = futimens((int)fi->fh, tv);
	else
#endif
		ret = utimensat((int)(intptr_t)(fuse_get_context()->private_data),
		                &name[1],
		                tv,
		                AT_SYMLINK_NOFOLLOW);

	if (ret < 0)
		return -errno;

	return 0;
}

static int pxfs_rename(const char *oldpath,
#if FUSE_USE_VERSION < 30
                       const char *newpath)
#else
                       const char *newpath,
                       unsigned int flags) /* XXX: handle RENAME_NOREPLACE */
#endif
{
	int dirfd = (int)(intptr_t)(fuse_get_context()->private_data);

	if (renameat(dirfd, &oldpath[1], dirfd, &newpath[1]) < 0)
		return -errno;

	return 0;
}

struct fuse_operations pxfs_oper = {
	.open		= pxfs_open,
	.create		= pxfs_create,
	.release	= pxfs_close,

	.truncate	= pxfs_truncate,

	.read		= pxfs_read,
	.write		= pxfs_write,

	.getattr	= pxfs_getattr,
	.access		= pxfs_access,

	.unlink		= pxfs_unlink,

	.mkdir		= pxfs_mkdir,
	.rmdir		= pxfs_rmdir,

	.opendir	= pxfs_opendir,
	.releasedir	= pxfs_closedir,
	.readdir	= pxfs_readdir,

	.symlink	= pxfs_symlink,
	.readlink	= pxfs_readlink,

	.mknod		= pxfs_mknod,

	.chmod		= pxfs_chmod,
	.chown		= pxfs_chown,
	.utimens	= pxfs_utimens,
	.rename		= pxfs_rename
};

#define DEFAULT_PATH_TO_VCS_DIR "vcs"

extern struct fuse_operations pxfs_oper;

int main(int argc, char *argv[])
{
	char *fuse_argv[4];
	int dirfd, ret;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s TARGET\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    if (mkdir(DEFAULT_PATH_TO_VCS_DIR, 0777) != 0)
        exit(EXIT_FAILURE);

	dirfd = open(".", O_DIRECTORY);
	if (dirfd < 0)
		exit(EXIT_FAILURE);

	fuse_argv[0] = argv[0];
	fuse_argv[1] = argv[1];
#if FUSE_USE_VERSION < 30
	fuse_argv[2] = "-ononempty,suid,dev,allow_other,default_permissions";
#else
	fuse_argv[2] = "-osuid,dev,allow_other,default_permissions";
#endif
	fuse_argv[3] = NULL;

	/* 
	TODO:
	1. ESTABLISH CONNECTION WITH SERVER 
	2. LOAD FILES FROM SERVER
	3. CREATE DIFF FILE

	!! ALL perror()'s should be inside functions from client.h !!
	!! Handle errors !!

	!!!! In process of creating this functions you can create auxiliary functions
		CREATE THEM !!!!
	*/
	//1. ESTABLISH CONNECTION WITH SERVER 
	connection = connectToServer(getenv("SERVER_IP"), atoi(getenv("SERVER_PORT")));
	if (connection == -1) 
		exit(EXIT_FAILURE);

	//3. CREATE DIFF FILE
	// пу пу пууууу


	ret = fuse_main(3, fuse_argv, &pxfs_oper, (void *)(intptr_t)dirfd);

	close(dirfd);
	exit(ret);
}