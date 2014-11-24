CC=gcc
CFLAGS=-std=c11 -Wall -g -Wpedantic
SRCS=ashell.c
LIBS=-lusb-1.0 -lcrypto

ashell: $(SRCS)
	${CC} ${CFLAGS} ${SRCS} -o $@ ${LIBS}

clean:
	rm -f ashell
