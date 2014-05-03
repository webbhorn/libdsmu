all: main

main: libdsmu
	gcc -c main.c -o main.o
	gcc -o main main.o b64.o rpc.o libdsmu.o -lpthread -lb64 -I.

libdsmu: rpc b64 mem libdsmu.c libdsmu.h
	gcc -c libdsmu.c -o libdsmu.o -I.

mem: mem.h

b64: b64.h b64.c
	gcc -c b64.c -o b64.o -I.

rpc: b64 rpc.c rpc.h mem.h
	gcc -c rpc.c -o rpc.o -I.

clean:
	rm -rf *o libdsmu
