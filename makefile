CC=gcc
CFLAGS=-std=c11 -Wall -g
SRCS=ashell.c
LIBS=-lusb-1.0

ashell: $(SRCS)
	${CC} ${CFLAGS} ${SRCS} -o $@ ${LIBS}

clean:
	rm -f ashell
