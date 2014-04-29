all: hellomake hellosignal serverdemo clientdemo

hellomake: main.c
	gcc --static -o main main.c -lpthread -lsigsegv -I.

hellosignal: signals.c
	gcc --static -o signals signals.c

serverdemo: server.c
	gcc --static -o server server.c

clientdemo: client.c
	gcc --static -o client client.c

clean:
	rm -rf *o main signals server client
