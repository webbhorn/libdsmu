CC = gcc
INCLUDES = -I../include
CFLAGS = -Wall $(INCLUDES)
LFLAGS =
LIBS = -lb64 -lpthread

TESTS = main pingpong pingpongpang matrixmultiply matrixmultiply2
SRCS = b64.c libdsmu.c rpc.c
OBJS = $(SRCS:.c=.o)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -g
endif

.PHONY: depend clean

all: $(TESTS)
	@echo Build complete.

main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

pingpong: pingpong.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

pingpongpang: pingpongpang.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

matrixmultiply: matrixmultiply.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

matrixmultiply2: matrixmultiply2.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LFLAGS) $(LIBS)

.c: .o
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o *~ $(TESTS)

depend:
	$(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend depends on it

