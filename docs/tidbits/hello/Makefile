CFLAGS = -Wall
LIBS = -lcpic
CC = gcc

all: hello hellod

hello: hello.o
	${CC} ${CFLAGS} -o hello hello.o ${LIBDIRS} ${LIBS}

hellod: hellod.o
	${CC} ${CFLAGS} -o hellod hellod.o ${LIBDIRS} ${LIBS}

clean:
	rm -f *.o core hello hellod
