
all: test libdummy.so

test: test.o
	gcc -g -o test test.o -ldl -rdynamic
	
test.o: test.c
	gcc -Wall -g -c test.c

dummy.o: dummy.c
	gcc -fPIC -Wall -g -c dummy.c

libdummy.so: dummy.o
	gcc -g -shared -o libdummy.so dummy.o

.PHONY: clean
clean:
	rm -f test libdummy.so *.o
