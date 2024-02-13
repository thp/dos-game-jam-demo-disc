#ifndef IMAGE_H_
#define IMAGE_H_

struct color {
	unsigned char r, g, b;
};

struct image {
	char *path;
	long width, height;
	long pitch;
	int bpp, ncolors;
	struct color cmap[256];
	unsigned char *pixels;

	unsigned int seq;
	struct image *next;
};

void init_image(struct image *img);
void destroy_image(struct image *img);

struct image *alloc_image(void);
void free_image(struct image *img);

int open_image(struct image *img, const char *fname);

int load_image_pixels(struct image *img);
void free_image_pixels(struct image *img);

#endif	/* IMAGE_H_ */
