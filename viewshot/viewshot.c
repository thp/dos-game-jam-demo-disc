#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <direct.h>
#include "video.h"
#include "image.h"

static void show_image(struct image *img);
static struct image *prev_image(struct image *img);
static struct image *open_imgdir(const char *dirname);

static struct image *curimg;


int main(int argc, char **argv)
{
	struct stat st;
	struct image *img;
	int c, special = 0;

	if(!argv[1]) {
		fprintf(stderr, "provide an image filename, or directory with images\n");
		return 1;
	}
	if(stat(argv[1], &st) == -1) {
		fprintf(stderr, "failed to stat: %s\n", argv[1]);
		return 1;
	}

	if((st.st_mode & S_IFMT) == S_IFDIR) {
		if(!(img = open_imgdir(argv[1]))) {
			return 1;
		}
	} else {
		if(!(img = alloc_image()) || open_image(img, argv[1]) == -1) {
			fprintf(stderr, "failed to load image: %s\n", argv[1]);
			free_image(img);
			return 1;
		}
		img->next = img;
	}

	vid_setmode(0x13);

	show_image(img);

	for(;;) {
		c = getch();

		if(special) {
			switch(c) {
			case 0x4d:	/* right */
				show_image(curimg->next);
				break;

			case 0x4b:	/* left */
				show_image(prev_image(curimg));
				break;

			default:
				//printf("S:%x\n", (unsigned int)c);
				break;
			}
			special = 0;

		} else {
			switch(c) {
			case 0:
				special = 1;
				break;

			case 'q':
			case 27:
				goto done;

			case ' ':
				show_image(curimg->next);
				break;

			case '\b':
				show_image(prev_image(curimg));
				break;

			default:
				//printf("C:%x\n", (unsigned int)c);
                                break;
			}
		}
	}

done:
	vid_setmode(3);
	return 0;
}

static void evict_lru(struct image *img)
{
	struct image *iter, *best = 0;

	iter = img->next;
	while(iter != img) {
		if(iter->pixels && (!best || iter->seq < best->seq)) {
			best = iter;
		}
		iter = iter->next;
	}

	if(best) {
		free_image_pixels(best);
	}
}

static void show_image(struct image *img)
{
	int i;
	struct color *col = img->cmap;
	static unsigned int seq;

	if(img == curimg) return;

	_fmemset(MK_FP(0xa000, 0), 0, 64000);

	for(i=0; i<256; i++) {
		vid_setcolor(i, col->r, col->g, col->b);
		col++;
	}

	img->seq = ++seq;
	if(!img->pixels) {
		if(load_image_pixels(img) == -1) {
			evict_lru(img);
			if(load_image_pixels(img) == -1) {
				vid_setmode(3);
				exit(1);
			}
		}
	}
	curimg = img;

	_fmemcpy(MK_FP(0xa000, 0), img->pixels, 64000);
}

static struct image *prev_image(struct image *img)
{
	struct image *prev = img;
	while(prev->next != img) prev = prev->next;
	return prev;
}

static bool has_prefix_case_insensitive(const char *haystack, const char *prefix)
{
    int i;
    if (strlen(haystack) < strlen(prefix)) {
        return false;
    }

    for (i=0; prefix[i] != '\0'; ++i) {
        if (tolower(haystack[i]) != tolower(prefix[i])) {
            return false;
        }
    }

    return true;
}

static struct image *open_imgdir(const char *dirname)
{
	DIR *dir;
	struct dirent *dent;
	struct image *img, *imglist = 0, *imgtail;
	char fname[256];
	struct stat st;
	struct image tmpimg;

	if(!(dir = opendir(dirname))) {
		fprintf(stderr, "failed to open directory: %s\n", dirname);
		return 0;
	}

	while((dent = readdir(dir))) {
		sprintf(fname, "%s/%s", dirname, dent->d_name);

		/* Skip the "blur" versions of the files */
		if (has_prefix_case_insensitive(dent->d_name, "blur")) {
			continue;
		}

		if(stat(fname, &st) == -1 || (st.st_mode & S_IFMT) != S_IFREG) {
			continue;	/* skip non-regular files */
		}

		if(open_image(&tmpimg, fname) != -1) {
			if(!(img = alloc_image())) {
				break;
			}
			*img = tmpimg;
			if(imglist) {
				imgtail->next = img;
				imgtail = img;
			} else {
				imglist = imgtail = img;
			}
		}
	}

	closedir(dir);

	imgtail->next = imglist;	/* make circular */
	return imglist;
}
