#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "server.h"

#if 0
#define _DEBUG_1_
#endif


return_type r;
char *ROOT_PATH;
struct fsDirent dent;
struct file_status *file_status_list_head = NULL;

/**
 * Returns the successor of a file_status object
 *
 * @param node predecessor object
 * @return     successor of object passed in
 */
void *file_status_next(void *node) {
    return (void *)((struct file_status *)node)->next;
}

/**
 * Sets connects two file_status elements. "next" will
 * be the successor to "node"
 *
 * @param node predecessor elements
 * @param node successor elements
 */
void file_status_set_next(void *node, void *next) {
    ((struct file_status *)node)->next = (struct file_status *)next;
}

/**
 * Inserts an element into the file_status linked list.
 *
 * @param entry element to insert into the list
 */
void file_status_list_insert(struct file_status *entry) {
    list_insert((void **)&file_status_list_head, (void *)entry, file_status_next, file_status_set_next);
}

/**
 * Remove a file_status element from the linked list
 *
 * @param entry element to be removed
 */
void file_status_list_delete(struct file_status *entry) {
    list_delete((void **)&file_status_list_head, (void *)entry, file_status_next, file_status_set_next);
}

/**
 * Returns size of the file_status linked list.
 *
 * @return size of the file_status linked list
 */
int file_status_list_size() {
    return list_size((void *)file_status_list_head, file_status_next);
}

/**
 * Searches for a file_status element with given path.
 *
 * @param fpath path to search for
 * @return      matching file_status element
 */
struct file_status *file_status_list_find_by_path(char *fpath) {
    return file_status_list_find((void *)fpath, 1);
}

/**
 * Searches for a file_status element with given fd.
 *
 * @param fd file descriptor to search for
 * @return   matching file_status element
 */
struct file_status *file_status_list_find_by_fd(int fd) {
    return file_status_list_find((void *)&fd, 2);
}

/**
 * Searches for a file_status element with given alias fd.
 *
 * @param fd alias file descriptor to search for
 * @return   matching file_status element
 */
struct file_status *file_status_list_find_by_alias_fd(int fd) {
    return file_status_list_find((void *)&fd, 3);
}

/**
 * Searches for a file_status element by either path of
 * file descriptor. If mode is 1, it searches for a matching path.
 * If mode is 2, it searches by file descriptor.
 *
 * @param crit criteria to search for
 * @param mode criteria mode
 * @return     matching file_status element
 */
struct file_status *file_status_list_find(void *crit, int mode) {
    struct file_status *current = file_status_list_head;
    for(; current != NULL; current = current->next) {
        if (mode == 1 && strcmp((char *)crit, current->path) == 0)
            return current;
        if (mode == 2 && *(int *)crit == current->fd)
            return current;
        if (mode == 3 && *(int *)crit == current->aliasfd)
            return current;
    }
    return NULL;
}

/**
 * Check if a file is open.
 *
 * @param fpath path of the file to check
 * @return      true if the file is open, false otherwise
 */
bool is_file_open(char *fpath) {
    return file_status_list_find_by_path(fpath) != NULL;
}

/**
 * Returns the real file descriptor for an alias one.
 *
 * @param aliasfd file descriptor
 * @return        real file descriptor
 */
int fd_for_alias(int aliasfd) {
    struct file_status *stat;
    if ((stat = file_status_list_find_by_alias_fd(aliasfd)) != NULL) {
        return stat->fd;
    }
    return -1;
}

/**
 * Checks that a file exists.
 *
 * @param local_folder_name name of the folder
 * @return                  0 on success, −1 on failure
 */
int _fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *local_folder_name) {
    struct stat sbuf;
    return stat(local_folder_name, &sbuf);
}

/**
 * Unmounts a remote filesystem that is referred to locally by local_folder_name.
 * Returns 0 on success, −1.
 *
 * @param local_folder_name name of the mounted folder
 * @return                  0 on success, −1 on failure
 */
int _fsUnmount(const char *local_folder_name) {
    return 0;
}

/**
 * Opens the folder.
 *
 * @param folder_name name of the directory
 * @return            struct containing details about the directory, NULL if failure
 */
DIR* _fsOpenDir(const char *folder_name) {
    return (opendir(folder_name));
}

/**
 * Closes a directory.
 *
 * @param folder DIR of the folder to be closed.
 * @return       0 on success, −1 on failure
 */
int _fsCloseDir(DIR *folder) {
    return(closedir(folder));
}

/**
 * Reads a directory. Returns NULL of an error occured.
 *
 * @param folder details of the directory to be read
 * @return       fsDirent of the directory to be read.
 */
struct fsDirent *_fsReadDir(DIR *folder) {
    const int initErrno = errno;
    struct dirent *d = readdir(folder);

    if(d == NULL) {
    if(errno == initErrno) errno = 0;
        return NULL;
    }

    if(d->d_type == DT_DIR) {
        dent.entType = 1;
    }
    else if(d->d_type == DT_REG) {
        dent.entType = 0;
    }
    else {
        dent.entType = -1;
    }

    memcpy(&(dent.entName), &(d->d_name), 256);
    return &dent;
}

/**
 * Opens a file whose path is fname. The mode is one of two values: 0 for read,
 * and 1 for write. Returns a file descriptor that can be used in future calls for
 * operations on this file.
 *
 * @param fname path of file to be opened
 * @param mode  read mode. 0 for read, and 1 for write
 * @return       a file descriptor of the file. -1 if failure
 */
int _fsOpen(const char *fname, int mode) {
    int flags = -1;

    if(mode == 0) {
        flags = O_RDONLY;
    }
    else if(mode == 1) {
        flags = O_WRONLY | O_CREAT;
    }
    return(open(fname, flags, S_IRWXU));
}

/**
 * Closes an open file.
 *
 * @param fd file descriptor of file to be closed
 * @return   0 on success, −1 on failure
 */
int _fsClose(int fd) {
    return(close(fd));
}

/**
 * Reads up to "count" bytes in to the supplied buffer buf from the file referred
 * to by the file descriptor fd, which was presumably the return from a call to
 * fsOpen() in read-mode.
 *
 * @param fd    file descriptor of file to be read
 * @param buf   buffer for data to be written to
 * @param count number of bytes to be read
 * @return      0 on success, −1 on failure
 */
int _fsRead(int fd, void *buf, const unsigned int count) {
    return(read(fd, buf, (size_t)count));
}

/**
 * Writes up to "count" bytes from buf into the file referred to with the file
 * descriptor fd that was presumably the return from an earlier fsOpen() call
 * in write-mode.
 *
 * @param fd    file descriptor of file to be written to
 * @param buf   buffer of data to be written
 * @param count number of bytes to be written
 * @return      0 on success, −1 on failure
 */
int _fsWrite(int fd, const void *buf, const unsigned int count) {
    return(write(fd, buf, (size_t)count));
}

/**
 * Removes (i.e., deletes) this file or folder from the server.
 *
 * @param name of file or folder to be removed
 * @return     0 on success, −1 on failure
 */
int _fsRemove(const char *name) {
    return(remove(name));
}

/**
 * RPC endpoint for fsMount.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return         RPC response
 */
return_type fsMount_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int mount_result = _fsMount((const char *) NULL, 0, (const char *)a->arg_val);

    fs_response *response = (fs_response *)malloc(sizeof(fs_response));
    response->in_error = 0;
    response->_errno = 0;
    *(int *)response->retval = 0;
    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsOpenDir.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return         RPC response
 */
return_type fsOpenDir_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    char *folderpath = (char *) a->arg_val;
    char *abspath = concat(ROOT_PATH, folderpath);
    //printf("%s\n", abspath);
    DIR* result = _fsOpenDir(abspath);
    free(abspath);

    fs_response *response = (fs_response *) malloc(sizeof(fs_response));
    response->in_error = result == NULL ? 1 : 0;
    response->_errno = errno;
    *(DIR **)response->retval = result;

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsCloseDir.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsCloseDir_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int result = _fsCloseDir(*(DIR **) a->arg_val);

    fs_response *response = (fs_response *) malloc(sizeof(fs_response));
    response->in_error = result < 0 ? 1 : 0;
    response->_errno = errno;
    *(int *)response->retval = result;

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsReadDir.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsReadDir_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    struct fsDirent *result = _fsReadDir(*(DIR **) a->arg_val);

    fs_response *response = (fs_response *) malloc(sizeof(fs_response));
    response->in_error = result == NULL ? 1 : 0;
    response->_errno = errno;
    if (result != NULL) {
        *(struct fsDirent *)response->retval = *result;
    }

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsOpen.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsOpen_rpc(const int nparams, arg_type* a) {
    if(nparams != 2) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }
    fs_response *response = (fs_response *) malloc(sizeof(fs_response));

    char *abspath = concat(ROOT_PATH, (char *)a->arg_val);
    int mode = *(int *)a->next->arg_val;
    printf("%s %d\n", abspath, is_file_open(abspath));
    if (is_file_open(abspath)) {
        response->in_error = 1;
        response->_errno = EACCES;
        *(int *)response->retval = FS_OPEN_WAIT_MSG;
    } else {
        int realfd = _fsOpen(abspath, mode);
        int aliasfd = realfd < 0 ? realfd : rand() % 1000;

        response->in_error = aliasfd < 0 ? 1 : 0;
        response->_errno = errno;
        *(int *)response->retval = aliasfd;

        if (!response->in_error) {
            struct file_status *entry = (struct file_status *)malloc(sizeof(struct file_status));
            strcpy(entry->path, abspath);
            entry->fd = realfd;
            entry->next = NULL;
            entry->mode = mode;
            entry->aliasfd = aliasfd;
            file_status_list_insert(entry);
        }
    }

    r.return_val = response;
    r.return_size = sizeof(fs_response);

    free(abspath);
    return r;
}

/**
 * RPC endpoint for fsClose.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsClose_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int fd = fd_for_alias(*(int *)a->arg_val);
    int result = _fsClose(fd);

    fs_response *response = (fs_response *) malloc(sizeof(fs_response));
    response->in_error = result < 0 ? 1 : 0;
    response->_errno = errno;
    *(int *)response->retval = result;

    struct file_status *stat;
    if ((stat = file_status_list_find_by_fd(fd)) != NULL) {
        file_status_list_delete(stat);
    }

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsRead.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsRead_rpc(const int nparams, arg_type* a) {
    if(nparams != 2) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int fd = fd_for_alias(*(int *)a->arg_val);
    int count = *(int *)a->next->arg_val;
    int return_size = (3 * sizeof(int)) + count;
    void *response = malloc(return_size);

    int bytesread = _fsRead(fd, response + (3 * sizeof(int)), count);
    *(int *)response = bytesread < 0;
    *(int *)(response + sizeof(int)) = errno;
    *(int *)(response + 2 * sizeof(int)) = bytesread;

    r.return_val = response;
    r.return_size = return_size;
    return r;
}

/**
 * RPC endpoint for fsWrite.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsWrite_rpc(const int nparams, arg_type* a) {
    if(nparams != 2) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    fs_response *response = (fs_response *)malloc(sizeof(fs_response));

    int fd = fd_for_alias(*(int *)a->arg_val);
    void *wbuff = a->next->arg_val;
    int count = a->next->arg_size;
    int byteswritten = _fsWrite(fd, wbuff, count);
    response->in_error = byteswritten < 0 ? 1 : 0;
    response->_errno = errno;
    *(int *)response->retval = byteswritten;

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    return r;
}

/**
 * RPC endpoint for fsRemove.
 *
 * @param nparams number of parameters passed in
 * @param a       linked list of arguments
 * @return        RPC response
 */
return_type fsRemove_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }
    fs_response *response = (fs_response *)malloc(sizeof(fs_response));
    char *abspath = concat(ROOT_PATH, (char *)a->arg_val);
    if (is_file_open(abspath)) {
        response->in_error = 1;
        response->_errno = EACCES;
        *(int *)response->retval = -1;
    } else {
        int result = _fsRemove(abspath);
        response->in_error = result < 0 ? 1 : 0;
        response->_errno = errno;
        *(int *)response->retval = result;
    }

    r.return_val = response;
    r.return_size = sizeof(fs_response);
    free(abspath);
    return r;
}

int main(int argc, char *argv[]) {
    srand (time(NULL));

    register_procedure("fsRead", 2, fsRead_rpc);
    register_procedure("fsOpen", 2, fsOpen_rpc);
    register_procedure("fsClose", 1, fsClose_rpc);
    register_procedure("fsWrite", 2, fsWrite_rpc);
    register_procedure("fsMount", 1, fsMount_rpc);
    register_procedure("fsRemove", 1, fsRemove_rpc);
    register_procedure("fsReadDir", 1, fsReadDir_rpc);
    register_procedure("fsOpenDir", 1, fsOpenDir_rpc);
    register_procedure("fsCloseDir", 1, fsCloseDir_rpc);

    // Check that root folder exists.
    ROOT_PATH = argv[1];
    struct stat sbuf;
    if (stat(ROOT_PATH, &sbuf) < 0) {
        perror("stat");
        exit(1);
    }

    // Check that root folder is a directory.
    if (!S_ISDIR(sbuf.st_mode)) {
        printf("path is not a directory\n");
        exit(1);
    }

    launch_server();
    return 0;
}
