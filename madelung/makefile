
CC=gcc

CFLAGS=-Wall

LDFLAGS=-lm

OBJS=lattice.o ion.o main.o

OUTPUT=madelung

$(OUTPUT): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

.c.o:

.PHONY: clean
clean:
	rm -f $(OBJS) $(OUTPUT)
