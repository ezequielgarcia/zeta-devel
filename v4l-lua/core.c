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


#include "core.h"

/* TODO LIST: 
 * 
 * - Make every control (contrast, brightness) value go from 0 to 100 
 * - Check error handling, seems sloppy: functions should return error code and
 *   returned value (if needed) through reference.
 *
 * - Implement capability querying, in a lazy way! 
 *     For video standard, 
 *     return a list of strings, this same string should be used to select the standard
 *     For user controls, we could do it in the same fashion,
 *     returning a list of strings (or table of strings?) and implement a function
 *     parameter(name, value) or something like that.  
 *
 * - Implement input selection, whenever a new input is selected 
 *   the _standards array should be erased, right?
 *
 * - Implement release _standards and _controls
 *
 *
 */

static int _fd = -1;
static int _width = 0, _height = 0;
static long _imagesize = 0;
static struct control** _controls = NULL;
static size_t _controls_count = 0;
static struct standard** _standards = NULL;
static size_t _standards_count = 0;

static struct v4lconvert_data*v4lconvert_data;
static struct v4l2_format src_fmt;    /* raw format */
static struct v4l2_format fmt;
static struct v4l2_buffer buf;
static unsigned char *dst_buf;
static const char *dev_name;
static struct buffer *buffers;
static int n_buffers;

static
int xioctl(long request, void *arg)
{
    int r;

    do {
        r = ioctl(_fd, request, arg);
    } while (r < 0 && EINTR == errno);
    return r;
}

static
void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    fprintf(stderr, "%s\n", v4lconvert_get_error_message(v4lconvert_data));
    exit(EXIT_FAILURE);
}

static                                            
void process_image(unsigned char *p, int len)
{

    if(v4lconvert_convert(v4lconvert_data,
                           &src_fmt,
                           &fmt,
                           p, len,
                           dst_buf,
                           fmt.fmt.pix.sizeimage) < 0)
   {
       if(errno != EAGAIN)
       {
           perror("v4l_convert");
       }
        p = dst_buf;
        len = fmt.fmt.pix.sizeimage;
    }
}

static
int read_frame()
{
    memset(&(buf), 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(VIDIOC_DQBUF, &buf) < 0) {
        switch (errno) {
            case EAGAIN:
                return 0;
                break;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                /*errno_exit("VIDIOC_DQBUF");*/
                perror("VIDIOC_DQBUF");
        }
    }
        
    assert((unsigned char)buf.index < n_buffers);

    if (xioctl(VIDIOC_QBUF, &buf) < 0) 
        perror("VIDIOC_QBUF");
        /*errno_exit("VIDIOC_QBUF");*/

    return 0;
}

static 
int wait_frame(void)
{
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(_fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(_fd + 1, &fds, NULL, NULL, &tv);
    if(r < 0)
    {
        if(EINTR == errno)
            return -1;

        perror("select");
        /*errno_exit("select");*/
    }

    if(0 == r)
    {
        perror("select timeout");
        /*exit(EXIT_FAILURE);*/
        return -1;
    }
    
    return 0;
}

static 
void init_mmap()
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    memset(&(req), 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(xioctl( VIDIOC_REQBUFS, &req) < 0)
    {
        if(EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }


    buffers = (struct buffer*) calloc(req.count, sizeof(struct buffer));

    if(!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        perror("EXIT_FAILURE");
    }

    for(n_buffers = 0; n_buffers < (unsigned char)req.count; ++n_buffers)
    {
        memset(&(buf), 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if(xioctl( VIDIOC_QUERYBUF, &buf) < 0)
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
                        buf.length,
                        PROT_READ | PROT_WRITE
                        /* required */ ,
                        MAP_SHARED
                        /* recommended */ ,
                        _fd, buf.m.offset);

        if(MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

static
void query_standards()
{
	struct v4l2_input input;
	struct v4l2_standard std;

	memset (&input, 0, sizeof (input));

	/* TODO: Why query input, oh why? */

	if (xioctl(VIDIOC_G_INPUT, &input.index) < 0) {
		perror ("VIDIOC_G_INPUT");
	}

	if (xioctl(VIDIOC_ENUMINPUT, &input) < 0) {
		perror ("VIDIOC_ENUM_INPUT");
	}

	/* This function should only be called if _standards is NULL,
		otherwise a memory leak may result */
	_standards_count = 0;
	_standards = NULL;

	memset (&std, 0, sizeof(struct v4l2_standard));
	std.index = 0;

	while ( !xioctl(VIDIOC_ENUMSTD, &std) ) {

		if (std.id & input.std) {

			/* alloc place for next standard */
			_standards = realloc(_standards, (_standards_count+1) * sizeof(struct standard*));

			/* save standard information */
			_standards[_standards_count] = (struct standard*)malloc(sizeof(struct standard));
			_standards[_standards_count]->id = std.id;
			_standards[_standards_count]->name = strdup((char*)std.name);
			_standards_count++;
		}

		std.index++;
	}

	/* EINVAL indicates the end of the enumeration */

	if (errno != EINVAL) {
		perror ("VIDIOC_ENUMSTD");
	}
}

static
void query_controls()
{
	struct v4l2_queryctrl queryctrl;
	
	/* This function should only be called if _controls is NULL,
		otherwise a memory leak may result */
	_controls_count = 0;
	_controls = NULL;

	memset (&queryctrl, 0, sizeof (queryctrl));
	queryctrl.id = V4L2_CID_BASE;

	for (; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++) {

		if ( !xioctl(VIDIOC_QUERYCTRL, &queryctrl) ) {

			/* continue if disabled */
			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;

			/* alloc place for next control */
			_controls = realloc(_controls, (_controls_count+1) * sizeof(struct control*));

			/* save control information */
			_controls[_controls_count] = (struct control*)malloc(sizeof(struct control));
			_controls[_controls_count]->id = queryctrl.id;
			_controls[_controls_count]->name = strdup((char*)queryctrl.name);
			_controls_count++;

			if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
				;
			}
		} 
		else {

			/* continue if unsupported */
			if (errno == EINVAL)
				continue;

			perror ("VIDIOC_QUERYCTRL");
		}
	}
}

/* exported functions */

int get_imagesize()
{
    return _imagesize;
}

int get_width()
{
    return _width;
}

int get_height()
{
    return _height;
}

unsigned char *newframe()
{
    wait_frame();
	read_frame();
    process_image((unsigned char *)buffers[buf.index].start, buf.bytesused);
    return dst_buf;
}

size_t list_standards(char*** arr_standards)
{
	size_t i;

	/* is _standards is empty, fill it */
	if (!_standards)
		query_standards();

	/* _standards is already filled up, 	
		this is the intended lazy behavior */

	if (!arr_standards || !_standards)
		return _standards_count;

	for (i=0; i < _standards_count; i++) 
		(*arr_standards)[i] = _standards[i]->name;

	return _standards_count;
}

int get_standard(char** name)
{
	v4l2_std_id std_id;
	struct v4l2_standard standard;

	*name = NULL;

	/* Get current standard id */
	if (xioctl(VIDIOC_G_STD, &std_id) < 0) {

		/* No standard */
		
		return 1;
	}

	memset (&standard, 0, sizeof (standard));
	standard.index = 0;

	/* Get standard name from id */
	while (xioctl(VIDIOC_ENUMSTD, &standard) == 0) {

		if (standard.id & std_id) {
			/* This is current standard, caller must release! */
			*name = strdup((const char*)standard.name);
			break;
		}

		standard.index++;
	}

	/* Found standard name? */
	if (*name == NULL)
		return 1;

	return 0;
}

void set_standard(const char* name)
{
	size_t i;
	int found;
	v4l2_std_id std_id;

	/* In case _standards is empty, query_standards */
	if (!_standards)
		query_standards();

	/* Search standard id from name */
	found = 0;
	for (i=0; i < _standards_count; i++) {

		if (!strcmp(_standards[i]->name, name)) {
			found = 1;
			std_id = _standards[i]->id;
			break;
		}
	}

	/* No such standard name */
	if (!found)
		return;

	if (xioctl(VIDIOC_S_STD, &std_id) < 0) 
		perror ("VIDIOC_S_STD");
}

size_t list_controls(char*** arr_controls)
{
	size_t i;

	/* is _controls is empty, fill it */
	if (!_controls)
		query_controls();

	/* _controls is already filled up, 	
		this is the intended lazy behavior */

	if (!arr_controls || !_controls)
		return _controls_count;

	for (i=0; i < _controls_count; i++) 
		(*arr_controls)[i] = _controls[i]->name;

	return _controls_count;
}

void set_control(const char* name, int value)
{
	struct v4l2_control control;
	int found, id;
	size_t i;

	/* In case _controls is empty, query_controls */
	if (!_controls)
		query_controls();

	/* Search control name */
	found = 0;
	for (i=0; i < _controls_count; i++) {

		if (!strcmp(_controls[i]->name, name)) {
			found = 1;
			id = _controls[i]->id;
			break;
		}
	}

	/* No such control name */
	if (!found)
		return;

	memset (&control, 0, sizeof (control));
	control.id = id;
	control.value = value;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_S_CTRL, &control) < 0 && errno != ERANGE) 
		perror ("VIDIOC_S_CTRL");
}

int get_control(const char* name, int* value)
{
	struct v4l2_control control;
	int found, id;
	size_t i;

	/* In case _controls is empty, query_controls */
	if (!_controls)
		query_controls();

	/* Search control name */
	found = 0;
	for (i=0; i < _controls_count; i++) {

		if (!strcmp(_controls[i]->name, name)) {
			found = 1;
			id = _controls[i]->id;
			break;
		}
	}

	/* No such control name */
	if (!found)
		return 1;

	memset (&control, 0, sizeof(struct v4l2_control));
	control.id = id;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_G_CTRL, &control) < 0 && errno != ERANGE) {
		perror ("VIDIOC_S_CTRL");
		return 1;
	}

	*value = control.value;
	
	/* OK */
	return 0;
}

void stop_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(VIDIOC_STREAMOFF, &type) < 0)
		perror("VIDIOC_STREAMOFF");
}

void start_capturing(void)
{
	int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i)
	{
		memset(&(buf), 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if(xioctl( VIDIOC_QBUF, &buf) < 0)
			perror("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(xioctl( VIDIOC_STREAMON, &type) < 0)
		perror("VIDIOC_STREAMON");
}

void uninit_device()
{
    int i;

    for(i = 0; i < n_buffers; ++i)
        if(-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");
    
    if(dst_buf != NULL)
        free(dst_buf);
              
    if(buffers != NULL)
        free(buffers);
}

void init_device()
{
	struct v4l2_capability cap;
	int ret;

	if (xioctl(VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			perror("EXIT_FAILURE");
			return;
		} 
		else {
			perror("VIDIOC_QUERYCAP");
			return ;
		}
	}

	if ( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		/*exit(EXIT_FAILURE);*/
		perror("EXIT_FAILURE");
		return;
	}

	memset(&(fmt), 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/* We dont know the size yet! ! */
	fmt.fmt.pix.width = _width;
	fmt.fmt.pix.height = _height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	v4lconvert_data = v4lconvert_create(_fd);

	if (v4lconvert_data == NULL) {
		perror("v4lconvert_create");
		return;
	}

	if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0) {
		/*errno_exit("v4lconvert_try_format");*/
		perror("v4lconvert_try_format");
		return;
	}

	ret = xioctl(VIDIOC_S_FMT, &src_fmt);
	dst_buf = (unsigned char *)malloc(fmt.fmt.pix.sizeimage);

#ifdef DEBUG

	printf("raw pixfmt: %c%c%c%c %dx%d\n",
			src_fmt.fmt.pix.pixelformat & 0xff,
			(src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
			(src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
			(src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
			src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);
#endif    

	if (ret < 0) {
		perror("VIDIOC_S_FMT");
		return;
	}

#ifdef DEBUG
	printf("pixfmt: %c%c%c%c %dx%d\n",
			fmt.fmt.pix.pixelformat & 0xff,
			(fmt.fmt.pix.pixelformat >> 8) & 0xff,
			(fmt.fmt.pix.pixelformat >> 16) & 0xff,
			(fmt.fmt.pix.pixelformat >> 24) & 0xff,
			fmt.fmt.pix.width, fmt.fmt.pix.height);

	/* Note VIDIOC_S_FMT may change width and height. */
#endif

	_imagesize = fmt.fmt.pix.sizeimage;
	_width = fmt.fmt.pix.width;
	_height = fmt.fmt.pix.height;

	/* Transfer method is mmap */
	init_mmap();
}

int close_device(int dev)
{
    return close(dev);
}

int open_device(const char *dev)
{
    struct stat st;

    if (stat(dev, &st) < 0) {
#ifdef DEBUG    	    
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev, errno, strerror(errno));
#endif
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
#ifdef DEBUG    	    
        fprintf(stderr, "%s is no device\n", dev);
#endif
        return -1;
    }

    _fd = open(dev, O_RDWR /* required */  | O_NONBLOCK, 0);
    
    if (_fd < 0) {
#ifdef DEBUG    	    
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev, errno, strerror(errno));
#endif
        return -1;
    }

    return _fd;
}

/* Deprecated implementation */

#if 0
int get_brightness()
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_BRIGHTNESS;

	if (xioctl(VIDIOC_G_CTRL, &control) < 0) {
		/* V4L2_CID_BRIGHTNESS is unsupported */
		perror ("VIDIOC_G_CTRL");
		return 0;
	}

	return control.value;
}

void set_brightness(int value)
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_BRIGHTNESS;
	control.value = value;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_S_CTRL, &control) < 0 && errno != ERANGE) 
		perror ("VIDIOC_S_CTRL");
}

int get_contrast()
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_CONTRAST;

	if (xioctl(VIDIOC_G_CTRL, &control) < 0) {
		/* V4L2_CID_CONTRAST is unsupported */
		perror ("VIDIOC_G_CTRL");
		return 0;
	}

	return control.value;
}

void set_contrast(int value)
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_CONTRAST;
	control.value = value;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_S_CTRL, &control) < 0 && errno != ERANGE) 
		perror ("VIDIOC_S_CTRL");
}

int get_hue()
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_HUE;

	if (xioctl(VIDIOC_G_CTRL, &control) < 0) {
		/* V4L2_CID_HUE is unsupported */
		perror ("VIDIOC_G_CTRL");
		return 0;
	}

	return control.value;
}

void set_hue(int value)
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_HUE;
	control.value = value;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_S_CTRL, &control) < 0 && errno != ERANGE) 
		perror ("VIDIOC_S_CTRL");
}

int get_saturation()
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SATURATION;

	if (xioctl(VIDIOC_G_CTRL, &control) < 0) {
		/* V4L2_CID_SATURATION is unsupported */
		perror ("VIDIOC_G_CTRL");
		return 0;
	}

	return control.value;
}

void set_saturation(int value)
{
	struct v4l2_control control;

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SATURATION;
	control.value = value;

	/* The driver may clamp the value or return ERANGE, ignored here */
	if (xioctl(VIDIOC_S_CTRL, &control) < 0 && errno != ERANGE) 
		perror ("VIDIOC_S_CTRL");
}

#endif
