OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

server: libstubs_stuff fsServer.o client_api.o
	gcc fsServer.o client_api.o simplified_rpc/libstubs.a -o fsServer

client-api.a: libstubs_stuff client_api.o
	ar r client-api.a client_api.o simplified_rpc/server_stub.o simplified_rpc/client_stub.o simplified_rpc/helper.o simplified_rpc/mybind.o

fs_client: client-api.a fs_client.o
	gcc fs_client.o client-api.a -o fs_client

libstubs_stuff:
	$(MAKE) -C simplified_rpc

$(OBJECTS): %.o: %.c ece454_fs.h
	gcc -c $< -o $@

clean:
	rm -rf a.out *.o core *.a fs_client