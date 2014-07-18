#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ece454_fs.h"

void printBuf(char *buf, int size);

int main(int argc, char *argv[]) {
   /* const char *ip = argv[1];
    const int port = atoi(argv[2]);
    char *dirname = NULL;

    if(argc > 3) dirname = argv[3];
    else {
    	dirname = (char *)calloc(strlen("/")+1, sizeof(char));
    	strcpy(dirname, "/");
    }

    printf("fsMount(%s): %d\n", dirname, fsMount(ip, port, dirname));
    FSDIR *fd = fsOpenDir(dirname);
    if(fd == NULL) {
	   perror("fsOpenDir"); exit(1);
    }

    struct fsDirent *fdent = NULL;
    for(fdent = fsReadDir(fd); fdent != NULL; fdent = fsReadDir(fd)) {
	   printf("\t %s, %d\n", fdent->entName, (int)(fdent->entType));
    }

    if(errno != 0) {
	   perror("fsReadDir");
    }

    printf("fsCloseDir(): %d\n", fsCloseDir(fd));

    int ff = fsOpen("/dev/urandom", 0);
    if(ff < 0) {
	   perror("fsOpen"); exit(1);
    }
    else printf("fsOpen(): %d\n", ff);

    char fname[15];
    if(fsRead(ff, (void *)fname, 10) < 0) {
	   perror("fsRead"); exit(1);
    }

    int i;
    for(i = 0; i < 10; i++) {
	   //printf("%d\n", ((unsigned char)(fname[i]))%26);
	   fname[i] = ((unsigned char)(fname[i]))%26 + 'a';
    }
    fname[10] = (char)0;
    printf("Filename to write: %s\n", (char *)fname);

    char buf[256];
    if(fsRead(ff, (void *)buf, 256) < 0) {
	   perror("fsRead(2)"); exit(1);
    }

    printBuf(buf, 256);

    printf("fsClose(): %d\n", fsClose(ff));*/

    /*ff = fsOpen("/adasdasdasd", 1);
    if(ff < 0) {
	   perror("fsOpen(write)"); exit(1);
    }

    if(fsWrite(ff, buf, 256) < 256) {
	   fprintf(stderr, "fsWrite() wrote fewer than 256\n");
    }

    if(fsClose(ff) < 0) {
	   perror("fsClose"); exit(1);
    }

    printf("fsRemove(%s): %d\n", fname, fsRemove(fname));
    perror(NULL);*/
    const char *ip = argv[1];
    const int port = atoi(argv[2]);
    if (fsMount(ip, port, "foo") < 0) {
        perror(NULL);
    }

    int fd = fsOpen("foo/largefile.txt", 0);
    char buf[3001];
    int bytesread;
    int i = 0;
    while ((bytesread = fsRead(fd, (void *)buf, 3000)) > 0) {
        *((char *) buf + bytesread) = '\0';
        printf("%s", (char *) buf);
        i += 1;
    }

    if (fsClose(fd) < 0) {
        perror(NULL);
    }


    if (fsUnmount("foo") < 0) {
        perror(NULL);
    }

    return 0;
}
