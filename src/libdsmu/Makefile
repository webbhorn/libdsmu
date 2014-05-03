all: libdsmu

libdsmu: rpc b64 mem libdsmu.c libdsmu.h
	gcc -c libdsmu.c -o libdsmu.o
	gcc -o libdsmu b64.o rpc.o libdsmu.o -lpthread -I. -lb64

mem: mem.h

b64: b64.h b64.c
	gcc -c b64.c -o b64.o

rpc: b64 rpc.c rpc.h mem.h
	gcc -c rpc.c -o rpc.o -I.

clean:
	rm -rf *o libdsmu
