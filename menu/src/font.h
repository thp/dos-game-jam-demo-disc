#ifndef FONT_H_
#define FONT_H_

#include "sprite.h"

struct font {
	struct spritesheet *spr;
	int size;
	float height, baseline;
};

int load_font(struct font *font, const char *fname);
void destroy_font(struct font *font);

void use_font(struct font *font);

float font_strwidth(struct font *font, const char *str);

#endif	/* FONT_H_ */
