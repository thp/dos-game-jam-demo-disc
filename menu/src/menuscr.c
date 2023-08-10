#include <string.h>
#include "app.h"
#include "rtk.h"

static int menu_init(void);
static void menu_destroy(void);
static int menu_start(void);
static void menu_stop(void);
static void menu_display(void);
static void menu_reshape(int x, int y);
static void menu_keyb(int key, int press);
static void menu_mouse(int bn, int press, int x, int y);
static void menu_motion(int x, int y);


struct app_screen menuscr = {
	"menu",
	menu_init, menu_destroy,
	menu_start, menu_stop,
	menu_display,
	menu_keyb, menu_mouse, menu_motion
};

static rtk_screen *gui;


static int menu_init(void)
{
	rtk_widget *win, *w;

	if(!(gui = rtk_create_screen())) {
		return -1;
	}
	rtk_invalidate_screen(gui);

	if(!(win = rtk_create_window(0, "CD menu", 100, 100, 400, 300, RTK_WIN_FRAME))) {
		return -1;
	}
	rtk_win_layout(win, RTK_NONE);
	rtk_add_window(gui, win);

	w = rtk_create_button(win, "foo", 0);
	rtk_autosize(w, RTK_AUTOSZ_NONE);
	rtk_move(w, 160, 130);
	rtk_resize(w, 60, 25);

	return 0;
}

static void menu_destroy(void)
{
	rtk_free_screen(gui);
}

static int menu_start(void)
{
	memset(framebuf, 0, SCR_WIDTH * SCR_HEIGHT);
	return 0;
}

static void menu_stop(void)
{
}

static void menu_display(void)
{
	rtk_draw_screen(gui);
}

static void menu_keyb(int key, int press)
{
	if(rtk_input_key(gui, key, press)) {
		return;
	}

	if(!press) return;

	switch(key) {
	case 27:
		app_quit();
		break;
	}
}

static void menu_mouse(int bn, int press, int x, int y)
{
	if(rtk_input_mbutton(gui, bn, press, x, y)) {
		return;
	}
}

static void menu_motion(int x, int y)
{
	if(rtk_input_mmotion(gui, x, y)) {
		return;
	}
}
