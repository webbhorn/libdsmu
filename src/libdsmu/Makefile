CC = gcc
CFLAGS = -Wall
INCLUDES = -I.
LFLAGS =
LIBS = -lb64 -lpthread

SRCS = b64.c libdsmu.c main.c rpc.c
OBJS = $(SRCS:.c=.o)

MAIN = main

.PHONY: depend clean

all: $(MAIN)
	@echo Build complete.

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c: .o
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend:
	$(SRCS)
	makedepend $(INCLUDES) $^
