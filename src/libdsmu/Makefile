all: hellomake hellosignal

hellomake: main.c
	gcc -o main main.c -lpthread -lsigsegv -I.

hellosignal: signals.c
	gcc -o signals signals.c

clean:
	rm -rf *o main
