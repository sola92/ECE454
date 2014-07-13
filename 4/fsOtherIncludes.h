/* 
 * Mahesh V. Tripunitara
 * University of Waterloo
 * You specify what goes in this file. I just have a "dummy"
 * specification of the FSDIR type.
 */

#ifndef _ECE_FS_OTHER_INCLUDES_
#define _ECE_FS_OTHER_INCLUDES_
#include <sys/types.h>
#include <dirent.h>


#define FS_OPEN_WAIT_MSG -2

typedef char BYTE;

typedef struct {
    short int in_error;
    int _errno;
    BYTE retval[1000];
} fs_response;

typedef struct {
    DIR *dirp;
    struct mount_info *mount_info;
} FSDIR;


#endif