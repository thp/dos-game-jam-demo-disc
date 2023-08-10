#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "app.h"
#include "timer.h"
#include "keyb.h"
#include "vidsys.h"
#include "cdpmi.h"
#include "mouse.h"
#include "logger.h"
#include "rtk.h"

static unsigned char *vmem;
static int quit, dirty_valid;
static rtk_rect dirty;
static int mx, my, prev_mx, prev_my, use_mouse;


int main(int argc, char **argv)
{
	int i;
	int vmidx;
	int mdx, mdy, bnstate, bndiff;
	static int prev_bnstate;
	char *env;

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	init_logger();
	if((env = getenv("MENULOG"))) {
		if(tolower(env[0]) == 'c' && tolower(env[1]) == 'o' && tolower(env[2]) == 'm'
				&& isdigit(env[3])) {
			add_log_console(env);
		} else {
			add_log_file(env);
		}
	} else {
		add_log_file("menu.log");
	}

	if(!(use_mouse = have_mouse())) {
		infomsg("no mouse detected\n");
	}
	init_timer(0);
	kb_init();

	if(vid_init() == -1) {
		return 1;
	}

	if((vmidx = vid_findmode(SCR_WIDTH, SCR_HEIGHT, 8)) == -1) {
		return 1;
	}
	if(!(vmem = vid_setmode(vmidx))) {
		return 1;
	}

	if(app_init() == -1) {
		goto break_evloop;
	}
	app_invalidate(0, 0, 0, 0);

	mx = SCR_WIDTH / 2;
	my = SCR_HEIGHT / 2;
	prev_mx = prev_my = -1;

	for(;;) {
		int key;

		modkeys = 0;
		if(kb_isdown(KEY_ALT)) {
			modkeys |= KEY_MOD_ALT;
		}
		if(kb_isdown(KEY_CTRL)) {
			modkeys |= KEY_MOD_CTRL;
		}
		if(kb_isdown(KEY_SHIFT)) {
			modkeys |= KEY_MOD_SHIFT;
		}

		while((key = kb_getkey()) != -1) {
			if(key == 'r' && (modkeys & KEY_MOD_CTRL)) {
				app_invalidate(0, 0, 0, 0);
			} else {
				app_keyboard(key, 1);
			}
			if(quit) goto break_evloop;
		}

		if(use_mouse) {
			bnstate = read_mouse_bn();
			bndiff = bnstate ^ prev_bnstate;
			prev_bnstate = bnstate;

			read_mouse_rel(&mdx, &mdy);
			mx += mdx;
			if(mx < 0) mx = 0;
			if(mx >= SCR_WIDTH) mx = SCR_WIDTH - 1;
			my += mdy;
			if(my < 0) my = 0;
			if(my >= SCR_HEIGHT) my = SCR_HEIGHT - 1;
			mdx = mx - prev_mx;
			mdy = my - prev_my;

			if(bndiff & 1) app_mouse(0, bnstate & 1, mx, my);
			if(bndiff & 2) app_mouse(1, bnstate & 2, mx, my);
			if(bndiff & 4) app_mouse(3, bnstate & 4, mx, my);

			if((mdx | mdy) != 0) {
				app_motion(mx, my);
			}
		}

		app_display();
		app_swap_buffers();
	}

break_evloop:
	app_shutdown();
	vid_cleanup();
	kb_shutdown();
	return 0;
}

void app_invalidate(int x, int y, int w, int h)
{
	rtk_rect r;

	if((w | h) == 0) {
		r.x = r.y = 0;
		r.width = SCR_WIDTH;
		r.height = SCR_HEIGHT;
	} else {
		r.x = x;
		r.y = y;
		r.width = w;
		r.height = h;
	}

	if(dirty_valid) {
		rtk_rect_union(&dirty, &r);
	} else {
		dirty = r;
	}
	dirty_valid = 1;
}

void app_swap_buffers(void)
{
	unsigned char *src;

	vid_vsync();

	if(dirty_valid) {
		if(dirty.width < SCR_WIDTH || dirty.height < SCR_HEIGHT) {
			src = framebuf + dirty.y * SCR_WIDTH + dirty.x;
			vid_blit(dirty.x, dirty.y, dirty.width, dirty.height, src, SCR_WIDTH);
		} else {
			vid_blitfb(framebuf, SCR_WIDTH);
		}
		dirty_valid = 0;
	}
}

void app_quit(void)
{
	quit = 1;
}

void app_setcolor(int idx, int r, int g, int b)
{
	struct vid_color col;
	col.r = r;
	col.g = g;
	col.b = b;
	vid_setpal(idx, 1, &col);
}
