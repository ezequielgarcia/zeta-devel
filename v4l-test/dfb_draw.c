
#include <directfb.h>
#include "display.h"

static IDirectFB             *dfb = NULL;
static IDirectFBSurface      *surface = NULL;
static IDirectFBDisplayLayer *layer= NULL;

static int width = 0;
static int height = 0;

#define PIXEL_RGB32(a,r,g,b) 		\
		((a & 0xff ) << 24) |	\
		((r & 0xff ) << 16) | 	\
		((g & 0xff ) << 8)  |	\
		(b & 0xff ) 		\

int refresh_display(__u8* p, size_t length)
{
	DFBResult ret;
	DFBSurfacePixelFormat  format;
	size_t i,j;
	int w,h;
	int pitch;

	/* Retrieve the width and height. */
	surface->GetSize( surface, &w, &h);
	surface->GetPixelFormat( surface, &format );

	if (format != DSPF_RGB32) {
		printf("invalid dfb pixel format!\n");
		return 1;	
	}

	/* Lock the surface's data for direct write access. */
	ret = surface->Lock( surface, DSLF_WRITE, &data, &pitch );
	if (ret) {
		DirectFBError( "IDirectFBSurface::Lock() failed", ret );
		return 1;
	}

	for (j=0; j<h; j++) {
		__u32 *dst = (__u32*)((__u8*)data + j * pitch);

		for (i=0; i<w; i++) {

			// Set pixel with no alpha (a=255)
			dst[i] = PIXEL_RGB32(0xff, p[0], p[1], p[2]);

			p+=3;
		}
	}

	/* Unlock the surface's data. */
	surface->Unlock( surface );

	/* Flip the surface to display the new frame. */
	surface->Flip( surface, NULL, 0 );
}

int close_display ()
{
	if (surface)
		surface->Release(surface);
	if (layer)
		layer->Release(layer);

	return 0;
}

int open_display(int w, int h)
{
	DirectFBInit(NULL, NULL);

	DirectFBCreate( &dfb );

	width = w;
	height = h;

	dfb->SetCooperativeLevel(dfb, DFSCL_EXCLUSIVE);

	dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer);

	layer->GetSurface(layer, &surface);

	return 0;
}
