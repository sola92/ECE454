#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdarg.h>
#include "ece454rpc_types.h"


/*
 * Following C Coding Standard from CMU.
 * http://users.ece.cmu.edu/~eno/coding/CCodingStandard.html
 */
BYTE buffer[500];               /* used to store data from outgoing requests */
const int REQUEST_LENGTH = 500; /* length of buffer declared above */

return_type ret;
return_type make_remote_call(const char *servernameorip,
                            const int serverportnumber,
                            const char *procedure_name,
                            const int nparams, ...) {
    va_list va_args;
    va_start (va_args, nparams);

    /* Try opening a socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("%s", "ERROR opening socket\n");
        va_end (va_args);
        return ret;
    } /* couldn't open a socket */

    /* Try parsing the given IP address */
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    if(!inet_aton(servernameorip, &remote_addr.sin_addr)) {
        printf("%s", "ERROR can't parse ip address\n");
        va_end(va_args);
        return ret;
    } /* couldn't parse the IP address */

    remote_addr.sin_port = serverportnumber;
    if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        printf("%s", "ERROR connecting\n");
        va_end(va_args);
        return ret;
    } /* couldn't connect to the server */

    BYTE *ptr = buffer; /* pointer used to iterate through the buffer */

    /* Write proc name to buffer. */
    int procedure_name_length = strlen(procedure_name) + 1;
    strcpy(ptr, procedure_name);
    ptr += procedure_name_length;

    /* Write nparams to buffer. */
    *(int *)ptr= nparams;
    ptr += sizeof(int);

    /* Write the parameters into to the buffer. */
    int i;
    for (i = 0; i < nparams; i++) {
        /* Write the param length into to buffer. */
        int val_len = va_arg(va_args, int);
        *(int *)ptr = val_len;
        ptr += sizeof(int);

        /* Write the param value into to buffer. */
        void *val_ptr = va_arg(va_args, void*);
        memcpy(ptr, val_ptr, val_len);
        ptr += val_len;
    }

    /* Try writing the buffer into the socket. */
    if(write(sockfd, buffer, REQUEST_LENGTH) < 0) {
        printf("%s", "ERROR write request bytes\n");
        va_end(va_args);
        return ret;
    } /* we couldn't write data out */

    char buffer[500] = { 0 };

    if (read(sockfd, buffer, REQUEST_LENGTH) < 0) {
        printf("%s", "ERROR reading from socket\n");
        va_end(va_args);
        return ret;
    } /* we couldn't read the response */

    ptr = buffer;
    ret.return_size = *(int *)ptr;
    ptr += sizeof(int);
    ret.return_val = (void *)ptr;

    va_end(va_args); /* free memory for va_args list */
    return ret;
}