all: libdsmu

libdsmu: libdsmu.c libdsmu.h
	gcc -o libdsmu libdsmu.c -lb64 -lpthread -I.

clean:
	rm -rf *o libdsmu
