#include <stdio.h>
#include "ece454rpc_types.h"

int ret_int;
return_type r;

return_type add(const int nparams, arg_type* a)
{
    if(nparams != 2) {
	/* Error! */
	r.return_val = NULL;
	r.return_size = 0;
	return r;
    }

    if(a->arg_size != sizeof(int) ||
       a->next->arg_size != sizeof(int)) {
    	/* Error! */
    	r.return_val = NULL;
    	r.return_size = 0;
    	return r;
    }

    int i = *(int *)(a->arg_val);
    int j = *(int *)(a->next->arg_val);
    printf("Adding %d and %d result = %d \n", i, j, i + j);
    ret_int = i+j;
    r.return_val = (void *)(&ret_int);
    r.return_size = sizeof(int);
    return r;
}

return_type square(const int nparams, arg_type* a)
{
    if(a->arg_size != sizeof(int)) {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int i = *(int *)(a->arg_val);
    printf("Taking square of %d result = %d \n", i, i * i);
    ret_int = i * i;
    r.return_val = (void *)(&ret_int);
    r.return_size = sizeof(int);
    return r;
}

return_type rand_me(const int nparams, arg_type* a)
{
    printf("%s \n", "Fetching random");
    ret_int = 123;
    r.return_val = (void *)(&ret_int);
    r.return_size = sizeof(int);
    return r;
}

int main(int argc, char *argv[]) {
    register_procedure("addtwo", 2, add);
    register_procedure("square", 1, square);
    register_procedure("rand_me", 0, rand_me);
    launch_server();
    return 0;
}