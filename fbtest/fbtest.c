#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define MARGIN					100
#define DEFAULT_MAX_RECT_COUNT	10000

static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static char*  fbp = NULL;

#define set_pixel(x,y,R,G,B,A) 										\
	do { 															\
		long location;												\
		location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +	\
			(y+vinfo.yoffset) * finfo.line_length;					\
			*(fbp + location) = B;									\
			*(fbp + location + 1) = G;  							\
			*(fbp + location + 2) = R; 								\
			*(fbp + location + 3) = A;  							\
	} while (0);													\

void draw_rectangle(size_t top, size_t left, size_t width, size_t height, char R, char G, char B)
{
	size_t right = left+width;
	size_t bottom = top+height;
	size_t x,y;

//	printf("Drawing rect at top=%lu, left=%lu, bottom=%lu, right=%lu.\n", top, left, bottom, right);

	for (x=left; x<right; x++) 
		for (y=top; y<bottom; y++) 
			set_pixel(x,y,R,G,B,0);
}

int main(int argc, char* argv[])
{
	size_t x,y,top,left,r,g,b;
	int fbfd = 0;
	long int screensize = 0;
	size_t max_rect_count;

	if (argc < 3) {
		printf("Usage: %s DEVICE RECTANGLE_COUNT\n", argv[0]);
		return 1;
	} 
	
	max_rect_count = atoi(argv[2]);

	// Open the file for reading and writing 
	fbfd = open(argv[1], O_RDWR);
	if (!fbfd) {
		printf("Error: cannot open framebuffer device.\n");
		return 1;
	}
	printf("The framebuffer device was opened successfully.\n");

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		return 1;
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		return 1;
	}

	printf("Framebuffer informationd:\n");
	printf("\tId: %s, accel id=%d\n", finfo.id, finfo.accel);
	printf("\txres=%d, yres=%d, bpp=%d.\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
	printf("\tvirtual_xres=%d, virtual_yres=%d.\n", vinfo.xres_virtual, vinfo.yres_virtual);
	printf("\txoffset=%d, yoffset=%d.\n", vinfo.xoffset, vinfo.yoffset);

	// Figure out the size of the screen in bytes
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	// Map the device to memory
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
			fbfd, 0);       
	if ((long)fbp == -1) { 
		printf("Error: failed to map framebuffer device to memory.\n"); 
		return 1;
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	printf("Are u ready ? [ENTER]");
	getchar();

	// Limpiar la pantalla
	system("clear");

	for (x=0; x<1280; x++) 
		for (y=0; y<1024; y++) 
			set_pixel(x, y, y*255/1024, y*255/1024, y*255/1024, 0);
	
	getchar();

	// Inicializar generador de numeros aleatorios
	srand(time(NULL));

	while(max_rect_count--) {

		top = rand() % (1024-2*MARGIN) + MARGIN;
		left = rand() % (1280-100) + 1;
		r = rand() % 255 + 1;
		g = rand() % 255 + 1;
		b = rand() % 255 + 1;
		draw_rectangle(top, left, 100, 100, r, g, b);
	}

	munmap(fbp, screensize);
	close(fbfd);

	return 0;
}
