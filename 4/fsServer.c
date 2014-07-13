#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "simplified_rpc/ece454rpc_types.h"
#include "ece454_fs.h"
#include "fs_utils.h"

#if 0
#define _DEBUG_1_
#endif

struct file_status {
    char path[512];
    int fd;
    int mode;
    struct file_status *next;
};

struct file_status *file_status_list_head = NULL;
struct fsDirent dent;
char *ROOT_PATH;
return_type r;

void *file_status_next(void *node) {
    return (void *)((struct file_status *)node)->next;
}

void file_status_set_next(void *node, void *next) {
    ((struct file_status *)node)->next = (struct file_status *)next;
}

void file_status_list_insert(struct file_status *entry) {
    list_insert((void **)&file_status_list_head, (void *)entry, file_status_next, file_status_set_next);
}

void file_status_list_delete(struct file_status *entry) {
    list_delete((void **)&file_status_list_head, (void *)entry, file_status_next, file_status_set_next);
}

int file_status_list_size() {
    return list_size((void *)file_status_list_head, file_status_next);
}

struct file_status *file_status_list_find(void *crit, bool bypath) {
    struct file_status *current = file_status_list_head;
    for(; current != NULL; current = current->next) {
        if (bypath && strcmp((char *)crit, current->path) == 0)
            return current;
        if (!bypath && *(int *)crit == current->fd)
            return current;
    }
    return NULL;
}

struct file_status *file_status_list_find_by_path(char *fpath) {
    return file_status_list_find((void *)fpath, true);
}

struct file_status *file_status_list_find_by_fd(int fd) {
    return file_status_list_find((void *)&fd, false);
}

bool is_file_open(char *fpath) {
    return file_status_list_find_by_path(fpath) != NULL;
}

bool is_file_open_for_writing(char *fpath) {
    struct file_status *current = file_status_list_head;
    for(; current != NULL; current = current->next) {
        if (strcmp(fpath, current->path) == 0 && current->mode == 1)
            return true;
    }
    return false;
}

int _fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName) {
    struct stat sbuf;
    return stat(localFolderName, &sbuf);
}

int _fsUnmount(const char *localFolderName) {
    return 0;
}

DIR* _fsOpenDir(const char *folderName) {
    return (opendir(folderName));
}

int _fsCloseDir(DIR *folder) {
    return(closedir(folder));
}

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

int _fsClose(int fd) {
    return(close(fd));
}

int _fsRead(int fd, void *buf, const unsigned int count) {
    return(read(fd, buf, (size_t)count));
}

int _fsWrite(int fd, const void *buf, const unsigned int count) {
    return(write(fd, buf, (size_t)count));
}

int _fsRemove(const char *name) {
    return(remove(name));
}

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

return_type fsOpenDir_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    char *folderpath = (char *) a->arg_val;
    char *abspath = concat(ROOT_PATH, folderpath);
    printf("%s\n", abspath);
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
    if (is_file_open_for_writing(abspath)) {
        response->in_error = 1;
        response->_errno = EACCES;
        *(int *)response->retval = FS_OPEN_WAIT_MSG;
    } else {
        int fd = _fsOpen(abspath, mode);

        printf("path: %s res: %d\n", abspath, fd);

        response->in_error = fd < 0 ? 1 : 0;
        response->_errno = errno;
        *(int *)response->retval = fd;

        if (!response->in_error) {
            struct file_status *entry = (struct file_status *)malloc(sizeof(struct file_status));
            strcpy(entry->path, abspath);
            entry->mode = mode;
            entry->fd = fd;
            entry->next = NULL;
            file_status_list_insert(entry);
            printf("size: %d\n", file_status_list_size());
        }
    }

    r.return_val = response;
    r.return_size = sizeof(fs_response);

    free(abspath);
    return r;
}

return_type fsClose_rpc(const int nparams, arg_type* a) {
    if(nparams != 1) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int fd = *(int *)a->arg_val;
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

return_type fsRead_rpc(const int nparams, arg_type* a) {
    if(nparams != 2) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int fd = *(int *)a->arg_val;
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

return_type fsWrite_rpc(const int nparams, arg_type* a) {
    if(nparams != 2) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    fs_response *response = (fs_response *)malloc(sizeof(fs_response));

    int fd = *(int *)a->arg_val;
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
    ROOT_PATH = argv[1];
    register_procedure("fsRead", 2, fsRead_rpc);
    register_procedure("fsOpen", 2, fsOpen_rpc);
    register_procedure("fsClose", 1, fsClose_rpc);
    register_procedure("fsWrite", 2, fsWrite_rpc);
    register_procedure("fsMount", 1, fsMount_rpc);
    register_procedure("fsRemove", 1, fsRemove_rpc);
    register_procedure("fsReadDir", 1, fsReadDir_rpc);
    register_procedure("fsOpenDir", 1, fsOpenDir_rpc);
    register_procedure("fsCloseDir", 1, fsCloseDir_rpc);
    launch_server();
    return 0;
}
