
CC=gcc
CFLAGS=-Wall
LDFLAGS=-lv4lconvert

WITH_DFB=0
WITH_XLIB=1

ifeq "$(WITH_DFB)" "1"
  CFLAGS=-I/usr/include/directfb
  LDFLAGS+=-ldirectfb 
  objects=dfb_draw.o
endif

ifeq "$(WITH_XLIB)" "1"
  LDFLAGS+=-lX11
  objects=xlib_draw.o
endif

capture: $(objects)

.c.o:

.PHONY: clean
clean: 
	rm -f capture *.o
