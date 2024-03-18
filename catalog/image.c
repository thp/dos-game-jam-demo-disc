#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

static int open_pcx(struct image *img, const char *filename);
static int load_pixels_pcx(struct image *img);


void init_image(struct image *img)
{
	memset(img, 0, sizeof *img);
}

void destroy_image(struct image *img)
{
	free(img->pixels);
}

struct image *alloc_image(void)
{
	struct image *img;
	if((img = malloc(sizeof *img))) {
		init_image(img);
	}
	return img;
}

void free_image(struct image *img)
{
	if(img) {
		destroy_image(img);
		free(img);
	}
}

int open_image(struct image *img, const char *fname)
{
	if(open_pcx(img, fname) == -1) {
		return -1;
	}
	if(!(img->path = strdup(fname))) {
		destroy_image(img);
		return -1;
	}
	return 0;
}

int load_image_pixels(struct image *img)
{
	return load_pixels_pcx(img);
}

void free_image_pixels(struct image *img)
{
	if(img) {
		free(img->pixels);
		img->pixels = 0;
	}
}

/* ---------- PCX loader adapted from menu.c ---------- */
struct PCXHeader {
    unsigned char header_field; // 0x0a
    unsigned char version; // 5
    unsigned char encoding; // 1 = RLE
    unsigned char bpp; // 8
    unsigned short min_x;
    unsigned short min_y;
    unsigned short max_x;
    unsigned short max_y;
    unsigned short dpi_x;
    unsigned short dpi_y;
    unsigned char ega_palette[16][3];
    unsigned char reserved0;
    unsigned char num_planes; // 1
    unsigned short stride_bytes; // 320
    unsigned short palette_mode; // 1 = color
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned char reserved1[54];
};

static int open_pcx(struct image *img, const char *filename)
{
	FILE *fp;
	struct PCXHeader header;
	int i, res, ch;

	if(!(fp = fopen(filename, "rb"))) {
		return -1;
	}
	if((res = fread(&header, sizeof(header), 1, fp)) != 1) {
		fclose(fp);
		return -1;
	}

	img->width = header.max_x - header.min_x + 1;
	img->height = header.max_y - header.min_y + 1;

	if(!(header.header_field == 0x0a && header.version == 5 && header.encoding == 1 &&
			header.bpp == 8 && header.num_planes == 1 && header.stride_bytes == 320 &&
			header.palette_mode == 1 && img->width == 320 && img->height == 200)) {
		fprintf(stderr, "Error, unsupported format:\n");
		fprintf(stderr, "hdr: 0x%04x\n", header.header_field);
		fprintf(stderr, "version: %d\n", header.version);
		fprintf(stderr, "encoding: %d\n", header.encoding);
		fprintf(stderr, "bpp: %d\n", header.bpp);
		fprintf(stderr, "res: [%d, %d]-[%d, %d]\n", header.min_x, header.min_y, header.max_x, header.max_y);
		fprintf(stderr, "num planes: %d\n", header.num_planes);
		fprintf(stderr, "stride_bytes: %d\n", header.stride_bytes);
		fprintf(stderr, "palette_mode: %d\n", header.palette_mode);
		fprintf(stderr, "resolution: [%d, %d]\n", header.x_resolution, header.y_resolution);
		fclose(fp);
		return -1;
	}

	img->bpp = header.bpp;
	img->ncolors = 256;
	img->pitch = img->width * img->bpp / 8;
	img->next = 0;
	img->pixels = 0;

	fseek(fp, -(768 + 1), SEEK_END);
    if((ch = fgetc(fp)) == 12) {
		for(i=0; i<256; ++i) {
			img->cmap[i].r = fgetc(fp);
			img->cmap[i].g = fgetc(fp);
			img->cmap[i].b = fgetc(fp);
		}
	} else {
		for(i=0; i<256; i++) {
			img->cmap[i].r = img->cmap[i].g = img->cmap[i].b = i;
		}
	}

	fclose(fp);
	return 0;
}

static int load_pixels_pcx(struct image *img)
{
	FILE *fp;
	long pixels_processed = 0;
	static unsigned char chunk[512];
	unsigned char repeat, *pptr;
	int i, ch, len = 0, pos = 0;
	long npixels;

	if(img->pixels) return 0;

	if(!img->path || !(fp = fopen(img->path, "rb"))) {
		return -1;
	}
	fseek(fp, sizeof(struct PCXHeader), SEEK_SET);

	npixels = img->pitch * img->height;
	if(!(img->pixels = malloc(npixels))) {
		fclose(fp);
		return -1;
	}
	pptr = img->pixels;

	while(pixels_processed < npixels) {
		if(pos == len) {
			pos = 0;
			len = fread(chunk, 1, sizeof(chunk), fp);
		}
		ch = chunk[pos++];

		if((ch & 0xc0) == 0xc0) {
			repeat = ch & 0x3f;

			if(pos == len) {
				pos = 0;
				len = fread(chunk, 1, sizeof(chunk), fp);
			}
			ch = chunk[pos++];

			for(i=0; i<repeat; ++i) {
				*pptr++ = ch;
			}
			pixels_processed += repeat;
		} else {
			*pptr++ = ch;
			pixels_processed++;
		}
	}

	fclose(fp);
	return 0;
}
