CC = gcc
INCLUDES = -I../include
CFLAGS = -Wall $(INCLUDES)
LFLAGS =
LIBS = -lb64 -lpthread

SRCS = b64.c libdsmu.c main.c rpc.c
OBJS = $(SRCS:.c=.o)

MAIN = main

.PHONY: depend clean

all: $(MAIN)
	@echo Build complete.

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c: .o
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o *~

depend:
	$(SRCS)
	makedepend $(INCLUDES) $^