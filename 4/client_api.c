#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "simplified_rpc/ece454rpc_types.h"
/* 
 * Mahesh V. Tripunitara
 * University of Waterloo
 * A dummy implementation of the functions for the remote file
 * system. This file just implements those functions as thin
 * wrappers around invocations to the local filesystem. In this
 * way, this dummy implementation helps clarify the semantics
 * of those functions. Look up the man pages for the calls
 * such as opendir() and read() that are made here.
 */
#include "ece454_fs.h"
#include <string.h>

// what error should be returned if you mount an already mounted folder?
// read after a close still reads
// O_CREAT causes isssues

struct fd_entry {
    int fd;
    struct mount_info *mount_info;
    struct fd_entry *next;
};

struct mount_info *mount_list_head = NULL;
struct fd_entry *fd_list_head = NULL;
struct fsDirent dent; // handle this.


void *mount_info_next(void *node) {
    return (void *)((struct mount_info *)node)->next;
}

void mount_info_set_next(void *node, void *next) {
    ((struct mount_info *)node)->next = (struct mount_info *)next;
}

void *fd_entry_next(void *node) {
    return (void *)((struct fd_entry *)node)->next;
}

void fd_entry_set_next(void *node, void *next) {
    ((struct fd_entry *)node)->next = (struct fd_entry *)next;
}

int list_size(void *head, void *(next)(void *)) {
    int size = 0;
    void *current = head;
    for(; current != NULL; current = next(current), size++);
    return size;
}

void list_delete(void **head, void *node, void *(next)(void *), void(set_next)(void *, void *)) {
    if (node == *head) {
        *head = next(head);
    } else {
        void *current = *head;
        for(; next(current) != node; current = next(current));
        set_next(current, next(node));
    }

    free(node);
}

void list_insert(void **head, void *new, void *(next)(void *), void(set_next)(void *, void *)) {
    if (*head == NULL) {
        *head = new;
        return;
    }

    void *current = *head;
    for(; next(current) != NULL; current = next(current));
    set_next(current, new);
}

struct mount_info* mount_info_list_find(const char *folder_name, bool strict_match) {
    const int query_length = strlen(folder_name);
    struct mount_info *current = mount_list_head;
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

struct fd_entry *fd_entry_find(int fd) {
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

char *get_relative_path(const char *path, const char *local_folder_name) {
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
        list_insert((void **)&mount_list_head, (void *)info, mount_info_next, mount_info_set_next);
        return 0;
    }
    return -1;
}

int fsUnmount(const char *local_folder_name) {
    struct mount_info *info = mount_info_list_find(local_folder_name, true);
    list_delete((void **)&mount_list_head, (void *)info, mount_info_next, mount_info_set_next);
    return 0;
}

FSDIR* fsOpenDir(const char *folder_name) {
    struct mount_info *info = mount_info_list_find(folder_name, false);
    if (info == NULL) {
        errno = ENOENT;
        return NULL;
    }

    char *relative_path = get_relative_path(folder_name, info->local_folder_name);
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

    char *path = get_relative_path(fname, info->local_folder_name);
    if (path == NULL) {
        errno = ENOENT;
        return -1;
    }

    int flags = -1;
    if (mode == 0) {
        flags = O_RDONLY;
    } else if(mode == 1) {
        flags = O_WRONLY;// | O_CREAT;
    }

    return_type ans = make_remote_call(info->ip_or_domain,
                                       info->port_no, "fsOpen", 2,
                                       strlen(path) + 1, (void *) path,
                                       sizeof(int), (void *) &flags);
    fs_response *response = (fs_response *)ans.return_val;
    int fd = *(int *)response->retval;
    if (!handle_possible_error(response)) {
        struct fd_entry *entry = (struct fd_entry *)malloc(sizeof(struct fd_entry));
        entry->fd = fd;
        entry->mount_info = info;
        entry->next = NULL;
        list_insert((void *)&fd_list_head, (void *)entry, fd_entry_next, fd_entry_set_next);
    }

    free(path);
    return fd;
}

int fsClose(int fd) {
    struct fd_entry *current = fd_entry_find(fd);

    if (current == NULL) {
        errno = EBADF;
        return -1;
    }

    return_type ans = make_remote_call(current->mount_info->ip_or_domain,
                                       current->mount_info->port_no, "fsClose", 1,
                                       sizeof(int), (void *) &fd);

    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        list_delete((void **)&fd_list_head, current, fd_entry_next, fd_entry_set_next);
    }

    return *(int *)response->retval;
}

int fsRead(int fd, void *buf, const unsigned int count) {
    struct fd_entry *current = fd_entry_find(fd);

    if (current == NULL) {
        errno = EBADF;
        return -1;
    }

    return_type ans = make_remote_call(current->mount_info->ip_or_domain,
                                       current->mount_info->port_no, "fsRead", 2,
                                       sizeof(int), (void *) &fd,
                                       sizeof(int), (void *) &count);

    fs_response *response = (fs_response *)ans.return_val;

    if (!handle_possible_error(response)) {
        int bytesread = *(int *)response->retval;
        memcpy(buf, response->retval + 4, bytesread);
        return bytesread;
    }

    return -1;
}

int fsWrite(int fd, const void *buf, const unsigned int count) {
    struct fd_entry *current = fd_entry_find(fd);

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
    return(remove(name));
}
