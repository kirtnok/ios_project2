CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -g
LDFLAGS= -lrt -lpthread

.PHONY: clean all zip

all: proj2

proj2: proj2.o

proj2.o: proj2.c proj2.h



zip:
	zip proj2.zip *.c *.h Makefile

clean:
	rm -f *.o water *.out

