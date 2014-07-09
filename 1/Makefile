objects := $(patsubst %.c,%.o,$(wildcard *.c))

server.out: libstubs.a myserver.o
	gcc myserver.o -L. -lstubs -o server.out

client.out: libstubs.a myclient.o
	gcc myclient.o -L. -lstubs -o client.out

libstubs.a: server_stub.o client_stub.o
	ar r libstubs.a server_stub.o client_stub.o

$(objects): %.o: %.c ece454rpc_types.h
	gcc -c $< -o $@

clean:
	rm -rf *.out *.o core *.a