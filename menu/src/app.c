#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "app.h"
#include "timer.h"
#include "rtk.h"

int mouse_x, mouse_y, mouse_state[3];
unsigned int modkeys;
int fullscr;

long time_msec;

struct app_screen *cur_scr;

unsigned char *framebuf;

/* available screens */
#define MAX_SCREENS	8
static struct app_screen *screens[MAX_SCREENS];
static int num_screens;


int app_init(void)
{
	int i;
	char *start_scr_name;
	static rtk_draw_ops guigfx = {gui_fill, 0, gui_drawtext, gui_textrect};

	if(!(framebuf = malloc(SCR_WIDTH * SCR_HEIGHT))) {
		errormsg("failed to allocate framebuffer (%dx%d)\n", SCR_WIDTH, SCR_HEIGHT);
		return -1;
	}

	rtk_setup(&guigfx);

	/* initialize screens */
	screens[num_screens++] = &menuscr;

	start_scr_name = getenv("START_SCREEN");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->init() == -1) {
			return -1;
		}
	}

	time_msec = get_msec();

	for(i=0; i<num_screens; i++) {
		if(screens[i]->name && start_scr_name && strcmp(screens[i]->name, start_scr_name) == 0) {
			app_chscr(screens[i]);
			break;
		}
	}
	if(!cur_scr) {
		app_chscr(&menuscr);
	}

	return 0;
}

void app_shutdown(void)
{
	int i;

	for(i=0; i<num_screens; i++) {
		if(screens[i]->destroy) {
			screens[i]->destroy();
		}
	}

	cleanup_logger();
	free(framebuf);
}

void app_display(void)
{
	time_msec = get_msec();

	cur_scr->display();
}

void app_keyboard(int key, int press)
{
	long msec;
	static long prev_esc;

	if(press) {
		switch(key) {
#ifdef DBG_ESCQUIT
		case 27:
			msec = get_msec();
			if(msec - prev_esc < 1000) {
				app_quit();
				return;
			}
			prev_esc = msec;
			break;
#endif

		case 'q':
			if(modkeys & KEY_MOD_CTRL) {
				app_quit();
				return;
			}
			break;
		}
	}

	if(cur_scr && cur_scr->keyboard) {
		cur_scr->keyboard(key, press);
	}
}

void app_mouse(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	if(bn < 3) {
		mouse_state[bn] = st;
	}

	if(cur_scr && cur_scr->mouse) {
		cur_scr->mouse(bn, st, x, y);
	}
}

void app_motion(int x, int y)
{
	if(cur_scr && cur_scr->motion) {
		cur_scr->motion(x, y);
	}
	mouse_x = x;
	mouse_y = y;
}

void app_chscr(struct app_screen *scr)
{
	struct app_screen *prev = cur_scr;

	if(!scr) return;

	if(scr->start && scr->start() == -1) {
		return;
	}

	if(prev && prev->stop) {
		prev->stop();
	}
	cur_scr = scr;
}

void gui_fill(rtk_rect *rect, uint32_t color)
{
	int i, j;
	unsigned char *fb;

	if(rect->x < 0) {
		rect->width += rect->x;
		rect->x = 0;
	}
	if(rect->y < 0) {
		rect->height += rect->y;
		rect->y = 0;
	}
	if(rect->x + rect->width >= SCR_WIDTH) {
		rect->width = SCR_WIDTH - rect->x;
	}
	if(rect->y + rect->height >= SCR_HEIGHT) {
		rect->height = SCR_HEIGHT - rect->y;
	}

	fb = framebuf + rect->y * SCR_WIDTH + rect->x;
	for(i=0; i<rect->height; i++) {
		for(j=0; j<rect->width; j++) {
			fb[j] = color;
		}
		fb += SCR_WIDTH;
	}
}

void gui_drawtext(int x, int y, const char *str)
{
}

void gui_textrect(const char *str, rtk_rect *rect)
{
}
