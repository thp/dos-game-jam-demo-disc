#ifndef VIDEO_H_
#define VIDEO_H_

int vid_setmode(int mode);

void vid_setcolor(int idx, int r, int g, int b);
void vid_setcolors(int idx, int ncol, unsigned char *rgb);

void vid_vsync(void);
#pragma aux vid_vsync = \
	"mov dx, 3dah" \
	"invb: in al, dx" \
	"and al, 8" \
	"jnz invb" \
	"novb: in al, dx" \
	"and al, 8" \
	"jz novb" \
	modify [al dx];

#endif	/* VIDEO_H_ */
