#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <GL/glut.h>

struct glyph {
	int code;
	int x, y;
	int width, height;
};

void display(void);
void reshape(int x, int y);
void keyboard(unsigned char key, int x, int y);

int save_image(const char *fname, int width, int height, unsigned char *img);
int save_metrics(const char *fname, struct glyph *glyphs, int num_glyphs);

int win_width, win_height;
int serif, fontsz = 10;
void *font;

struct glyph glyphs[128];
int num_glyphs;
int baseline;
unsigned char *glyphmap;
float fgcolor[] = {1, 1, 1, 1}, bgcolor[] = {0, 0, 0, 0};

static const char *usage_fmt = "Usage %s [options]\n"
	" -sans: sans-serif font\n"
	" -serif: serif font\n"
	" -size <n>: font size\n"
	" -bg/-fg <hexcolor>: backdrop/font color\n"
	" -help: print usage and exit\n\n";

int main(int argc, char **argv)
{
	int i;

	glutInitWindowSize(256, 32);
	glutInit(&argc, argv);

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-serif") == 0) {
				serif = 1;
			} else if(strcmp(argv[i], "-sans") == 0) {
				serif = 0;
			} else if(strcmp(argv[i], "-size") == 0) {
				if(!argv[++i] || !(fontsz = atoi(argv[i]))) {
					fprintf(stderr, "-size must be followed by the font size\n");
					return 1;
				}
			} else if(strcmp(argv[i], "-fg") == 0 || strcmp(argv[i], "-bg") == 0) {
				unsigned int packed;
				char *endp;
				float *col;
				if(!argv[++i] || ((packed = strtol(argv[i], &endp, 16)), endp == argv[i])) {
					fprintf(stderr, "%s must be followed by a hex/html packed color\n", argv[i - 1]);
					return 1;
				}
				col = (argv[i - 1][1] == 'f') ? fgcolor : bgcolor;
				col[0] = ((packed >> 16) & 0xff) / 255.0f;
				col[1] = ((packed >> 8) & 0xff) / 255.0f;
				col[2] = (packed & 0xff) / 255.0f;

			} else if(strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
				printf(usage_fmt, argv[0]);
				return 0;
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			fprintf(stderr, "unknown argument: %s\n", argv[i]);
			return -1;
		}
	}

	if(serif) {
		switch(fontsz) {
		case 10:
			font = GLUT_BITMAP_TIMES_ROMAN_10;
			break;
		case 24:
			font = GLUT_BITMAP_TIMES_ROMAN_24;
			break;
		default:
			fprintf(stderr, "serif %d not available (only 10, 24)\n", fontsz);
			return -1;
		}
	} else {
		switch(fontsz) {
		case 10:
			font = GLUT_BITMAP_HELVETICA_10;
			break;
		case 12:
			font = GLUT_BITMAP_HELVETICA_12;
			break;
		case 18:
			font = GLUT_BITMAP_HELVETICA_18;
			break;
		default:
			fprintf(stderr, "sans %d not available (only 10, 12, 18)\n", fontsz);
			return -1;
		}
	}

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("fontconv");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	glutMainLoop();
	return 0;
}

void display(void)
{
	int i, x, y;
	struct glyph *g;
	unsigned char *pixptr;

	glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	x = 0;
	y = win_height - fontsz;

	num_glyphs = 0;

	glColor4f(fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
	glRasterPos2i(x, y);
	for(i=32; i<128; i++) {
		int width = glutBitmapWidth(font, i);
		if(x + width >= win_width) {
			x = 0;
			y -= fontsz;
			glRasterPos2i(x, y);
		}

		g = glyphs + num_glyphs++;
		g->code = i;
		g->x = x;
		g->y = y;
		g->width = width;
		g->height = fontsz;

		glutBitmapCharacter(font, i);
		x += width;
	}

	if(y < 0) {
		glutReshapeWindow(win_width, win_height * 2);
	} else if(y >= win_height / 2) {
		glutReshapeWindow(win_width, win_height / 2);
	}

	pixptr = glyphmap;
	for(i=0; i<win_height; i++) {
		glReadPixels(0, win_height - i - 1, win_width, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixptr);
		pixptr += win_width;
	}

	glutSwapBuffers();
}

void reshape(int x, int y)
{
	char buf[128];

	free(glyphmap);
	glyphmap = malloc(x * y);

	glViewport(0, 0, x, y);

	win_width = x;
	win_height = y;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, x, 0, y, -1, 1);

	sprintf(buf, "fontconv (%dx%d)", x, y);
	glutSetWindowTitle(buf);
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case '=':
		if(win_width < 1024) {
			glutReshapeWindow(win_width * 2, win_height);
		}
		break;
	case '-':
		if(win_width > 2) {
			glutReshapeWindow(win_width / 2, win_height);
		}
		break;

	case 's':
		printf("saving glyphmap (%dx%d): font.pgm\n", win_width, win_height);
		save_image("font.pgm", win_width, win_height, glyphmap);

		printf("saving metrics: font.metrics\n");
		save_metrics("font.metrics", glyphs, num_glyphs);
	}
}

int save_image(const char *fname, int width, int height, unsigned char *img)
{
	FILE *fp;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open output file: %s: %s\n", fname, strerror(errno));
		return -1;
	}
	fprintf(fp, "P5\n%d %d\n255\n", width, height);
	fwrite(img, win_width * win_height, 1, fp);
	fclose(fp);
	return 0;
}

int save_metrics(const char *fname, struct glyph *glyphs, int num_glyphs)
{
	FILE *fp;
	int i;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open output file: %s: %s\n", fname, strerror(errno));
		return -1;
	}

	fprintf(fp, "MLBFONT1\n");
	fprintf(fp, "map: font.pgm\n");
	fprintf(fp, "height: %d\n", fontsz);
	fprintf(fp, "baseline: %d\n", baseline);

	for(i=0; i<num_glyphs; i++) {
		fprintf(fp, "%d: %dx%d%+d%+d\n", glyphs->code, glyphs->width,
				glyphs->height, glyphs->x, glyphs->y);
		glyphs++;
	}
	fclose(fp);
	return 0;
}
