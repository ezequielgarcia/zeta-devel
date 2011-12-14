DFB_HEADER := $(shell pkg-config --variable=includedir directfb)/directfb/directfb.h

CFLAGS := -Wall -fPIC $(shell pkg-config --cflags directfb lua)
LDFLAGS := -shared $(shell pkg-config --libs directfb lua)

SRC_DIR := src/
SRC := $(wildcard $(SRC_DIR)*.c)

OUTPUT=directfb.so

$(OUTPUT): $(SRC)
	gcc $(CFLAGS) $(SRC) $(LDFLAGS) -o $@ 

.PHONY: clean
clean:
	rm -f *.so

.PHONY: gen
gen:
	cat $(DFB_HEADER) | ./gendfb-lua.pl
