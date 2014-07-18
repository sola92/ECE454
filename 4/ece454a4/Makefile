OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

server: libstubs_stuff server.o
	gcc server.o simplified_rpc/libstubs.a -o server

client-api.a: libstubs_stuff client_api.o
	ar r libclient-api.a client_api.o simplified_rpc/server_stub.o simplified_rpc/client_stub.o simplified_rpc/helper.o simplified_rpc/mybind.o

fs_client: client-api.a fs_client.o
	gcc fs_client.o libclient-api.a -o fs_client

libstubs_stuff:
	$(MAKE) -C simplified_rpc

$(OBJECTS): %.o: %.c ece454_fs.h
	gcc -c $< -o $@

clean:
	$(MAKE) -C simplified_rpc clean
	rm -rf a.out *.o core *.a fs_client server