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

#endif