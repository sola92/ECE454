/*
 * Mahesh V. Tripunitara
 * University of Waterloo
 * You are NOT allowed to modify this file.
 */

#ifndef _ECE_454_FS_H_
#define _ECE_454_FS_H_
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

typedef char BYTE;

#include "fsOtherIncludes.h" /* At the minimum, this should define
				the type FSDIR. */

struct fsDirent {
    char entName[256];
    unsigned char entType; /* 0 for file, 1 for folder,
			      -1 otherwise. */
};

struct mount_info {
    char ip_or_domain[250];
    char local_folder_name[250];
    unsigned int port_no;
    struct mount_info *next;
};

typedef struct {
    DIR *dirp;
    struct mount_info *mount_info;
} FSDIR;

int  list_size(void *head, void *(next)(void *));
void list_delete(void **head, void *node, void *(next)(void *), void(set_next)(void *, void *));
void list_insert(void **head, void *new, void *(next)(void *), void(set_next)(void *, void *));

extern int fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName);
extern int fsUnmount(const char *localFolderName);
extern FSDIR* fsOpenDir(const char *folderName);
extern int fsCloseDir(FSDIR *);
extern struct fsDirent *fsReadDir(FSDIR *);
extern int fsOpen(const char *fname, int mode);
extern int fsClose(int);
extern int fsRead(int fd, void *buf, const unsigned int count);
extern int fsWrite(int fd, const void *buf, const unsigned int count);
extern int fsRemove(const char *name);


#endif