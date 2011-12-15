
#DEBUG=-DDEBUG
CFLAGS=-Wall -pedantic -fPIC -I/usr/include/lua5.1
LDFLAGS=-shared -llua -lv4lconvert

SRC=v4l_lua.c core.c

OUTPUT=v4l.so

$(OUTPUT): $(SRC)
	gcc $(DEBUG) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@ 

.PHONY: clean
clean:
	rm -f *.so
