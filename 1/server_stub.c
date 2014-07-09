#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <ifaddrs.h>
#include <errno.h>
#include "ece454rpc_types.h"


#define PORT_RANGE_LO   10000
#define PORT_RANGE_HI   10100

/*
 * mybind() -- a wrapper to bind that tries to bind() to a port in the
 * range PORT_RANGE_LO - PORT_RANGE_HI, inclusive.
 *
 * Parameters:
 *
 * sockfd -- the socket descriptor to which to bind
 *
 * addr -- a pointer to struct sockaddr_in. mybind() works for AF_INET sockets only.
 * Note that addr is and in-out parameter. That is, addr->sin_family and
 * addr->sin_addr are assumed to have been initialized correctly before the call.
 * Also, addr->sin_port must be 0, or the call returns with an error. Up on return,
 * addr->sin_port contains, in network byte order, the port to which the call bound
 * sockfd.
 *
 * returns int -- negative return means an error occurred, else the call succeeded.
 */
int mybind(int sockfd, struct sockaddr_in *addr) {
    if(sockfd < 1) {
        fprintf(stderr, "mybind(): sockfd has invalid value %d\n", sockfd);
        return -1;
    }

    if(addr == NULL) {
        fprintf(stderr, "mybind(): addr is NULL\n");
        return -1;
    }

    if(addr->sin_port != 0) {
        fprintf(stderr, "mybind(): addr->sin_port is non-zero. Perhaps you want bind() instead?\n");
        return -1;
    }

    unsigned short p;
    for(p = PORT_RANGE_LO; p <= PORT_RANGE_HI; p++) {
        addr->sin_port = htons(p);
        int b = bind(sockfd, (const struct sockaddr *)addr, sizeof(struct sockaddr_in));
        if(b < 0) {
            continue;
        }
        else {
            break;
        }
    }

    if(p > PORT_RANGE_HI) {
        fprintf(stderr, "mybind(): all bind() attempts failed. No port available...?\n");
        return -1;
    }

    /* Note: upon successful return, addr->sin_port contains, in network byte order, the
     * port to which we successfully bound. */
    return 0;
}


/*
 * Following C Coding Standard from CMU.
 * http://users.ece.cmu.edu/~eno/coding/CCodingStandard.html
 */
typedef struct {
    const char *procedure_name; /* stored procedure name */
    int nparams;                /* expected number of parameters for the stored procedure */
    fp_type fnpointer;          /* pointer to the stored procedure function */
} stored_procedure;


BYTE buffer[500];               /* used to store data from incoming requests */
const int BUFFER_LENGTH = 500;  /* length of buffer declared above */



stored_procedure proc_list[50]; /* list of registered procedures */
int num_procs = 0;              /* current number of registered procedures */
bool register_procedure(const char *procedure_name,
                        const int nparams, fp_type fnpointer) {
    proc_list[num_procs].procedure_name = procedure_name;
    proc_list[num_procs].fnpointer = fnpointer;
    proc_list[num_procs].nparams = nparams;
    num_procs += 1;
    return 1;
}

/*
 * Returns the public address of the current host machine.
 */
uint32_t getpublicaddress() {
    struct ifaddrs * ifa_list_head = NULL;
    uint32_t addr = 0;
    getifaddrs(&ifa_list_head);
    struct ifaddrs *ifa;

    for (ifa = ifa_list_head; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family == AF_INET) {
            struct in_addr * address_ptr = NULL;
            address_ptr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            if (strncmp("lo", ifa->ifa_name, 2) != 0) {
                addr = address_ptr->s_addr;
            }
        }
    }

    if (ifa_list_head != NULL) {
        freeifaddrs(ifa_list_head);
    } /* if we found an IP address list, free it's memory */

    return addr;
}

void launch_server() {
    int i; /* Re-usable loop variable. */

    /* Try opening a socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("%s\n", "ERROR opening socket\n");
        return;
    } /* couldn't open a socket */

    struct sockaddr_in serv_address, client_address;
    serv_address.sin_family = AF_INET;
    serv_address.sin_addr.s_addr = getpublicaddress();
    serv_address.sin_port = 0; /* want a random port number */
    unsigned int len_address = sizeof(serv_address);


    /* Bind socket to the host IP address and a random portno. */
    if (mybind(sockfd, &serv_address) < 0) {
        printf("%s", "Unable to bind socket to IP and port number\n");
        perror("bind: ");
        return;
    } /* couldn't bind the socket to an IP address/port */

    getsockname(sockfd, (struct sockaddr *) &serv_address, &len_address);

    printf("%s %d \n", inet_ntoa(serv_address.sin_addr),
                       serv_address.sin_port);

    /* Listen and queue incoming requests. */
    listen(sockfd, /* max queue size */ 3);

    while (1) {
        /* Try accepting an incoming connection. */
        int newfd = accept(sockfd, (struct sockaddr *) &client_address,
                                    &len_address);
        if (newfd < 0) {
            printf("%s\n", "ERROR on accept\n");
            continue;
        } /* not able to accept an incoming connection. */

        /* Read request data into the buffer. */
        if (read(newfd, buffer, BUFFER_LENGTH) < 0) {
            printf("%s\n", "ERROR reading from socket\n");
            continue;
        }

        BYTE *ptr = buffer; /* pointer used to iterate through the buffer */

        /* account for NULL character in length */
        int procedure_name_length = strlen(ptr) + 1;
        ptr += procedure_name_length;

        int nparams = *(int *)ptr;
        ptr += sizeof(int);

        /* Search for the registered procedure. */
        bool found_proc = 0;    /* indicates that we've found the target procedure */
        stored_procedure proc;
        for (i = 0; i < num_procs; ++i) {
            if (strcmp(buffer, proc_list[i].procedure_name) == 0) {
                proc = proc_list[i];
                found_proc = 1;
                break;
            }  /* if we found the registered procedure */
        }

        if (!found_proc) {
            printf("%s", "ERROR requested procedure not found!");
            continue;
        } /* if we didn't find the registered procedure */

        if (proc.nparams != nparams) {
            printf("Expected %d parameters, Got %d \n", proc.nparams,
                                                        nparams);
            continue;
        } /* if the number of params sent don't match what we expected */

        arg_type args_head;
        arg_type *arg = &args_head;
        args_head.next = NULL;
        for (i = 0; i < nparams; i++, arg = arg->next) {
            arg->arg_size = *(int *)ptr; ptr += sizeof(int);
            arg->arg_val = (void *)ptr;  ptr += arg->arg_size;
            if (i < nparams - 1) {
                arg->next = (arg_type *)malloc(sizeof(arg_type));
            } /* if we're not in the last parameter */
            else {
                arg->next = NULL;
            } /* we're in the last parameter */
        }

        return_type r = proc.fnpointer(nparams, &args_head);
        ptr = buffer;
        *(int *)ptr = r.return_size;
        ptr += sizeof(int);
        memcpy(ptr, r.return_val, r.return_size);
        if (write(newfd, buffer, BUFFER_LENGTH) < 0) {
            printf("%s\n", "ERROR writing to socket");
            continue;
        }

        /* free memory allocated for param list */
        arg_type* current = args_head.next;
        while (current != NULL) {
            arg_type* next = current->next;
            free(current);
            current = next;
        }
    } /* end forever */
}