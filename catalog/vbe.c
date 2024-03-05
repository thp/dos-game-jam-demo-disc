/* Real-mode VBE 1.x code by John Tsiombikas <nuclear@mutantstargoat.com>
 * This code is released into the public domain, do whatever you want with it.
 */
#include <conio.h>
#if defined(DJGPP)
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#else
#include <i86.h>
#endif
#include "vbe.h"

int vbe_getinfo(struct vbe_info *vbe)
{
	// Do NOT request VBE 2.0 info
	vbe->sig = 0;

#if defined(DJGPP)
	// http://www.delorie.com/djgpp/v2faq/faq18_2.html
	__dpmi_regs regs;

	dosmemput(vbe, sizeof(*vbe), __tb);

	regs.x.ax = 0x4f00;
	regs.x.es = (__tb >> 4) & 0xFFFF;
	regs.x.di = __tb & 0x0F;
	__dpmi_int(0x10, &regs);

	if (regs.x.ax == 0x4f) {
		dosmemget(__tb, sizeof(*vbe), vbe);
		return 0;
	}
        return -1;
#else
	union REGS regs = {0};
	struct SREGS sregs = {0};

	regs.x.ax = 0x4f00;
	sregs.es = FP_SEG(vbe);
	regs.x.di = FP_OFF(vbe);
	int86x(0x10, &regs, &regs, &sregs);

	return regs.x.ax == 0x4f ? 0 : -1;
#endif
}

int vbe_getmodeinfo(int mode, struct vbe_mode_info *minf)
{
#if defined(DJGPP)
	// http://www.delorie.com/djgpp/v2faq/faq18_2.html
	__dpmi_regs regs;

	regs.x.ax = 0x4f01;
	regs.x.cx = mode;
	regs.x.es = (__tb >> 4) & 0xFFFF;
	regs.x.di = __tb & 0x0F;
	__dpmi_int(0x10, &regs);

	if (regs.x.ax == 0x4f) {
		dosmemget(__tb, sizeof(*minf), minf);
		return 0;
	}
	return -1;
#else
	union REGS regs = {0};
	struct SREGS sregs = {0};

	regs.x.ax = 0x4f01;
	regs.x.cx = mode;
	sregs.es = FP_SEG(minf);
	regs.x.di = FP_OFF(minf);
	int86x(0x10, &regs, &regs, &sregs);

	return regs.x.ax == 0x4f ? 0 : -1;
#endif
}


int vbe_setmode(int mode)
{
	union REGS regs = {0};

	if(mode < 0x100) {
		regs.x.ax = mode;
	} else {
		regs.x.ax = 0x4f02;
		regs.x.bx = mode;
	}
	int86(0x10, &regs, &regs);

	return regs.x.ax == 0x4f ? 0 : -1;
}

void vbe_setwin(int offs)
{
	union REGS regs = {0};

	regs.x.ax = 0x4f05;
	regs.x.dx = offs;
	int86(0x10, &regs, &regs);
}

void vbe_setcolor(int idx, int r, int g, int b)
{
	outp(0x3c8, idx);
	outp(0x3c9, r >> 2);
	outp(0x3c9, g >> 2);
	outp(0x3c9, b >> 2);
}

void vbe_setcolors(int idx, int count, unsigned char *cmap)
{
	outp(0x3c8, idx);

	while(count-- > 0) {
		outp(0x3c9, cmap[0] >> 2);
		outp(0x3c9, cmap[1] >> 2);
		outp(0x3c9, cmap[2] >> 2);
		cmap += 3;
	}
}
