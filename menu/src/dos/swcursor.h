#ifndef SWCURSOR_H_
#define SWCURSOR_H_

void reset_swcursor(void);
void set_swcursor(int width, int height, int hotx, int hoty, void *pixels, void *mask, int pitch);
void draw_swcursor(int x, int y);

#endif	/* SWCURSOR_H_ */
