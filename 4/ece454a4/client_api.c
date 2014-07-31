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

#include "client_api.h"

/* Following convetions @ https://code.google.com/p/google-styleguide */

struct fsDirent    dent;
struct fd_entry   *fd_list_head = NULL;
struct mount_info *mount_info_list_head = NULL;

/**
 * Returns the successor of a mount_info object
 *
 * @param node predecessor object
 * @return     successor of object passed in
 */
void *mount_info_next(void *node) {
    return (void *)((struct mount_info *)node)->next;
}

/**
 * Sets connects two mount_info elements. "next" will
 * be the successor to "node"
 *
 * @param node predecessor elements
 * @param node successor elements
 */
void mount_info_set_next(void *node, void *next) {
    ((struct mount_info *)node)->next = (struct mount_info *)next;
}

/**
 * Inserts an element into the mount_info linked list.
 *
 * @param entry element to insert into the list
 */
void mount_info_list_insert(struct mount_info *entry) {
    list_insert((void **)&mount_info_list_head, (void *)entry, mount_info_next, mount_info_set_next);
}

/**
 * Remove a mount_info element from the linked list
 *
 * @param entry element to be removed
 */
void mount_info_list_delete(struct mount_info *entry) {
    list_delete((void **)&mount_info_list_head, (void *)entry, mount_info_next, mount_info_set_next);
}

/**
 * Returns the successor of a fd_entry object
 *
 * @param node predecessor object
 * @return     successor of object passed in
 */
void *fd_entry_next(void *node) {
    return (void *)((struct fd_entry *)node)->next;
}

/**
 * Sets connects two fd_entry elements. "next" will
 * be the successor to "node"
 *
 * @param node predecessor elements
 * @param node successor elements
 */
void fd_entry_set_next(void *node, void *next) {
    ((struct fd_entry *)node)->next = (struct fd_entry *)next;
}

/**
 * Inserts an element into the fd_entry linked list.
 *
 * @param entry element to insert into the list
 */
void fd_entry_list_insert(struct fd_entry *entry) {
    list_insert((void **)&fd_list_head, (void *)entry, fd_entry_next, fd_entry_set_next);
}

/**
 * Remove a fd_entry element from the linked list
 *
 * @param entry element to be removed
 */
void fd_entry_list_delete(struct fd_entry *entry) {
    list_delete((void **)&fd_list_head, entry, fd_entry_next, fd_entry_set_next);
}

/**
 * Searches the mount_info linked list for an element with a folder name.
 * If strict_match is specified, an element with exact same folder name as the argument
 * is returned. If strict_match is false, an element with longest matching folder name
 * as the argument is returned.
 *
 * @param folder_name  folder to search for
 * @param strict_match specifies if a strict match is to be made
 * @return             matched element in the linked list
 */
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

/**
 * Searches the fd_entry linked list for an element with the same file descriptor
 * as the argument.
 *
 * @param fd file descriptor to search for
 * @return   matched element in the linked list
 */
struct fd_entry *fd_entry_list_find(int fd) {
    struct fd_entry *current = fd_list_head;
    for (; current != NULL; current = current->next) {
        if (current->fd == fd) break;
    }
    return current;
}

/**
 * Analyzes a response from the server and checks if it has an error.
 * If the response has an error, errno is to the errno in the response.
 *
 * @param resp server response to be analyzed
 * @return     boolean indicating whether the response has an error
 */
short int handle_possible_error(fs_response *resp) {
    if (resp->in_error) {
        errno = resp->_errno;
    }
    return resp->in_error;
}

/**
 * Takes a local path to a mounted folder and the name of the mounted
 * folder and returns a path relative to the mounted folder.
 *
 * @param path              path to a mounted folder
 * @param local_folder_name name of the mounted folder
 * @return                  relative path to the mounted folder
 */
char *relative_path_from_mount_path(const char *path, const char *local_folder_name) {
    char *relpath;
    if (strcmp(path, local_folder_name) == 0) {
        relpath = (char *) malloc(2);
        strcpy(relpath, "/");
    } else {
        const int slash_index = strlen(local_folder_name);
        const int path_length = strlen(path);
        relpath = (char *)malloc(path_length - slash_index + 1);
        strcpy((char *)relpath, path + slash_index);
    }
    return relpath;
}

/**
 * Mounts the remote server folder locally, and have it be referred to as local_folder_name.
 * Returns 0 on success, and −1 on failure.
 *
 * @param ip_or_domain      server IP address or domain name
 * @param port_no           server port no.
 * @param local_folder_name name of the mounted folder
 * @return                  0 on success, and −1 on failure
 */
int fsMount(const char *ip_or_domain, const unsigned int port_no, const char *local_folder_name) {
    struct mount_info *info = mount_info_list_find(local_folder_name, true);
    if (info != NULL) {
        errno = EBUSY;
        return -1;
    }

    return_type ans = make_remote_call(ip_or_domain, port_no, "fsMount", 1,
                                       strlen(local_folder_name), (void *) local_folder_name);
    fs_response *response = (fs_response *)ans.return_val;
    int retval = -1;
    if (!handle_possible_error(response)) {
        struct mount_info *info = (struct mount_info *) malloc(sizeof(struct mount_info));
        strcpy(info->ip_or_domain, ip_or_domain);
        strcpy(info->local_folder_name, local_folder_name);
        info->port_no = port_no;
        info->next = NULL;
        mount_info_list_insert(info);
        retval = 0;
    }

    free(response);
    return retval;
}

/**
 * Unmounts a remote filesystem that is referred to locally by local_folder_name.
 * Returns 0 on success, −1.
 *
 * @param local_folder_name name of the mounted folder
 * @return                  0 on success, −1 on failure
 */
int fsUnmount(const char *local_folder_name) {
    struct mount_info *info = mount_info_list_find(local_folder_name, true);
    if (info == NULL) {
        errno = ENOENT;
        return -1;
    }

    mount_info_list_delete(info);
    return 0;
}

/**
 * Opens the folder folder_name that is presumably the local name of
 * a folder that has been mounted previously. Returns a NULL if an error occurs.
 *
 * @param folder_name name of the directory
 * @return            struct containing details about the directory, NULL if failure
 */
FSDIR* fsOpenDir(const char *folder_name) {
    struct mount_info *info = mount_info_list_find(folder_name, false);
    if (info == NULL) {
        errno = ENOENT;
        return NULL;
    }

    char *relpath = relative_path_from_mount_path(folder_name, info->local_folder_name);
    if (relpath == NULL) {
        errno = ENOENT;
        return NULL;
    }

    return_type ans = make_remote_call(info->ip_or_domain,
                                       info->port_no, "fsOpenDir", 1,
                                       strlen(relpath) + 1, (void *) relpath);

    fs_response *response = (fs_response *)ans.return_val;
    FSDIR *fsdirp = NULL;
    if (!handle_possible_error(response)) {
        fsdirp = (FSDIR *)malloc(sizeof(FSDIR));
        fsdirp->dirp = *(DIR **)response->retval;
        fsdirp->mount_info = info;
    }

    free(relpath);
    free(response);
    return fsdirp;
}

/**
 * Closes a directory.
 *
 * @param fsdirp details of the directory to be closed
 * @return       0 on success, −1 on failure
 */
int fsCloseDir(FSDIR *fsdirp) {
    const return_type ans = make_remote_call(fsdirp->mount_info->ip_or_domain,
                                       fsdirp->mount_info->port_no, "fsCloseDir", 1,
                                       sizeof(DIR *), (void *) &fsdirp->dirp);

    fs_response *response = (fs_response *)ans.return_val;
    int retval = *(int *)response->retval;
    if (!handle_possible_error(response)) {
        free(fsdirp);
    }

    free(response);
    return retval;
}

/**
 * Reads a directory. Returns NULL of an error occured.
 *
 * @param fsdirp details of the directory to be read
 * @return       fsDirent of the directory to be read.
 */
struct fsDirent *fsReadDir(FSDIR *fsdirp) {
    return_type ans = make_remote_call(fsdirp->mount_info->ip_or_domain,
                                       fsdirp->mount_info->port_no, "fsReadDir", 1,
                                       sizeof(DIR *), (void *) &fsdirp->dirp);

    fs_response *response = (fs_response *)ans.return_val;
    struct fsDirent *retval = NULL;
    if (!handle_possible_error(response)) {
        dent = *(struct fsDirent *)response->retval;
        retval = &dent;
    }

    free(response);
    return retval;
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
        if (attempts++ > 0) sleep(1);
        ans = make_remote_call(info->ip_or_domain,
                               info->port_no, "fsOpen", 2,
                               strlen(path) + 1, (void *) path,
                               sizeof(int), (void *) &mode);
        response = (fs_response *)ans.return_val;
    } while (*(int *)response->retval == FS_OPEN_WAIT_MSG);


    int fd = *(int *)response->retval;
    if (!handle_possible_error(response)) {
        struct fd_entry *entry = (struct fd_entry *)malloc(sizeof(struct fd_entry));
        entry->fd = fd;
        entry->mount_info = info;
        entry->next = NULL;
        fd_entry_list_insert(entry);
    }

    free(path);
    free(response);
    return fd;
}

/**
 * Closes an open file.
 *
 * @param fd file descriptor of file to be closed
 * @return   0 on success, −1 on failure
 */
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

    int retval = *(int *)response->retval;
    free(response);
    return retval;
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
        free(ans.return_val);
        return -1;
    }

    int bytesread = *(int *)(ans.return_val + (2 * sizeof(int)));
    memcpy(buf, ans.return_val + (3 * sizeof(int)), bytesread);
    free(ans.return_val);
    return bytesread;
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
    int retval = *(int *)response->retval;
    handle_possible_error(response);
    free(response);
    return retval;
}

/**
 * Removes (i.e., deletes) this file or folder from the server.
 *
 * @param name of file or folder to be removed
 * @return     0 on success, −1 on failure
 */
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
    fs_response *response = (fs_response *)ans.return_val;

    int retval = *(int *)response->retval;
    handle_possible_error(response);

    free(relpath);
    free(response);
    return retval;
}