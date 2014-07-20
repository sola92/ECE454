#ifndef _SERVER_H_
#define _SERVER_H_

#include "simplified_rpc/ece454rpc_types.h"
#include "ece454_fs.h"
#include "fs_utils.h"

struct file_status {
    char path[512];
    int fd;
    int mode;
    struct file_status *next;
};

void *file_status_next(void *node);
void  file_status_set_next(void *node, void *next);
void  file_status_list_insert(struct file_status *entry);
void  file_status_list_delete(struct file_status *entry);
int   file_status_list_size();

struct file_status *file_status_list_find(void *crit, bool bypath);
struct file_status *file_status_list_find_by_path(char *fpath);
struct file_status *file_status_list_find_by_fd(int fd);

bool is_file_open(char *fpath);
bool is_file_open_for_writing(char *fpath);

int  _fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName);
int  _fsUnmount(const char *localFolderName);
int  _fsCloseDir(DIR *folder);
int  _fsOpen(const char *fname, int mode);
int  _fsClose(int fd);
int  _fsRead(int fd, void *buf, const unsigned int count);
int  _fsWrite(int fd, const void *buf, const unsigned int count);
int  _fsRemove(const char *name);
DIR *_fsOpenDir(const char *folderName);
struct fsDirent *_fsReadDir(DIR *folder);

return_type fsMount_rpc(const int nparams, arg_type* a);
return_type fsOpenDir_rpc(const int nparams, arg_type* a);
return_type fsCloseDir_rpc(const int nparams, arg_type* a);
return_type fsReadDir_rpc(const int nparams, arg_type* a);
return_type fsOpen_rpc(const int nparams, arg_type* a);
return_type fsClose_rpc(const int nparams, arg_type* a);
return_type fsRead_rpc(const int nparams, arg_type* a);
return_type fsWrite_rpc(const int nparams, arg_type* a);
return_type fsRemove_rpc(const int nparams, arg_type* a);

#endif