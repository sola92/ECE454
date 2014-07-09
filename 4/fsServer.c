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

#if 0
#define _DEBUG_1_
#endif


struct fsDirent dent;
char *root_directory;

char* concat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);
    return result;
}

extern int _fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName) {
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


return_type r;

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
    char *abspath = concat(root_directory, folderpath);
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

    char *abspath = concat(root_directory, (char *)a->arg_val);
    int flags = *(int *)a->next->arg_val;
    int fd = _fsOpen(abspath, flags);
    printf("path: %s res: %d\n", abspath, fd);
    free(abspath);

    fs_response *response = (fs_response *) malloc(sizeof(fs_response));
    response->in_error = fd < 0 ? 1 : 0;
    response->_errno = errno;
    *(int *)response->retval = fd;

    r.return_val = response;
    r.return_size = sizeof(fs_response);
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

    fs_response *response = (fs_response *)malloc(sizeof(fs_response));

    int fd = *(int *)a->arg_val;
    int count = *(int *)a->next->arg_val;
    int bytesread = _fsRead(fd, (void *)(response->retval + 4), count);
    *(int *)response->retval = bytesread;
    response->in_error = bytesread < 0 ? 1 : 0;
    response->_errno = errno;

    r.return_val = response;
    r.return_size = sizeof(fs_response);
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

int main(int argc, char *argv[]) {
    root_directory = argv[1];
    register_procedure("fsMount", 1, fsMount_rpc);
    register_procedure("fsOpenDir", 1, fsOpenDir_rpc);
    register_procedure("fsCloseDir", 1, fsCloseDir_rpc);
    register_procedure("fsReadDir", 1, fsReadDir_rpc);
    register_procedure("fsOpen", 2, fsOpen_rpc);
    register_procedure("fsClose", 1, fsClose_rpc);
    register_procedure("fsRead", 2, fsRead_rpc);
    register_procedure("fsWrite", 2, fsWrite_rpc);
    launch_server();
    return 0;
}
