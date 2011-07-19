
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include "display.h"

static Display* display = NULL;
static Visual* visual = NULL;
static XImage* ximage = NULL;
static Window window;
static int width = 0;
static int height = 0;

int refresh_display(__u8* p, size_t length)
{
	ximage = XCreateImage(display, visual, 24, ZPixmap, 0, (char*)p, width, height, 32, 0);

	XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 10, 10, width, height);

	return 0;
}

int close_display()
{
	return 0;
}

int open_display(int w, int h)
{
	display = XOpenDisplay(NULL);

	visual = DefaultVisual(display, 0);

	width = w;

	height = h;

	window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, width, height, 1, 0, 0);

	XSelectInput(display, window, ExposureMask);

	XMapWindow(display, window);

	return 0;
}
