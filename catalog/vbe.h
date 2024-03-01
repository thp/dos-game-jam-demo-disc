/* Real-mode VBE 1.x code by John Tsiombikas <nuclear@mutantstargoat.com>
 * This code is released into the public domain, do whatever you want with it.
 */
#ifndef VBE_H_
#define VBE_H_

#define VBE_MAJOR(ver)		((ver) >> 8)
#define VBE_MINOR(ver)		((ver) & 0xff)

struct vbe_info {
	unsigned long sig;
	unsigned short ver;
	char far *oemname;
	unsigned long caps;
	unsigned short far *modes;
	unsigned short vmem;
	unsigned char rsvd[512 - 20];
} _Packed;

/* vbe_info caps bits */
enum {
	VBE_8BITDAC		= 1,
	VBE_NONVGA		= 2,
	VBE_DACBLANK	= 4
};

struct vbe_mode_info {
	unsigned short attr;
	unsigned char wina_attr, winb_attr;
	unsigned short win_gran;
	unsigned short win_size;
	unsigned short wina_seg, winb_seg;
	unsigned long setwin_func_addr;
	unsigned short scan_size;
	unsigned char rsvd[256 - 18];
} _Packed;

int vbe_getinfo(struct vbe_info *vbe);
int vbe_getmodeinfo(int mode, struct vbe_mode_info *minf);

int vbe_setmode(int mode);

void vbe_setwin(int offs);

void vbe_setcolor(int idx, int r, int g, int b);
void vbe_setcolors(int idx, int count, unsigned char *cmap);

void vbe_vsync(void);
#pragma aux vbe_vsync = \
	"mov dx, 0x3da" \
	"invb:" \
	"in al, dx" \
	"and al, 8" \
	"jnz invb" \
	"novb:" \
	"in al, dx" \
	"and al, 8" \
	"jz novb" \
	modify [al dx];


#endif	/* VBE_H_ */
