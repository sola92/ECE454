#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ece454_fs.h"

int main(int argc, char *argv[]) {
    if(argc < 3) {
      fprintf(stderr, "usage: %s <srv-ip> <srv-port>\n", argv[0]);
      exit(1);
    }
    printf("1: %s 2: %d 3: %s 4: %s\n", argv[1], argv[2], argv[3], argv[4]);
    fsMount(argv[1], atoi(argv[2]), "test");

    if (fsMount(argv[3], atoi(argv[4]), "test") == -1) printf("fsMount: PASS");
    else printf("fsMount: FAIL");

    if (fsMount(argv[3], atoi(argv[4]), "test2") == 0) printf("fsMount: PASS");
    else printf("fsMount: FAIL");

    char buf[256];

    int b;
    if ((b = fsOpen("test/create", 0)) == -1) printf("fsOpen(read): PASS\n");
    else printf("fsOpen(read): FAIL\n");

    if ((b = fsOpen("test/create", 1)) != -1) printf("fsOpen(write): PASS\n");
    else printf("fsOpen(write): FAIL\n");

    if (fsRead(b, buf, 20) == -1) printf("fsRead: PASS\n");
    else printf("fsRead: FAIL\n");

    if (fsWrite(b, "Hello World", 10) == 10) printf("fsWrite: PASS\n");
    else printf("fsWrite: FAIL\n");

    if (fsRemove("test/create") == -1) printf("fsRemove: PASS\n");
    else printf("fsRemove: FAIL\n");

    if (fsClose(b) == 0) printf("fsClose: PASS\n");
    else printf("fsClose: FAIL\n");

    if ((b = fsOpen("test/create", 0)) != -1) printf("fsOpen(read): PASS\n");
    else printf("fsOpen(read): FAIL\n");

    char readbuf[10];
    int n;
    if ((n = fsRead(b, readbuf, 4) == 4)) {
      if (!strcmp(readbuf, "Hell")) printf("fsRead: PASS\n");
      else printf("fsRead - string compare: FAIL\n");
    }
    else printf("fsRead - num_bytes: FAIL\n");

    if ((n = fsRead(b, readbuf, 10) == 6)) {
      if (!strcmp(readbuf, "o Worl")) printf("fsRead: PASS\n");
      else printf("fsRead - string compare: FAIL\n");
    }
    else printf("fsRead - num_bytes: FAIL\n");

    fsClose(b);

    if (fsRemove("test/create") == 0) printf("fsRemove: PASS\n");
    else printf("fsRemove: FAIL\n");

    FSDIR *fb;
    if (!(fb = fsOpenDir("test/noexist"))) printf("fsOpenDir: PASS\n");
    else printf("fsOpenDir: FAIL\n");

    if (fsUnmount("noexist") == -1) printf("fsUnmount: PASS\n");
    else printf("fsUnmount: FAIL\n");

    if (fsClose(1337) == -1) printf("fsClose: PASS\n");
    else printf("fsClose: FAIL\n");

    if (fsRemove("test/noexist") == -1) printf("fsRemove: PASS\n");
    else printf("fsRemove: FAIL\n");

    fsUnmount("test");
    fsUnmount("test2");
    return 0;
}
