#ifndef APP_H_
#define APP_H_

#include "sizeint.h"
#include "logger.h"
#include "rtk.h"

#define SCR_WIDTH	640
#define SCR_HEIGHT	480

enum {
	KEY_BACKSP = 8,
	KEY_ESC	= 27,
	KEY_DEL	= 127,

	KEY_NUM_0 = 256, KEY_NUM_1, KEY_NUM_2, KEY_NUM_3, KEY_NUM_4,
	KEY_NUM_5, KEY_NUM_6, KEY_NUM_7, KEY_NUM_8, KEY_NUM_9,
	KEY_NUM_DOT, KEY_NUM_DIV, KEY_NUM_MUL, KEY_NUM_MINUS, KEY_NUM_PLUS, KEY_NUM_ENTER, KEY_NUM_EQUALS,
	KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
	KEY_INS, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_F13, KEY_F14, KEY_F15,
	KEY_NUMLK, KEY_CAPSLK, KEY_SCRLK,
	KEY_RSHIFT, KEY_LSHIFT, KEY_RCTRL, KEY_LCTRL, KEY_RALT, KEY_LALT,
	KEY_RMETA, KEY_LMETA, KEY_LSUPER, KEY_RSUPER, KEY_MODE, KEY_COMPOSE,
	KEY_HELP, KEY_PRINT, KEY_SYSRQ, KEY_BREAK
};

#ifndef KEY_ANY
#define KEY_ANY		(-1)
#define KEY_ALT		(-2)
#define KEY_CTRL	(-3)
#define KEY_SHIFT	(-4)
#endif

enum {
	KEY_MOD_SHIFT	= 1,
	KEY_MOD_CTRL	= 4,
	KEY_MOD_ALT	= 8
};


struct app_screen {
	const char *name;

	int (*init)(void);
	void (*destroy)(void);
	int (*start)(void);
	void (*stop)(void);
	void (*display)(void);
	void (*keyboard)(int, int);
	void (*mouse)(int, int, int, int);
	void (*motion)(int, int);
};

extern int mouse_x, mouse_y, mouse_state[3];
extern unsigned int modkeys;
extern int fullscr;

extern long time_msec;
extern struct app_screen *cur_scr;
extern struct app_screen menuscr;

extern unsigned char *framebuf;


int app_init(void);
void app_shutdown(void);

void app_display(void);
void app_reshape(int x, int y);
void app_keyboard(int key, int press);
void app_mouse(int bn, int st, int x, int y);
void app_motion(int x, int y);

void app_chscr(struct app_screen *scr);

void gui_fill(rtk_rect *rect, uint32_t color);
void gui_drawtext(int x, int y, const char *str);
void gui_textrect(const char *str, rtk_rect *rect);

/* defined in main.c */
void app_invalidate(int x, int y, int w, int h);
void app_swap_buffers(void);
void app_quit(void);

void app_setcolor(int idx, int r, int g, int b);

#endif	/* APP_H_ */
