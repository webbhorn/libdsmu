all: hellomake hellosignal serverdemo clientdemo

hellomake: main.c
	gcc -o main main.c -lpthread -lsigsegv -I.

hellosignal: signals.c
	gcc -o signals signals.c -lb64

serverdemo: server.c
	gcc -o server server.c

clientdemo: client.c
	gcc -o client client.c

clean:
	rm -rf *o main signals server client
