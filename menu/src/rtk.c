#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "app.h"
#include "rtk.h"
#include "rtk_impl.h"
#include "logger.h"

static void on_any_nop();
static void on_window_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy);
static void on_button_click(rtk_widget *w);
static void on_textbox_key(rtk_widget *w, int key, int press);


void rtk_setup(rtk_draw_ops *drawop)
{
	rtk_gfx = *drawop;

	rtk_init_drawing();
}

static int wsize(int type)
{
	switch(type) {
	case RTK_LABEL:
	case RTK_CHECKBOX:
	case RTK_SEP:
	case RTK_DRAWBOX:
		return sizeof(rtk_widget);
	case RTK_WIN:
		return sizeof(rtk_window);
	case RTK_BUTTON:
		return sizeof(rtk_button);
	case RTK_TEXTBOX:
		return sizeof(rtk_textbox);
	case RTK_SLIDER:
		return sizeof(rtk_slider);
	default:
		break;
	}
	return 0;
}

rtk_widget *rtk_create_widget(int type)
{
	rtk_widget *w;

	if(!(w = calloc(1, wsize(type)))) {
		return 0;
	}
	w->type = type;
	w->flags = VISIBLE | ENABLED | GEOMCHG | DIRTY;

	w->on_key = on_any_nop;
	w->on_mbutton = on_any_nop;
	w->on_click = on_any_nop;
	w->on_drag = on_any_nop;
	w->on_drop = on_any_nop;

	w->pad = 2;
	return w;
}

void rtk_free_widget(rtk_widget *w)
{
	if(!w) return;

	if(w->type == RTK_WIN) {
		rtk_window *win = (rtk_window*)w;
		while(win->clist) {
			rtk_widget *c = win->clist;
			win->clist = win->clist->next;
			rtk_free_widget(c);
		}
	}

	free(w->text);
	free(w);
}

int rtk_type(rtk_widget *w)
{
	return w->type;
}

void rtk_padding(rtk_widget *w, int pad)
{
	w->pad = pad;
}

void rtk_move(rtk_widget *w, int x, int y)
{
	if(!w->par) {
		rtk_invalfb(w);
	}
	w->x = x;
	w->y = y;
	w->flags |= GEOMCHG | DIRTY;
	if(!w->par) {
		rtk_invalfb(w);
	}
}

void rtk_pos(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->x;
	*yptr = w->y;
}

void rtk_abspos(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->absx;
	*yptr = w->absy;
}

void rtk_resize(rtk_widget *w, int xsz, int ysz)
{
	if(!w->par) {
		rtk_invalfb(w);
	}
	w->width = xsz;
	w->height = ysz;
	w->flags |= GEOMCHG | DIRTY;
	if(!w->par) {
		rtk_invalfb(w);
	}
}

void rtk_size(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->width;
	*yptr = w->height;
}

int rtk_get_width(rtk_widget *w)
{
	return w->width;
}

int rtk_get_height(rtk_widget *w)
{
	return w->height;
}

void rtk_get_rect(rtk_widget *w, rtk_rect *r)
{
	r->x = w->x;
	r->y = w->y;
	r->width = w->width;
	r->height = w->height;
}

void rtk_get_absrect(rtk_widget *w, rtk_rect *r)
{
	r->x = w->absx;
	r->y = w->absy;
	r->width = w->width;
	r->height = w->height;
}

void rtk_autosize(rtk_widget *w, unsigned int szopt)
{
	if(szopt & RTK_AUTOSZ_WIDTH) {
		w->flags |= AUTOWIDTH;
	} else {
		w->flags &= ~AUTOWIDTH;
	}

	if(szopt & RTK_AUTOSZ_HEIGHT) {
		w->flags |= AUTOHEIGHT;
	} else {
		w->flags &= ~AUTOHEIGHT;
	}
}

int rtk_set_text(rtk_widget *w, const char *str)
{
	rtk_rect rect;
	char *s = strdup(str);
	if(!s) return -1;

	free(w->text);
	w->text = s;

	if(w->type == RTK_TEXTBOX) {
		rtk_textbox *tb = (rtk_textbox*)w;
		tb->cursor = tb->scroll = 0;
		tb->bufsz = strlen(str) + 1;
	}

	rtk_calc_widget_rect(w, &rect);
	rtk_resize(w, rect.width, rect.height);
	rtk_invalidate(w);
	return 0;
}

const char *rtk_get_text(rtk_widget *w)
{
	return w->text;
}

void rtk_set_value(rtk_widget *w, int val)
{
	w->value = val;
	rtk_invalidate(w);
}

int rtk_get_value(rtk_widget *w)
{
	return w->value;
}

void rtk_set_callback(rtk_widget *w, rtk_callback cbfunc, void *cls)
{
	w->cbfunc = cbfunc;
	w->cbcls = cls;
}

void rtk_set_callback_closure(rtk_widget *w, void *cls)
{
	w->cbcls = cls;
}

rtk_callback rtk_get_callback(const rtk_widget *w)
{
	return w->cbfunc;
}

void *rtk_get_callback_closure(const rtk_widget *w)
{
	return w->cbcls;
}

void rtk_set_drawfunc(rtk_widget *w, rtk_callback drawfunc, void *cls)
{
	w->drawcb = drawfunc;
	w->drawcls = cls;
}

void rtk_set_key_handler(rtk_widget *w, rtk_key_callback func)
{
	w->on_key = func;
}

void rtk_set_mbutton_handler(rtk_widget *w, rtk_mbutton_callback func)
{
	w->on_mbutton = func;
}

void rtk_set_click_handler(rtk_widget *w, rtk_click_callback func)
{
	w->on_click = func;
}

void rtk_set_drag_handler(rtk_widget *w, rtk_drag_callback func)
{
	w->on_drag = func;
}

void rtk_set_drop_handler(rtk_widget *w, rtk_drop_callback func)
{
	w->on_drop = func;
}

void rtk_show(rtk_widget *w)
{
	w->flags |= VISIBLE;
	rtk_invalidate(w);
}

void rtk_show_modal(rtk_widget *w)
{
	rtk_show(w);
	if(w->scr) {
		w->scr->modal = w;
	}
}

void rtk_hide(rtk_widget *w)
{
	w->flags &= ~VISIBLE;
	if(w->scr && w->scr->modal == w) {
		w->scr->modal = 0;
	}
	rtk_invalfb(w);
}

int rtk_visible(const rtk_widget *w)
{
	return w->flags & VISIBLE;
}

void rtk_invalidate(rtk_widget *w)
{
	w->flags |= DIRTY;

#if !defined(MSDOS) && !defined(__MSDOS__)
	/* this is necessary because the GLUT backend will have to see a
	 * glutRedisplay in order for anything to be redrawn
	 */
	rtk_invalfb(w);
#endif
}

void rtk_validate(rtk_widget *w)
{
	w->flags &= ~DIRTY;
}

void rtk_win_layout(rtk_widget *w, int layout)
{
	((rtk_window*)w)->layout = layout;
}

void rtk_win_clear(rtk_widget *w)
{
	rtk_widget *tmp;
	rtk_window *win = (rtk_window*)w;

	RTK_ASSERT_TYPE(w, RTK_WIN);

	while(win->clist) {
		tmp = win->clist;
		win->clist = win->clist->next;
		rtk_free_widget(tmp);
	}

	win->clist = win->ctail = 0;
	rtk_invalidate(w);
}

void rtk_win_add(rtk_widget *par, rtk_widget *child)
{
	rtk_window *parwin = (rtk_window*)par;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	if(rtk_win_has(par, child)) {
		return;
	}

	if(child->par) {
		rtk_win_rm((rtk_widget*)child->par, child);
	}

	if(parwin->clist) {
		parwin->ctail->next = child;
		parwin->ctail = child;
	} else {
		parwin->clist = parwin->ctail = child;
	}
	child->next = 0;

	child->par = parwin;
	rtk_invalidate(par);
}

void rtk_win_rm(rtk_widget *par, rtk_widget *child)
{
	rtk_widget *prev, dummy;
	rtk_window *parwin = (rtk_window*)par;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	dummy.next = parwin->clist;
	prev = &dummy;
	while(prev->next) {
		if(prev->next == child) {
			if(!child->next) {
				parwin->ctail = prev;
			}
			prev->next = child->next;
			break;
		}
		prev = prev->next;
	}
	parwin->clist = dummy.next;
	rtk_invalidate(par);
}

int rtk_win_has(rtk_widget *par, rtk_widget *child)
{
	rtk_widget *w;
	rtk_window *parwin = (rtk_window*)par;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	w = parwin->clist;
	while(w) {
		if(w == child) {
			return 1;
		}
		w = w->next;
	}
	return 0;
}

rtk_widget *rtk_win_child(const rtk_widget *par, int idx)
{
	rtk_widget *w;
	rtk_window *win = (rtk_window*)par;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	w = win->clist;
	while(w && idx-- > 0) {
		w = w->next;
	}
	return idx > 0 ? 0 : w;
}

int rtk_win_descendant(const rtk_widget *par, const rtk_widget *w)
{
	rtk_widget *c;
	rtk_window *parwin = (rtk_window*)par;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	c = parwin->clist;
	while(c) {
		if(c == w) {
			return 1;
		}
		if(c->type == RTK_WIN) {
			if(rtk_win_descendant(c, w)) {
				return 1;
			}
		}
		c = c->next;
	}
	return 0;
}

/* --- button functions --- */
void rtk_bn_mode(rtk_widget *w, int mode)
{
	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	((rtk_button*)w)->mode = mode;
	rtk_invalidate(w);
}

void rtk_bn_set_icon(rtk_widget *w, rtk_icon *icon)
{
	rtk_rect rect;

	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	((rtk_button*)w)->icon = icon;

	rtk_calc_widget_rect(w, &rect);
	rtk_resize(w, rect.width, rect.height);
	rtk_invalidate(w);
}

rtk_icon *rtk_bn_get_icon(rtk_widget *w)
{
	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	return ((rtk_button*)w)->icon;
}

/* --- slider functions --- */
void rtk_slider_set_range(rtk_widget *w, int vmin, int vmax)
{
	rtk_slider *slider = (rtk_slider*)w;
	RTK_ASSERT_TYPE(w, RTK_SLIDER);

	slider->vmin = vmin;
	slider->vmax = vmax;
}

void rtk_slider_get_range(const rtk_widget *w, int *vmin, int *vmax)
{
	rtk_slider *slider = (rtk_slider*)w;
	RTK_ASSERT_TYPE(w, RTK_SLIDER);

	*vmin = slider->vmin;
	*vmax = slider->vmax;
}


/* --- constructors --- */

rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y,
		int width, int height, unsigned int flags)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_WIN))) {
		return 0;
	}
	if(par) {
		rtk_win_add(par, w);
	}
	w->on_drag = on_window_drag;
	rtk_set_text(w, title);
	rtk_move(w, x, y);
	rtk_resize(w, width, height);
	rtk_win_layout(w, RTK_VBOX);

	w->flags |= flags << 16;
	return w;
}

rtk_widget *rtk_create_button(rtk_widget *par, const char *str, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_BUTTON))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_SIZE);
	w->on_click = on_button_click;
	rtk_set_text(w, str);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_iconbutton(rtk_widget *par, rtk_icon *icon, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_BUTTON))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_SIZE);
	w->on_click = on_button_click;
	rtk_bn_set_icon(w, icon);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_label(rtk_widget *par, const char *text)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_LABEL))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_SIZE);
	rtk_set_text(w, text);
	return w;
}

rtk_widget *rtk_create_checkbox(rtk_widget *par, const char *text, int chk, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_CHECKBOX))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_SIZE);
	rtk_set_text(w, text);
	rtk_set_value(w, chk ? 1 : 0);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_textbox(rtk_widget *par, const char *text, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_TEXTBOX))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_HEIGHT);
	w->on_key = on_textbox_key;
	if(text) {
		rtk_set_text(w, text);
	}
	rtk_set_callback(w, cbfunc, 0);
	rtk_resize(w, 40, 1);

	w->flags |= CANFOCUS;
	return w;
}

rtk_widget *rtk_create_slider(rtk_widget *par, int vmin, int vmax, int val, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_SLIDER))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_HEIGHT);
	rtk_set_callback(w, cbfunc, 0);
	rtk_slider_set_range(w, vmin, vmax);
	rtk_set_value(w, val);
	return w;
}

rtk_widget *rtk_create_separator(rtk_widget *par)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_SEP))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_autosize(w, RTK_AUTOSZ_SIZE);
	return w;
}

rtk_widget *rtk_create_drawbox(rtk_widget *par, int width, int height, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget(RTK_DRAWBOX))) {
		return 0;
	}
	if(par) rtk_win_add(par, w);
	rtk_resize(w, width, height);
	rtk_set_drawfunc(w, cbfunc, 0);
	return w;
}

/* --- compound widgets --- */
rtk_widget *rtk_create_hbox(rtk_widget *par)
{
	rtk_widget *box = rtk_create_window(par, "hbox", 0, 0, 0, 0, 0);
	rtk_autosize(box, RTK_AUTOSZ_SIZE);
	rtk_win_layout(box, RTK_HBOX);
	return box;
}

rtk_widget *rtk_create_vbox(rtk_widget *par)
{
	rtk_widget *box = rtk_create_window(par, "vbox", 0, 0, 0, 0, 0);
	rtk_autosize(box, RTK_AUTOSZ_SIZE);
	rtk_win_layout(box, RTK_VBOX);
	return box;
}

rtk_widget *rtk_create_field(rtk_widget *par, const char *lbtext, rtk_callback cbfunc)
{
	rtk_widget *hbox;
	rtk_widget *lb, *tb;

	if(!(hbox = rtk_create_window(par, "field", 0, 0, 0, 0, 0))) {
		return 0;
	}
	rtk_win_layout(hbox, RTK_HBOX);
	if(!(lb = rtk_create_label(hbox, lbtext))) {
		rtk_free_widget(hbox);
		return 0;
	}
	if(!(tb = rtk_create_textbox(hbox, 0, cbfunc))) {
		rtk_free_widget(hbox);
		return 0;
	}
	return tb;
}

/* --- icon functions --- */
rtk_iconsheet *rtk_load_iconsheet(const char *fname)
{
	/*
	 rtk_iconsheet *is;

	if(!(is = malloc(sizeof *is))) {
		return 0;
	}
	is->icons = 0;

	if(!(is->pixels = img_load_pixels(fname, &is->width, &is->height, IMG_FMT_RGBA32))) {
		free(is);
		return 0;
	}
	return is;
	*/
	return 0;
}

void rtk_free_iconsheet(rtk_iconsheet *is)
{
	/*
	rtk_icon *icon;

	img_free_pixels(is->pixels);

	while(is->icons) {
		icon = is->icons;
		is->icons = is->icons->next;
		free(icon->name);
		free(icon);
	}
	free(is);
	*/
}

rtk_icon *rtk_define_icon(rtk_iconsheet *is, const char *name, int x, int y, int w, int h)
{
	rtk_icon *icon;

	if(!(icon = malloc(sizeof *icon))) {
		return 0;
	}
	if(!(icon->name = strdup(name))) {
		free(icon);
		return 0;
	}
	icon->width = w;
	icon->height = h;
	icon->scanlen = is->width;
	icon->pixels = is->pixels + y * is->width + x;

	icon->next = is->icons;
	is->icons = icon;
	return icon;
}

rtk_icon *rtk_lookup_icon(rtk_iconsheet *is, const char *name)
{
	rtk_icon *icon = is->icons;
	while(icon) {
		if(strcmp(icon->name, name) == 0) {
			return icon;
		}
		icon = icon->next;
	}
	return 0;
}

static void sethover(rtk_screen *scr, rtk_widget *w)
{
	if(scr->hover == w) return;

	if(scr->hover) {
		scr->hover->flags &= ~HOVER;

		if(scr->hover->type != RTK_WIN) {
			rtk_invalidate(scr->hover);
		}
	}
	scr->hover = w;
	if(w) {
		w->flags |= HOVER;

		if(w->type != RTK_WIN) {
			rtk_invalidate(w);
		}
	}

	/*dbgmsg("hover \"%s\"\n", w ? (w->text ? w->text : "?") : "-");*/
}

static void setpress(rtk_widget *w, int press)
{
	if(press) {
		w->flags |= PRESS;
	} else {
		w->flags &= ~PRESS;
	}
	rtk_invalidate(w);
}

static void setfocus(rtk_screen *scr, rtk_widget *w)
{
	rtk_window *win;

	if(scr->focus) {
		scr->focus->flags &= ~FOCUS;
	}

	if(scr->focuswin) {
		scr->focuswin->flags &= ~FOCUS;
		rtk_invalidate((rtk_widget*)scr->focuswin);
	}

	if(!w) return;

	if(w->flags & CANFOCUS) {
		w->flags |= FOCUS;
		scr->focus = w;
	}

	win = (rtk_window*)w;
	while(win->par) {
		win = win->par;
	}
	win->flags |= FOCUS;
	scr->focuswin = win;

	rtk_invalidate(w);
	rtk_invalidate((rtk_widget*)win);
}

/* --- screen functions --- */
rtk_screen *rtk_create_screen(void)
{
	rtk_screen *scr;

	if(!(scr = calloc(1, sizeof *scr))) {
		return 0;
	}
	return scr;
}

void rtk_free_screen(rtk_screen *scr)
{
	int i;

	for(i=0; i<scr->num_win; i++) {
		rtk_free_widget(scr->winlist[i]);
	}
	free(scr);
}

int rtk_add_window(rtk_screen *scr, rtk_widget *win)
{
	if(scr->num_win >= MAX_WINDOWS) {
		return -1;
	}
	scr->winlist[scr->num_win++] = win;
	win->scr = scr;
	return 0;
}

static rtk_widget *find_widget_at(rtk_widget *w, int x, int y, unsigned int flags)
{
	rtk_widget *c, *res;
	rtk_window *win;

	if(!(w->flags & VISIBLE)) {
		return 0;
	}

	if(w->type != RTK_WIN) {
		if((w->flags & flags) && rtk_hittest(w, x, y)) {
			return w;
		}
		return 0;
	}

	win = (rtk_window*)w;
	c = win->clist;
	while(c) {
		if((res = find_widget_at(c, x, y, flags))) {
			return res;
		}
		c = c->next;
	}
	return rtk_hittest(w, x, y) ? w : 0;
}

rtk_widget *rtk_find_widget_at(rtk_screen *scr, rtk_widget *win, int x, int y, unsigned int flags)
{
	int i;
	rtk_widget *w;

	if(!flags) flags = ~0;

	if(win) {
		RTK_ASSERT_TYPE(win, RTK_WIN);
		return find_widget_at(win, x, y, flags);
	}

	for(i=0; i<scr->num_win; i++) {
		if((w = find_widget_at(scr->winlist[i], x, y, flags))) {
			return w;
		}
	}
	return 0;
}

int rtk_input_key(rtk_screen *scr, int key, int press)
{
	if(scr->focus && (!scr->modal || rtk_win_descendant(scr->modal, scr->focus))) {
		scr->focus->on_key(scr->focus, key, press);
		return 1;
	}

	if(scr->modal) {
		if(!press) return 1;

		switch(key) {
		case 27:
			rtk_hide(scr->modal);
			break;

		case '\n':
			/* TODO */
			break;

		default:
			break;
		}
		return 1;
	}
	return 0;
}

int rtk_input_mbutton(rtk_screen *scr, int bn, int press, int x, int y)
{
	int handled = 0;
	rtk_widget *w;

	if((w = rtk_find_widget_at(scr, scr->modal, x, y, 0))) {
		int relx = x - w->absx;
		int rely = y - w->absy;
		w->on_mbutton(w, bn, press, relx, rely);
	}

	if(press) {
		scr->prev_mx = x;
		scr->prev_my = y;
		if(bn == 0) {
			scr->press = w;
			scr->press_x = x;
			scr->press_y = y;

			if(w) setpress(w, 1);
		}
		handled = w ? 1 : 0;
	} else {
		if(!w && scr->modal) {
			rtk_hide(scr->modal);
			return 1;
		}

		if(bn == 0) {
			rtk_widget *newfocus = 0;
			if(w) {
				if(scr->press == w) {
					newfocus = rtk_find_widget_at(scr, scr->modal, x, y, 0);
					w->on_click(w);
				} else {
					w->on_drop(scr->press, w);
				}
				handled = 1;
			}

			setfocus(scr, newfocus);

			if(scr->press) {
				setpress(scr->press, 0);
				scr->press = 0;
			}
		}
	}
	return scr->modal ? 1 : handled;
}

int rtk_input_mmotion(rtk_screen *scr, int x, int y)
{
	int dx, dy;
	rtk_widget *w;

	dx = x - scr->prev_mx;
	dy = y - scr->prev_my;
	scr->prev_mx = x;
	scr->prev_my = y;

	if(scr->press) {
		w = scr->press;
		if((dx | dy)) {
			w->on_drag(w, dx, dy, x - scr->press_x, y - scr->press_y);
		}
		return 1;
	}

	if(!(w = rtk_find_widget_at(scr, scr->modal, x, y, 0))) {
		return scr->modal ? 1 : 0;
	}
	sethover(scr, w);
	return 1;
}

void rtk_invalidate_screen(rtk_screen *scr)
{
	int i;
	for(i=0; i<scr->num_win; i++) {
		rtk_invalidate(scr->winlist[i]);
	}
}

void rtk_draw_screen(rtk_screen *scr)
{
	int i;
	for(i=0; i<scr->num_win; i++) {
		rtk_draw_widget(scr->winlist[i]);
	}
}

/* --- misc functions --- */
void rtk_null_rect(rtk_rect *rect)
{
	rect->x = rect->y = INT_MAX / 2;
	rect->width = rect->height = INT_MIN;
}

void rtk_fix_rect(rtk_rect *rect)
{
	int x, y, w, h;

	x = rect->x;
	y = rect->y;

	if(rect->width < 0) {
		w = -rect->width;
		x += rect->width;
	} else {
		w = rect->width;
	}
	if(rect->height < 0) {
		h = -rect->height;
		y += rect->height;
	} else {
		h = rect->height;
	}

	rect->x = x;
	rect->y = y;
	rect->width = w;
	rect->height = h;
}

void rtk_rect_union(rtk_rect *a, const rtk_rect *b)
{
	int x0, y0, x1, y1;

	x0 = a->x;
	y0 = a->y;
	x1 = a->x + a->width;
	y1 = a->y + a->height;

	if(b->x < x0) x0 = b->x;
	if(b->y < y0) y0 = b->y;
	if(b->x + b->width > x1) x1 = b->x + b->width;
	if(b->y + b->height > y1) y1 = b->y + b->height;

	a->x = x0;
	a->y = y0;
	a->width = x1 - x0;
	a->height = y1 - y0;
}


/* --- widget event handlers --- */
static void on_any_nop()
{
}

static void on_window_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy)
{
	if(w->flags & MOVABLE) {
		rtk_move(w, w->x + dx, w->y + dy);
	}
}

static void on_button_click(rtk_widget *w)
{
	rtk_button *bn = (rtk_button*)w;

	switch(w->type) {
	case RTK_BUTTON:
		if(bn->mode == RTK_TOGGLEBN) {
		case RTK_CHECKBOX:
			bn->value ^= 1;
		}
		if(bn->cbfunc) {
			bn->cbfunc(w, bn->cbcls);
		}
		rtk_invalidate(w);
		break;

	default:
		break;
	}
}

static void on_textbox_key(rtk_widget *w, int key, int press)
{
	rtk_textbox *tb = (rtk_textbox*)w;

	if(!press) return;

	switch(key) {
	case KEY_BACKSP:
		if(tb->cursor > 0) {
			if(tb->cursor < tb->len) {
				memmove(tb->text + tb->cursor - 1, tb->text + tb->cursor, tb->len - tb->cursor + 1);
				tb->cursor--;
			} else {
				tb->text[--tb->cursor] = 0;
			}
			tb->len--;
		}
		rtk_invalidate(w);
		return;

	case KEY_HOME:
		tb->cursor = 0;
		tb->scroll = 0;
		rtk_invalidate(w);
		return;

	case KEY_END:
		tb->cursor = tb->len;
		rtk_invalidate(w);
		return;

	case KEY_LEFT:
		if(tb->cursor > 0) {
			tb->cursor--;
		}
		if(tb->cursor < tb->scroll) {
			tb->scroll = tb->cursor;
		}
		rtk_invalidate(w);
		return;

	case KEY_RIGHT:
		if(tb->cursor < tb->len) {
			tb->cursor++;
		}
		rtk_invalidate(w);
		return;

	default:
		if(!isprint(key)) {
			return;
		}
		break;
	}

	/* we end up here if it's a printable character */
	if(tb->len >= tb->bufsz - 1) {
		int nsz = tb->bufsz ? tb->bufsz * 2 : 16;
		void *tmp = realloc(tb->text, nsz);
		if(!tmp) return;
		tb->text = tmp;
		tb->bufsz = nsz;
	}

	if(tb->len <= 0 || tb->cursor >= tb->len) {
		tb->text[tb->len++] = key;
		tb->text[tb->len] = 0;
		tb->cursor++;
	} else {
		memmove(tb->text + tb->cursor + 1, tb->text + tb->cursor, tb->len - tb->cursor + 1);
		tb->text[tb->cursor++] = key;
		tb->len++;
	}

	rtk_invalidate(w);
}

void rtk_dbg_showrect(rtk_widget *w, int show)
{
	if(show) {
		w->flags |= DBGRECT;
	} else {
		w->flags &= ~DBGRECT;
	}
}
