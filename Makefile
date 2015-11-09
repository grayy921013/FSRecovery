CC=gcc
CFLAGS=-Wall

EXE=main
OBJ=main.o

all: ${EXE}

main: ${OBJ}

clean:
	rm -f ${OBJ} ${EXE}
