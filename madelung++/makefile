
CC=g++

CFLAGS=-Wall

LDFLAGS=-lm

OBJS=lattice.o ion.o main.o

OUTPUT=madelung

$(OUTPUT): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

.cc.o:

.PHONY: clean
clean:
	rm -f $(OBJS) $(OUTPUT)
