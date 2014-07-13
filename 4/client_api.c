#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "simplified_rpc/ece454rpc_types.h"
#include "ece454_fs.h"
#include "fs_utils.h"

/**
    TODO:
        * what happends when you remove a mounted folder (EBUSY ?)
        * what error should be returned if you mount an already mounted folder?
        * remove should be allowed if requesting proc has write access
 */

struct mount_info {
    char ip_or_domain[256];
    char local_folder_name[256];
    unsigned int port_no;
    struct mount_info *next;
};

struct fd_entry {
    int fd;
    struct mount_info *mount_info;
    struct fd_entry *next;
};

struct mount_info *mount_info_list_head = NULL;
struct fd_entry *fd_list_head = NULL;
struct fsDirent dent; // handle this.

void *mount_info_next(void *node) {
    return (void *)((struct mount_info *)node)->next;
}

void mount_info_set_next(void *node, void *next) {
    ((struct mount_info *)node)->next = (struct mount_info *)next;
}

void mount_info_list_insert(struct mount_info *entry) {
    list_insert((void **)&mount_info_list_head, (void *)entry, mount_info_next, mount_info_set_next);
}

void mount_info_list_delete(struct mount_info *entry) {
    list_delete((void **)&mount_info_list_head, (void *)entry, mount_info_next, mount_info_set_next);
}

void *fd_entry_next(void *node) {
    return (void *)((struct fd_entry *)node)->next;
}

void fd_entry_set_next(void *node, void *next) {
    ((struct fd_entry *)node)->next = (struct fd_entry *)next;
}

void fd_entry_list_insert(struct fd_entry *entry) {
    list_insert((void **)&fd_list_head, (void *)entry, fd_entry_next, fd_entry_set_next);
}

void fd_entry_list_delete(struct fd_entry *entry) {
    list_delete((void **)&fd_list_head, entry, fd_entry_next, fd_entry_set_next);
}

struct mount_info* mount_info_list_find(const char *folder_name, bool strict_match) {
    const int query_length = strlen(folder_name);
    struct mount_info *current = mount_info_list_head;
    int longest_length_so_far = 0;
    struct mount_info *longest_match_so_far = NULL;
    for(; current != NULL; current = current->next) {
        const int name_length = strlen(current->local_folder_name);
        if (strict_match && query_length != name_length) continue;
        if (name_length > longest_length_so_far) {
            if (strncmp(current->local_folder_name, folder_name, name_length) == 0) {
                longest_match_so_far = current;
                longest_length_so_far = name_length;
            }
        }
    }
    return longest_match_so_far;
}

struct fd_entry *fd_entry_list_find(int fd) {
    struct fd_entry *current = fd_list_head;
    for (; current != NULL; current = current->next) {
        if (current->fd == fd) break;
    }
    return current;
}

short int handle_possible_error(fs_response *resp) {
    if (resp->in_error) {
        errno = resp->_errno;
    }
    return resp->in_error;
}

char *relative_path_from_mount_path(const char *path, const char *local_folder_name) {
    char *relpath;
    if (strcmp(path, local_folder_name) == 0) {
        relpath = (char *) malloc(2);
        strcpy(relpath, "/");
    } else {
        const int slash_index = strlen(local_folder_name);
        const int path_length = strlen(path);
        if (*(path + slash_index) == '/') {
            printf("%s \n", local_folder_name + slash_index);
            relpath = (char *)malloc(path_length - slash_index + 1);
            strcpy((char *)relpath, path + slash_index);
        } else {
            errno = ENOENT;
            relpath = NULL;
        }
    }
    return relpath;
}

int fsMount(const char *ip_or_domain, const unsigned int port_no, const char *local_folder_name) {
    struct mount_info *info = mount_info_list_find(local_folder_name, true);
    if (info != NULL) {
        errno = EBUSY;
        return -1;
    }

    return_type ans = make_remote_call(ip_or_domain, port_no, "fsMount", 1,
                                       strlen(local_folder_name), (void *) local_folder_name);
    fs_response *response = (fs_response *)ans.return_val;
    if (!handle_possible_error(response)) {
        struct mount_info *info = (struct mount_info *) malloc(sizeof(struct mount_info));
        strcpy(info->ip_or_domain, ip_or_domain);
        strcpy(info->local_folder_name, local_folder_name);
        info->port_no = port_no;
        info->next = NULL;
        mount_info_list_insert(info);
        return 0;
    }
    return -1;
}

int fsUnmount(const char *local_folder_name) {
    struct mount_info *info = mount_info_list_find(local_folder_name, true);
    if (info == NULL) {
        errno = ENOENT;
        return -1;
    }

    mount_info_list_delete(info);
    return 0;
}

FSDIR* fsOpenDir(const char *folder_name) {
    struct mount_info *info = mount_info_list_find(folder_name, false);
    if (info == NULL) {
        errno = ENOENT;
        return NULL;
    }

    char *relative_path = relative_path_from_mount_path(folder_name, info->local_folder_name);
    if (relative_path == NULL) {
        errno = ENOENT;
        return NULL;
    }

    return_type ans = make_remote_call(info->ip_or_domain,
                                       info->port_no, "fsOpenDir", 1,
                                       strlen(relative_path) + 1, (void *) relative_path);
    free(relative_path);
    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        FSDIR *fsdirp = (FSDIR *)malloc(sizeof(FSDIR));
        fsdirp->dirp = *(DIR **)response->retval;
        fsdirp->mount_info = info;
        return fsdirp;
    }
    return NULL;
}

int fsCloseDir(FSDIR *fsdirp) {
    const return_type ans = make_remote_call(fsdirp->mount_info->ip_or_domain,
                                       fsdirp->mount_info->port_no, "fsCloseDir", 1,
                                       sizeof(DIR *), (void *) &fsdirp->dirp);

    fs_response *response = (fs_response *)ans.return_val;
    if (!handle_possible_error(response)) {
        free(fsdirp);
    }

    return *(int *)response->retval;
}

struct fsDirent *fsReadDir(FSDIR *fsdirp) {
    return_type ans = make_remote_call(fsdirp->mount_info->ip_or_domain,
                                       fsdirp->mount_info->port_no, "fsReadDir", 1,
                                       sizeof(DIR *), (void *) &fsdirp->dirp);

    fs_response *response = (fs_response *)ans.return_val;
    if (!handle_possible_error(response)) {
        return (struct fsDirent *)response->retval;
    }
    return NULL;
}

int fsOpen(const char *fname, int mode) {
    struct mount_info *info = mount_info_list_find(fname, false);
    if (info == NULL) {
        errno = ENOENT;
        return -1;
    }

    char *path = relative_path_from_mount_path(fname, info->local_folder_name);
    if (path == NULL) {
        errno = ENOENT;
        return -1;
    }

    return_type ans;
    int attempts = 0, retval;
    fs_response *response;
    do {
        if (attempts++ > 0) sleep(30);
        ans = make_remote_call(info->ip_or_domain,
                               info->port_no, "fsOpen", 2,
                               strlen(path) + 1, (void *) path,
                               sizeof(int), (void *) &mode);
        response = (fs_response *)ans.return_val;
    } while (*(int *)response->retval == FS_OPEN_WAIT_MSG);

    free(path);
    int fd = *(int *)response->retval;
    if (!handle_possible_error(response)) {
        struct fd_entry *entry = (struct fd_entry *)malloc(sizeof(struct fd_entry));
        entry->fd = fd;
        entry->mount_info = info;
        entry->next = NULL;
        fd_entry_list_insert(entry);
        return fd;
    }

    return -1;
}

int fsClose(int fd) {
    struct fd_entry *current = fd_entry_list_find(fd);

    if (current == NULL) {
        errno = EBADF;
        return -1;
    }

    return_type ans = make_remote_call(current->mount_info->ip_or_domain,
                                       current->mount_info->port_no, "fsClose", 1,
                                       sizeof(int), (void *) &fd);

    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        fd_entry_list_delete(current);
    }

    return *(int *)response->retval;
}

int fsRead(int fd, void *buf, const unsigned int count) {
    struct fd_entry *current = fd_entry_list_find(fd);

    if (current == NULL) {
        errno = EBADF;
        return -1;
    }

    return_type ans = make_remote_call(current->mount_info->ip_or_domain,
                                       current->mount_info->port_no, "fsRead", 2,
                                       sizeof(int), (void *) &fd,
                                       sizeof(int), (void *) &count);

    int inerror = *(int *)ans.return_val;
    if (inerror) {
        int _errno = *(int *)(ans.return_val + sizeof(int));
        errno = _errno;
        return -1;
    }

    int bytesread = *(int *)(ans.return_val + (2 * sizeof(int)));
    memcpy(buf, ans.return_val + (3 * sizeof(int)), bytesread);
    return bytesread;
}

int fsWrite(int fd, const void *buf, const unsigned int count) {
    struct fd_entry *current = fd_entry_list_find(fd);

    if (current == NULL) {
        errno = EBADF;
        return -1;
    }

    return_type ans = make_remote_call(current->mount_info->ip_or_domain,
                                       current->mount_info->port_no, "fsWrite", 2,
                                       sizeof(int), (void *) &fd,
                                       count, (void *) buf);

    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        int byteswritten = *(int *)response->retval;
        return byteswritten;
    }

    return -1;
}

int fsRemove(const char *name) {
    struct mount_info *info = mount_info_list_find(name, false);
    if (info == NULL) {
        errno = ENOENT;
        return -1;
    }

    char * relpath = relative_path_from_mount_path(name, info->local_folder_name);

    if (strcmp(relpath, "/") == 0) {
        free(relpath);
        errno = EBUSY;
        return -1;
    }

    return_type ans = make_remote_call(info->ip_or_domain,
                                       info->port_no, "fsRemove", 1,
                                       strlen(relpath) + 1, (void *)relpath);
    free(relpath);
    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        return *(int *)response->retval;
    }

    return -1;
}
