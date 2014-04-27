all: hellomake hellosignal

hellomake: main.c
	gcc --static -o main main.c -lpthread -lsigsegv -I.

hellosignal: signals.c
	gcc --static -o signals signals.c

clean:
	rm -rf *o main signals
