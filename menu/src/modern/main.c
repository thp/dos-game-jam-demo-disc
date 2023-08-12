#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "miniglut.h"
#include "app.h"
#include "rtk.h"
#include "logger.h"

struct color {
	int r, g, b;
};

#define SCALE	2

static void display(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);
static int translate_skey(int key);

#if defined(__unix__) || defined(unix)
#include <GL/glx.h>
static Display *xdpy;
static Window xwin;

static void (*glx_swap_interval_ext)();
static void (*glx_swap_interval_sgi)();
#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif

#ifndef GL_BGRA
#define GL_BGRA	0x80e1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE	0x812f
#endif

static uint32_t *framebuf_rgb;
static struct color cmap[256];


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitWindowSize(640 * SCALE, 480 * SCALE);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("CDmenu");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);

#if defined(__unix__) || defined(unix)
	xdpy = glXGetCurrentDisplay();
	xwin = glXGetCurrentDrawable();

	if(!(glx_swap_interval_ext = glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT"))) {
		glx_swap_interval_sgi = glXGetProcAddress((unsigned char*)"glXSwapIntervalSGI");
	}
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif

	if(!(framebuf_rgb = malloc(SCR_WIDTH * SCR_HEIGHT * sizeof *framebuf_rgb))) {
		return 1;
	}

	init_logger();
	add_log_stream(stdout);

	if(app_init() == -1) {
		return 1;
	}
	atexit(app_shutdown);

	glutMainLoop();
	return 0;
}

unsigned long get_msec(void)
{
	return (unsigned long)glutGet(GLUT_ELAPSED_TIME);
}

void app_invalidate(int x, int y, int w, int h)
{
	/*dbgmsg("fakeupd: %d,%d (%dx%d)\n", x, y, w, h);*/
	glutPostRedisplay();
}

void app_swap_buffers(void)
{
	int i, j, idx;
	unsigned char *src = framebuf;
	uint32_t *dest = framebuf_rgb;

	for(i=0; i<SCR_HEIGHT; i++) {
		for(j=0; j<SCR_WIDTH; j++) {
			idx = src[j];
			dest[j] = (cmap[idx].b << 16) | (cmap[idx].g << 8) | cmap[idx].r;
		}
		src += SCR_WIDTH;
		dest += SCR_WIDTH;
	}

	glViewport(0, 0, SCR_WIDTH * SCALE, SCR_HEIGHT * SCALE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, SCR_WIDTH, SCR_HEIGHT, 0, -1, 1);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glRasterPos2i(0, 0);
	glPixelZoom(SCALE, -SCALE);
	glDrawPixels(SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuf_rgb);

	glPopAttrib();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

void app_quit(void)
{
	exit(0);
}

void app_resize(int x, int y)
{
	if(x == SCR_WIDTH * SCALE && y == SCR_HEIGHT * SCALE) {
		return;
	}

	glutReshapeWindow(x * SCALE, y * SCALE);
}

void app_fullscreen(int fs)
{
	static int prev_w, prev_h;

	if(fs == -1) {
		fs = !fullscr;
	}

	if(fs == fullscr) return;

	if(fs) {
		prev_w = glutGet(GLUT_WINDOW_WIDTH);
		prev_h = glutGet(GLUT_WINDOW_HEIGHT);
		glutFullScreen();
	} else {
		glutReshapeWindow(prev_w, prev_h);
	}
	fullscr = fs;
}

#if defined(__unix__) || defined(unix)
void app_vsync(int vsync)
{
	vsync = vsync ? 1 : 0;
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(xdpy, xwin, vsync);
	} else if(glx_swap_interval_sgi) {
		glx_swap_interval_sgi(vsync);
	}
}
#endif
#ifdef WIN32
void app_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync ? 1 : 0);
	}
}
#endif

void app_setcolor(int idx, int r, int g, int b)
{
	cmap[idx].r = r;
	cmap[idx].g = g;
	cmap[idx].b = b;
}

static void display(void)
{
	app_display();
	app_swap_buffers();
}

static void reshape(int x, int y)
{
}

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();
	app_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	app_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
	int k;
	modkeys = glutGetModifiers();
	if((k = translate_skey(key)) >= 0) {
		app_keyboard(k, 1);
	}
}

static void skeyup(int key, int x, int y)
{
	int k = translate_skey(key);
	if(k >= 0) {
		app_keyboard(k, 0);
	}
}

static void mouse(int bn, int st, int x, int y)
{
	modkeys = glutGetModifiers();
	app_mouse(bn - GLUT_LEFT_BUTTON, st == GLUT_DOWN, x / SCALE, y / SCALE);
}

static void motion(int x, int y)
{
	app_motion(x / SCALE, y / SCALE);
}

static int translate_skey(int key)
{
	switch(key) {
	case GLUT_KEY_LEFT:
		return KEY_LEFT;
	case GLUT_KEY_UP:
		return KEY_UP;
	case GLUT_KEY_RIGHT:
		return KEY_RIGHT;
	case GLUT_KEY_DOWN:
		return KEY_DOWN;
	case GLUT_KEY_PAGE_UP:
		return KEY_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return KEY_PGDN;
	case GLUT_KEY_HOME:
		return KEY_HOME;
	case GLUT_KEY_END:
		return KEY_END;
	case GLUT_KEY_INSERT:
		return KEY_INS;
	default:
		if(key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12) {
			return key - GLUT_KEY_F1 + KEY_F1;
		}
	}

	return -1;
}
