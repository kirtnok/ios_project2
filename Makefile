CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -g
LDFLAGS= -lrt -lpthread

.PHONY: clean all zip

all: proj2

proj2: proj2.o

proj2.o: proj2.c

zip:
	zip proj2.zip *.c Makefile

clean:
	rm -f *.o proj2 *.out

