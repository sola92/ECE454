#include <stdio.h>
#include <stdlib.h>
#include "ece454rpc_types.h"


int main(int argc, char *argv[]) {
    int a = -10, b = 20;
    if (argc < 3) {
        printf("Error: please supply a server address & port\n");
        return 0;
    }

    char *server_addr = argv[1];
    int server_port = atoi(argv[2]);
    return_type ans = make_remote_call(
                        server_addr,
                        server_port,
                        "addtwo", 2,
                        sizeof(int), (void *)(&a),
                        sizeof(int), (void *)(&b)
                      );
    int i = *(int *)(ans.return_val);
    printf("add(-10, 20), got result: %d\n", i);

    a = 7;
    ans = make_remote_call(
            server_addr,
            server_port,
            "square", 1,
            sizeof(int), (void *)(&a)
          );
    i = *(int *)(ans.return_val);
    printf("square(7), got result: %d\n", i);

    ans = make_remote_call(
            server_addr,
            server_port,
            "rand_me", 0
          );
    i = *(int *)(ans.return_val);
    printf("rand(), got result: %d\n", i);
    return 0;
}