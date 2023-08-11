#include "vidsys.h"
#include "drv.h"
#include "logger.h"

static struct vid_modeinfo *minf;

void reset_swcursor(void)
{
	minf = vid_modeinfo(vid_curmode());
}

void set_swcursor(int width, int height, int hotx, int hoty, void *pixels, void *mask, int pitch)
{
}

void draw_swcursor(int x, int y)
{
	unsigned char *fb8;

	dbgmsg("foo\n");

	switch(minf->bpp) {
	case 8:
		fb8 = (unsigned char*)vid_vmem + y * minf->pitch + x;
		fb8[0] = 0xff;
		fb8[1] = 0xff;
		fb8[2] = 0xff;
		fb8 += minf->pitch;
		fb8[0] = 0xff;
		fb8 += minf->pitch;
		fb8[0] = 0xff;
		break;
	}
}
