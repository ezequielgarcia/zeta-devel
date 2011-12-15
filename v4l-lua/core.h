/*
 Copyright (c) 2011 Gabriel Duarte <gabrield@impa.br>
 Copyright (c) 2011 Ezequiel Garcia <elezegarcia@yahoo.com.ar>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#ifndef CORE_H
#define CORE_H

#define WITH_V4L2_LIB 1		/* v4l library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>
#include <libv4lconvert.h>

typedef unsigned char BYTE;

struct buffer
{
	void*  start;
	size_t length;
};

struct standard
{
	char* name;
	v4l2_std_id id;
};

struct control
{
	char* name;
	__u32 id;
};

/* exported functions */

BYTE* newframe();

size_t list_controls(char*** arr_controls);
void set_control(const char*, int);
int get_control(const char*, int*);

void set_standard(const char*);
int get_standard(char**);
size_t list_standards(char*** arr_standards);

int get_imagesize();
int get_width();
int get_height();

int open_device(const char*);
int close_device(int);

void init_device();
void uninit_device(void);

void start_capturing(void);
void stop_capturing(void);

#endif /*CORE_H*/
