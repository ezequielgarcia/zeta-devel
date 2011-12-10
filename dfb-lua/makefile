CFLAGS=-Wall -fPIC -I/usr/include/lua5.1 -I/usr/include/directfb
LDFLAGS=-shared -llua -ldirect -ldirectfb -lfusion

SRC=src/core.c src/directfb.c src/surface.c

OUTPUT=directfb.so

$(OUTPUT): $(SRC)
	gcc $(CFLAGS) $(SRC) $(LDFLAGS) -o $@ 

.PHONY: clean
clean:
	rm -f *.so
