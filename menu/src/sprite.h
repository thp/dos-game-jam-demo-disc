#ifndef SPRITE_H_
#define SPRITE_H_

#include "imago2.h"

struct sprite;
struct spritesheet;

struct spritesheet *create_spritesheet(void);
void free_sprsheet(struct spritesheet *ss);

struct spritesheet *load_sprsheet(const char *fname);
void save_sprsheet(struct spritesheet *ss, const char *fname);

/* spritesheet building */
void set_sprsheet_image(struct spritesheet *ss, img_pixmap *img);
void define_sprite(struct spritesheet *ss, int x, int y, int w, int h);
int build_sprites(struct spritesheet *ss);

#endif	/* SPRITE_H_ */
