#include <string.h>
#include "app.h"
#include "rtk.h"
#include "rtk_impl.h"
#include "util.h"

rtk_draw_ops rtk_gfx;
#define gfx rtk_gfx

static int fontheight;

enum {
	FRM_SOLID,
	FRM_OUTSET,
	FRM_INSET,
	FRM_FILLBG = 0x100
};

static void widget_rect(rtk_widget *w, rtk_rect *rect);
static void abs_widget_rect(rtk_widget *w, rtk_rect *rect);
static void uicolor(uint32_t col, uint32_t lcol, uint32_t scol);
static void draw_frame(rtk_rect *rect, int type, int sz, uint32_t bgcol, uint32_t shad, uint32_t lit);

static void draw_window(rtk_widget *w);
static void draw_label(rtk_widget *w);
static void draw_button(rtk_widget *w);
static void draw_checkbox(rtk_widget *w);
static void draw_textbox(rtk_widget *w);
static void draw_slider(rtk_widget *w);
static void draw_separator(rtk_widget *w);
static void draw_drawbox(rtk_widget *w);


#define BEVELSZ		2
#define PAD			(w->pad)
#define OFFS		(BEVELSZ + PAD)
#define CHKBOXSZ	(BEVELSZ * 2 + 8)

#define WINFRM_SZ	2
#define WINFRM_TBAR	16

enum {
	COL_BLACK = 240,
	COL_BG,
	COL_BGHL,
	COL_SHAD,
	COL_LIT,
	COL_FRM_BG,
	COL_FRM_SHAD,
	COL_FRM_LIT,
	COL_TEXT,
	COL_FRM_TEXT,
	COL_WHITE
};

void rtk_init_drawing(void)
{
	app_setcolor(COL_BLACK, 0, 0, 0);
	app_setcolor(COL_WHITE, 0xff, 0xff, 0xff);
	app_setcolor(COL_TEXT, 0, 0, 0);
	app_setcolor(COL_BG, 0x66, 0x66, 0x66);
	app_setcolor(COL_BGHL, 0x80, 0x80, 0x80);
	app_setcolor(COL_SHAD, 0x22, 0x22, 0x22);
	app_setcolor(COL_LIT, 0xaa, 0xaa, 0xaa);
	app_setcolor(COL_FRM_TEXT, 0xff, 0xff, 0xff);
	app_setcolor(COL_FRM_BG, 0x6d, 0x2a, 0x83);
	app_setcolor(COL_FRM_SHAD, 0x49, 0x26, 0x55);
	app_setcolor(COL_FRM_LIT, 0xa1, 0x34, 0xc5);
}


void rtk_calc_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rtk_widget *child;
	rtk_button *bn;
	rtk_rect txrect = {0};

	rect->x = w->x;
	rect->y = w->y;
	rect->width = w->width;
	rect->height = w->height;

	rtk_abs_pos(w, &w->absx, &w->absy);

	if((w->flags & (AUTOWIDTH | AUTOHEIGHT)) == 0) {
		return;
	}

	if(w->text) {
		gfx.textrect(w->text, &txrect);
	}

	switch(w->type) {
	case RTK_WIN:
		if((w->flags & (AUTOWIDTH | AUTOHEIGHT))) {
			rtk_rect subrect;
			rtk_window *win = (rtk_window*)w;
			rtk_null_rect(&subrect);
			child = win->clist;
			while(child) {
				rtk_rect crect;
				widget_rect(child, &crect);
				rtk_rect_union(&subrect, &crect);
				child = child->next;
			}
			if(w->flags & AUTOWIDTH) {
				rect->width = subrect.width + PAD * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = subrect.height + PAD * 2;
			}
		}
		break;

	case RTK_DRAWBOX:
		if(w->flags & AUTOWIDTH) {
			rect->width = w->width;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = w->height;
		}
		break;

	case RTK_BUTTON:
		bn = (rtk_button*)w;
		if(bn->icon) {
			if(w->flags & AUTOWIDTH) {
				rect->width = bn->icon->width + OFFS * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = bn->icon->height + OFFS * 2;
			}
		} else {
			if(w->flags & AUTOWIDTH) {
				rect->width = txrect.width + OFFS * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = txrect.height + OFFS * 2;
			}
		}
		break;

	case RTK_CHECKBOX:
		if(w->flags & AUTOWIDTH) {
			rect->width = txrect.width + CHKBOXSZ + OFFS * 2 + PAD;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = txrect.height + OFFS * 2;
		}
		break;

	case RTK_LABEL:
		if(w->flags & AUTOWIDTH) {
			rect->width = txrect.width + PAD * 2;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = txrect.height + PAD * 2;
		}
		break;

	case RTK_TEXTBOX:
		if(w->flags & AUTOHEIGHT) {
			if(rect->height < fontheight + OFFS * 2) {
				rect->height = fontheight + OFFS * 2;
			}
		}
		break;

	case RTK_SEP:
		if(w->par->layout == RTK_VBOX) {
			if(w->flags & AUTOWIDTH) {
				rect->width = w->par->width - PAD * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = PAD * 4 + BEVELSZ * 2;
			}
		} else if(w->par->layout == RTK_HBOX) {
			if(w->flags & AUTOWIDTH) {
				rect->width = PAD * 4 + BEVELSZ * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = w->par->height - PAD * 2;
			}
		} else {
			if(w->flags & AUTOWIDTH) {
				rect->width = 0;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = 0;
			}
		}
		break;

	default:
		if(w->flags & AUTOWIDTH) {
			rect->width = 0;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = 0;
		}
	}
}

void rtk_abs_pos(rtk_widget *w, int *xpos, int *ypos)
{
	int x, y, px, py;

	x = w->x;
	y = w->y;

	if(w->par) {
		rtk_abs_pos((rtk_widget*)w->par, &px, &py);
		x += px;
		y += py;
	}

	*xpos = x;
	*ypos = y;
}


int rtk_hittest(rtk_widget *w, int x, int y)
{
	int x0, y0, x1, y1;

	if(!(w->flags & VISIBLE)) {
		return 0;
	}

	x0 = w->absx;
	y0 = w->absy;
	x1 = x0 + w->width;
	y1 = y0 + w->height;

	if(w->type == RTK_WIN && (w->flags & FRAME)) {
		x0 -= WINFRM_SZ;
		y0 -= WINFRM_SZ + WINFRM_TBAR;
		x1 += WINFRM_SZ;
		y1 += WINFRM_SZ;
	}

	if(x < x0 || y < y0) return 0;
	if(x >= x1 || y >= y1) return 0;
	return 1;
}


void rtk_invalfb(rtk_widget *w)
{
	rtk_rect rect;

	rect.x = w->x;
	rect.y = w->y;
	rect.width = w->width;
	rect.height = w->height;

	rtk_abs_pos(w, &rect.x, &rect.y);

	if(w->type == RTK_WIN && (w->flags & FRAME)) {
		rect.x -= WINFRM_SZ;
		rect.y -= WINFRM_SZ + WINFRM_TBAR;
		rect.width += WINFRM_SZ * 2;
		rect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
	}

	app_invalidate(rect.x, rect.y, rect.width, rect.height);
}

static int need_relayout(rtk_widget *w)
{
	rtk_widget *c;

	if(w->flags & GEOMCHG) {
		return 1;
	}

	if(w->type == RTK_WIN) {
		c = ((rtk_window*)w)->clist;
		while(c) {
			if(need_relayout(c)) {
				return 1;
			}
			c = c->next;
		}
	}
	return 0;
}

static void calc_layout(rtk_widget *w)
{
	int x, y, dx, dy;
	rtk_widget *c;
	rtk_window *win = (rtk_window*)w;
	rtk_rect rect;

	if(w->type == RTK_WIN) {
		x = y = PAD;

		c = win->clist;
		while(c) {
			if(win->layout != RTK_NONE) {
				rtk_move(c, x, y);
				calc_layout(c);

				if(win->layout == RTK_VBOX) {
					y += c->height + PAD * 2;
				} else if(win->layout == RTK_HBOX) {
					x += c->width + PAD * 2;
				}
			} else {
				calc_layout(c);
			}

			c = c->next;
		}
	}

	rtk_calc_widget_rect(w, &rect);
	dx = rect.width - w->width;
	dy = rect.height - w->height;
	w->width = rect.width;
	w->height = rect.height;

	if((dx | dy) == 0) {
		w->flags &= ~GEOMCHG;
	}
	rtk_invalidate(w);
}

static int calc_substr_width(const char *str, int start, int end)
{
	rtk_rect rect;
	int len;
	char *buf;

	if(end <= start) {
		return 0;
	}
	if(end <= 0) {
		end = strlen(str);
	}

	len = end - start;
	buf = alloca(len + 1);

	memcpy(buf, str + start, len);
	buf[len] = 0;

	gfx.textrect(buf, &rect);
	return rect.width;
}

void rtk_draw_widget(rtk_widget *w)
{
	int dirty;

	if(!(w->flags & VISIBLE)) {
		return;
	}

	if(need_relayout(w)) {
		calc_layout(w);
	}

	dirty = w->flags & DIRTY;
	if(!dirty && w->type != RTK_WIN) {
		return;
	}

	switch(w->type) {
	case RTK_WIN:
		draw_window(w);
		break;

	case RTK_LABEL:
		draw_label(w);
		break;

	case RTK_BUTTON:
		draw_button(w);
		break;

	case RTK_CHECKBOX:
		draw_checkbox(w);
		break;

	case RTK_TEXTBOX:
		draw_textbox(w);
		break;

	case RTK_SLIDER:
		draw_slider(w);
		break;

	case RTK_SEP:
		draw_separator(w);
		break;

	case RTK_DRAWBOX:
		draw_drawbox(w);
		break;

	default:
		break;
	}

	if(w->drawcb) {
		w->drawcb(w, w->drawcls);
	}

	if(dirty) {
		rtk_validate(w);
		rtk_invalfb(w);
	}
}

static void widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rect->x = w->x;
	rect->y = w->y;
	rect->width = w->width;
	rect->height = w->height;
}

static void abs_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rect->x = w->absx;
	rect->y = w->absy;
	rect->width = w->width;
	rect->height = w->height;
}

static void hline(int x, int y, int sz, uint32_t col)
{
	rtk_rect rect;
	rect.x = x;
	rect.y = y;
	rect.width = sz;
	rect.height = 1;
	gfx.fill(&rect, col);
}

static void vline(int x, int y, int sz, uint32_t col)
{
	rtk_rect rect;
	rect.x = x;
	rect.y = y;
	rect.width = 1;
	rect.height = sz;
	gfx.fill(&rect, col);
}

static void draw_frame(rtk_rect *rect, int type, int sz, uint32_t bgcol, uint32_t shad, uint32_t lit)
{
	int i, tlcol, brcol, fillbg;
	rtk_rect r = *rect;

	fillbg = type & FRM_FILLBG;
	type &= ~FRM_FILLBG;

	switch(type) {
	case FRM_OUTSET:
		tlcol = lit;
		brcol = shad;
		break;
	case FRM_INSET:
		tlcol = shad;
		brcol = lit;
		break;
	case FRM_SOLID:
	default:
		tlcol = brcol = bgcol;
	}

	for(i=0; i<sz; i++) {
		if(r.width < 2 || r.height < 2) break;

		hline(r.x, r.y, r.width, tlcol);
		vline(r.x, r.y + 1, r.height - 2, tlcol);
		hline(r.x, r.y + r.height - 1, r.width, brcol);
		vline(r.x + r.width - 1, r.y + 1, r.height - 2, brcol);

		r.x++;
		r.y++;
		r.width -= 2;
		r.height -= 2;
	}

	if(fillbg) {
		gfx.fill(&r, bgcol);
	}
}

static void draw_window(rtk_widget *w)
{
	rtk_rect rect, frmrect, tbrect;
	rtk_widget *c;
	rtk_window *win = (rtk_window*)w;
	int win_dirty = w->flags & DIRTY;

	if(win_dirty) {
		abs_widget_rect(w, &rect);

		if(w->flags & FRAME) {
			/*if(w->flags & FOCUS) {
				uicolor(COL_WINFRM_FOCUS, COL_WINFRM_LIT_FOCUS, COL_WINFRM_SHAD_FOCUS);
			} else {
				uicolor(COL_WINFRM, COL_WINFRM_LIT, COL_WINFRM_SHAD);
			}*/

			frmrect = rect;
			frmrect.width += WINFRM_SZ * 2;
			frmrect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
			frmrect.x -= WINFRM_SZ;
			frmrect.y -= WINFRM_SZ + WINFRM_TBAR;

			tbrect.x = rect.x;
			tbrect.y = rect.y - WINFRM_TBAR;
			tbrect.width = rect.width;
			tbrect.height = WINFRM_TBAR;

			draw_frame(&frmrect, FRM_OUTSET, BEVELSZ, COL_FRM_BG, COL_FRM_SHAD, COL_FRM_LIT);
			frmrect.x++;
			frmrect.y++;
			frmrect.width -= 2;
			frmrect.height -= 2;
			draw_frame(&frmrect, FRM_INSET, BEVELSZ, COL_FRM_BG, COL_FRM_SHAD, COL_FRM_LIT);

			draw_frame(&tbrect, FRM_OUTSET | FRM_FILLBG, BEVELSZ, COL_FRM_BG, COL_FRM_SHAD, COL_FRM_LIT);
			tbrect.x++;
			tbrect.y++;
			tbrect.width -= 2;
			tbrect.height -= 2;

			gfx.drawtext(tbrect.x, tbrect.y + tbrect.height - 1, w->text);
		}

		gfx.fill(&rect, COL_BG);
	}

	c = win->clist;
	while(c) {
		if(win_dirty) {
			rtk_invalidate(c);
		}
		rtk_draw_widget(c);
		c = c->next;
	}
}

static void draw_label(rtk_widget *w)
{
	rtk_rect rect;

	abs_widget_rect(w, &rect);

	gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);
}

static void draw_button(rtk_widget *w)
{
	int pressed, flags;
	rtk_rect rect;
	uint32_t col;
	rtk_button *bn = (rtk_button*)w;

	abs_widget_rect(w, &rect);

	if(bn->mode == RTK_TOGGLEBN) {
		pressed = w->value;
	} else {
		pressed = w->flags & PRESS;
	}

	col = w->flags & HOVER ? COL_BGHL : COL_BG;
	flags = (pressed ? FRM_INSET : FRM_OUTSET) | FRM_FILLBG;

	draw_frame(&rect, flags, BEVELSZ, col, COL_SHAD, COL_LIT);
	rect.x++;
	rect.y++;
	rect.width -= 2;
	rect.height -= 2;

	if(bn->icon) {
		int offs = w->flags & PRESS ? PAD + 1 : PAD;
		gfx.blit(rect.x + offs, rect.y + offs, bn->icon);
	} else if(w->text) {
		gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);
	}
}

static void draw_checkbox(rtk_widget *w)
{
}

static void draw_textbox(rtk_widget *w)
{
	rtk_rect rect;
	rtk_textbox *tb = (rtk_textbox*)w;
	int curx = 0;

	abs_widget_rect(w, &rect);

	draw_frame(&rect, FRM_INSET | FRM_FILLBG, BEVELSZ, COL_WHITE, COL_SHAD, COL_LIT);

	rect.x++;
	rect.y++;
	rect.width -= 2;
	rect.height -= 2;

	if(w->text) {
		gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);

		if(w->flags & FOCUS) {
			curx = calc_substr_width(w->text, tb->scroll, tb->cursor);
		}
	}

	/* cursor */
	if(w->flags & FOCUS) {
		int x = rect.x + PAD + curx - 1;
		int y = rect.y + rect.height - PAD - fontheight;
		vline(x, y, fontheight, COL_TEXT);
	}

	rtk_invalfb(w);
}

static void draw_slider(rtk_widget *w)
{
}

static void draw_separator(rtk_widget *w)
{
	rtk_window *win = (rtk_window*)w->par;
	rtk_rect rect;

	if(!win) return;

	abs_widget_rect(w, &rect);

	switch(win->layout) {
	case RTK_VBOX:
		rect.y += PAD * 2;
		rect.height = 2;
		break;

	case RTK_HBOX:
		rect.x += PAD * 2;
		rect.width = 2;
		break;

	default:
		break;
	}

	draw_frame(&rect, FRM_INSET, BEVELSZ, COL_BG, COL_SHAD, COL_LIT);
}

static void draw_drawbox(rtk_widget *w)
{
	if(!w->cbfunc) {
		rtk_rect r;
		abs_widget_rect(w, &r);
		gfx.fill(&r, 0xff000000);
		return;
	}

	w->cbfunc(w, w->cbcls);
}
