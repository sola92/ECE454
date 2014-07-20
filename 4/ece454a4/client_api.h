#ifndef _CLIENT_API_H_
#define _CLIENT_API_H_

#include "simplified_rpc/ece454rpc_types.h"
#include "ece454_fs.h"
#include "fs_utils.h"

struct mount_info {
    char   ip_or_domain[256];
    char   local_folder_name[256];
    struct mount_info *next;
    unsigned int port_no;
};

struct fd_entry {
    int    fd;
    struct mount_info *mount_info;
    struct fd_entry *next;
};

void *mount_info_next(void *node);
void  mount_info_set_next(void *node, void *next);
void  mount_info_list_insert(struct mount_info *entry);
void  mount_info_list_delete(struct mount_info *entry);
struct mount_info *mount_info_list_find(const char *folder_name, bool strict_match);

void *fd_entry_next(void *node);
void  fd_entry_set_next(void *node, void *next);
void  fd_entry_list_insert(struct fd_entry *entry);
void  fd_entry_list_delete(struct fd_entry *entry);

struct fd_entry *fd_entry_list_find(int fd);
short int handle_possible_error(fs_response *resp);
char *relative_path_from_mount_path(const char *path, const char *local_folder_name);

#endif