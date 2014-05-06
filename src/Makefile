CC = gcc
INCLUDES = -I../include
CFLAGS = -Wall $(INCLUDES)
LFLAGS =
LIBS = -lb64 -lpthread

TESTS = main pingpong
SRCS = b64.c libdsmu.c rpc.c
OBJS = $(SRCS:.c=.o)

.PHONY: depend clean

all: $(TESTS)
	@echo Build complete.

main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

pingpong: pingpong.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

.c: .o
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o *~ $(TESTS)

depend:
	$(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend depends on it

