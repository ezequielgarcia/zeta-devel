#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <libv4lconvert.h>
#include <signal.h>
#include "display.h"

static struct v4lconvert_data *v4lconvert_data;
static struct v4l2_format src_fmt;	/* raw format */
static struct v4l2_format fmt;		/* directfb format */
static __u8* dst_buf;

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define VIDEO_WIDTH	640
#define VIDEO_HEIGHT	480

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void* start;
	size_t length;
};

static int 		alive 		= 1;
static char*		dev_name        = NULL;
static io_method	io		= IO_METHOD_MMAP;
static int		fd              = -1;
struct buffer*		buffers		= NULL;
static unsigned int	n_buffers 	= 0;

static void errno_exit(const char* s)
{
	fprintf (stderr, "%s error %d, %s\n",
			s, errno, strerror (errno));

	exit (EXIT_FAILURE);
}

static int xioctl(int fd, int request, void* arg)
{
	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

static void process_image (__u8* p, size_t length)
{
	if (v4lconvert_convert(v4lconvert_data,
				&src_fmt,
				&fmt,
				p, length,
				dst_buf, fmt.fmt.pix.sizeimage) < 0) 
	{
		if (errno != EAGAIN)
			errno_exit("v4l_convert");
		return;
	}

	refresh_display(dst_buf, fmt.fmt.pix.sizeimage);
}

static int read_frame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {

		case IO_METHOD_READ:
			i = read(fd, buffers[0].start, buffers[0].length);
			if (i < 0) {
				switch (errno) {
					case EAGAIN:
						return 0;
					case EIO:
						/* Could ignore EIO, see spec. */
						/* fall through */
					default:
						errno_exit("read");
				}
			}
			process_image(buffers[0].start, i);
			break;

		case IO_METHOD_MMAP:
			CLEAR (buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
					case EAGAIN:
						return 0;

					case EIO:
						/* Could ignore EIO, see spec. */

						/* fall through */

					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}

			assert (buf.index < n_buffers);

			process_image (buffers[buf.index].start, buffers[buf.index].length);

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");

			break;

		case IO_METHOD_USERPTR:
			CLEAR (buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;

			if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
					case EAGAIN:
						return 0;

					case EIO:
						/* Could ignore EIO, see spec. */

						/* fall through */

					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}

			for (i = 0; i < n_buffers; ++i)
				if (buf.m.userptr == (unsigned long) buffers[i].start
						&& buf.length == buffers[i].length)
					break;

			assert (i < n_buffers);

			process_image ((void *) buf.m.userptr, buf.length);

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");

			break;
	}

	return 1;
}

static void mainloop(void)
{
	while (alive) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO (&fds);
			FD_SET (fd, &fds);

			// Timeout. 
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select (fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;

				errno_exit ("select");
			}

			if (0 == r) {
				fprintf (stderr, "select timeout\n");
				exit (EXIT_FAILURE);
			}
	
			if (read_frame ())
				break;

			// EAGAIN - continue select loop. 
		}
	}
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;

	switch (io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
				errno_exit ("VIDIOC_STREAMOFF");

			break;
	}
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR (buf);

				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_MMAP;
				buf.index       = i;

				if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
					errno_exit ("VIDIOC_QBUF");
			}

			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");

			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR (buf);

				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_USERPTR;
				buf.index       = i;
				buf.m.userptr	= (unsigned long) buffers[i].start;
				buf.length      = buffers[i].length;

				if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
					errno_exit ("VIDIOC_QBUF");
			}

			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");

			break;
	}
}

static void uninit_device(void)
{
	unsigned int i;

	switch (io) {
		case IO_METHOD_READ:
			free (buffers[0].start);
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
				if (-1 == munmap (buffers[i].start, buffers[i].length))
					errno_exit ("munmap");
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
				free (buffers[i].start);
			break;
	}

	free (buffers);
}

static void init_read(unsigned int buffer_size)
{
	buffers = calloc (1, sizeof (*buffers));

	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);

	if (!buffers[0].start) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					"memory mapping\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
				dev_name);
		exit (EXIT_FAILURE);
	}

	buffers = calloc (req.count, sizeof (*buffers));

	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap (NULL /* start anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit ("mmap");
	}
}

static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize ();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					"user pointer i/o\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc (4, sizeof (*buffers));

	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign (/* boundary */ page_size,
				buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}
	}
}

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_input input;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	int ret;

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
				dev_name);
		exit (EXIT_FAILURE);
	}

	switch (io) {
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf (stderr, "%s does not support read i/o\n",
						dev_name);
				exit (EXIT_FAILURE);
			}

			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf (stderr, "%s does not support streaming i/o\n",
						dev_name);
				exit (EXIT_FAILURE);
			}

			break;
	}


	/* Select video input, video standard and tune here. */

	CLEAR (input);
	if (0 == xioctl (fd, VIDIOC_G_INPUT, &input.index)) {

		if (0 == xioctl (fd, VIDIOC_ENUMINPUT, &input)) {

			if (-1 == (input.std & V4L2_STD_PAL_Nc)) {
				fprintf (stderr, "%s does not support PAL_Nc standard\n",
						dev_name);
				exit (EXIT_FAILURE);
			}
			else {

				v4l2_std_id std_id = V4L2_STD_PAL_Nc;

				if (-1 == xioctl (fd, VIDIOC_S_STD, &std_id)) {
					errno_exit ("VIDIOC_S_STD");
				}
			}
		}
	}
	else {
		errno_exit ("VIDIOC_G_INPUT");
	}

	CLEAR (cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {	
		/* Errors ignored. */
	}

	CLEAR (fmt);

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = VIDEO_WIDTH; 
	fmt.fmt.pix.height      = VIDEO_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	v4lconvert_data = v4lconvert_create(fd);
	if (v4lconvert_data == NULL)
		errno_exit("v4lconvert_create");
	if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
		errno_exit("v4lconvert_try_format");

	ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
	dst_buf = malloc(fmt.fmt.pix.sizeimage);
	printf("raw pixfmt: %c%c%c%c %dx%d\n",
		src_fmt.fmt.pix.pixelformat & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
	       (src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
		src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);

	if (ret < 0)
		errno_exit ("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	printf("pixfmt: %c%c%c%c %dx%d\n", fmt.fmt.pix.pixelformat & 0xff,
			(fmt.fmt.pix.pixelformat >> 8) & 0xff,
			(fmt.fmt.pix.pixelformat >> 16) & 0xff,
			(fmt.fmt.pix.pixelformat >> 24) & 0xff,
			fmt.fmt.pix.width, fmt.fmt.pix.height);

	printf("sizeimage %d\n", fmt.fmt.pix.sizeimage);
	printf("bytesperline %d\n", fmt.fmt.pix.bytesperline);

	switch (io) {
		case IO_METHOD_READ:
			init_read (fmt.fmt.pix.sizeimage);
			break;

		case IO_METHOD_MMAP:
			init_mmap ();
			break;

		case IO_METHOD_USERPTR:
			init_userp (fmt.fmt.pix.sizeimage);
			break;
	}
}

static void close_device(void)
{
	if (close(fd) < 0)
		errno_exit ("close");

	fd = -1;
}

static void open_device(void)
{
	struct stat st; 

	if (stat (dev_name, &st) < 0) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
				dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", dev_name);
		exit (EXIT_FAILURE);
	}

	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (fd < 0) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
				dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void handler(int sig)
{
	printf("Signal %d caught\n", sig);

	alive = 0;

	stop_capturing();

	uninit_device();

	close_device();

	close_display();

	signal(SIGINT, SIG_DFL);

	kill(getpid(), SIGINT);
}

int main (int argc, char** argv)
{
	dev_name = "/dev/video0";

	io = IO_METHOD_MMAP;

	open_display(VIDEO_WIDTH, VIDEO_HEIGHT);

	printf("Opening device\n");
	open_device();

	printf("Initing device\n");
	init_device();

	printf("Starting capture\n");
	start_capturing();

	signal(SIGINT, handler);

	// Code will never leave this call
	printf("Mainloop...\n");
	mainloop();

	return 0;
}

