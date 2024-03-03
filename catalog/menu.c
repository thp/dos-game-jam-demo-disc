#if defined(VESAMENU)
#include <malloc.h>
#include <i86.h>
#include "vbe.h"
#include "vbe.c"

struct vbe_info vbe;
struct vbe_mode_info minf;

/* huge framebuffer to extend over multiple segments */
unsigned char huge *fb;
long fbsize;
long fb_winsz;
long fb_winstep;
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#include <dos.h>
#include <time.h>

#include <conio.h>
#define KEY_LEFT (0x100 | 75)
#define KEY_RIGHT (0x100 | 77)
#define KEY_DOWN (0x100 | 80)
#define KEY_UP (0x100 | 72)
#define KEY_NPAGE (0x100 | 81)
#define KEY_PPAGE (0x100 | 73)
#define KEY_HOME (0x100 | 71)
#define KEY_END (0x100 | 79)
#define KEY_F2 (0x100 | 60)
#define KEY_F3 (0x100 | 61)
#define KEY_F4 (0x100 | 62)
#define KEY_F5 (0x100 | 63)
#define KEY_ENTER (13)
#define KEY_ESCAPE (27)
#define KEY_BACKSPACE (8)

#include "gamectlg.h"
#include "ipc.h"
#include "vgautil.h"
#include "cpuutil.h"
#include "emuutil.h"

#if defined(VGAMENU)
#include "vgafont.h"
#endif

#if defined(VESAMENU)
#include "font_8x16.h"
#endif

static struct IPCBuffer __far *
ipc_buffer = NULL;

static enum DisplayAdapter
display_adapter_type;

static enum CPUType
cpu_type;

static enum DOSEmulator
emulator_type;

static int
have_32bit_cpu = 0;

static int
mouse_available = 0;

static int
have_mouse_driver()
{
    union REGS inregs, outregs;
    inregs.x.ax = 0;

    int86(0x33, &inregs, &outregs);

    if (outregs.x.ax == 0xffff) {
        return 1;
    }

    return 0;
}

static int
screenshot_idx = 0;

static void
show_screenshots(struct GameCatalog *cat, int game);

static void
configure_text_mode();

#if defined(VGAMENU)
#  define VGA_WIDTH (320)
#  define VGA_HEIGHT (200)
#  define SCREEN_WIDTH (VGA_WIDTH / 4)
#  define SCREEN_HEIGHT (VGA_HEIGHT / 6)
#elif defined(VESAMENU)
#  define VESA_WIDTH (640)
#  define VESA_HEIGHT (480)
#  define SCREEN_WIDTH (VESA_WIDTH / 8)
#  define SCREEN_HEIGHT (VESA_HEIGHT / 16)
#else
#  define SCREEN_WIDTH (80)
#  define SCREEN_HEIGHT (25)
#endif

static uint8_t
SCREEN_BUFFER[SCREEN_WIDTH*SCREEN_HEIGHT][2];

static uint8_t
SCREEN_BUFFER_OLD[SCREEN_WIDTH*SCREEN_HEIGHT][2];

static int16_t
screen_x = 0;

static int16_t
screen_y = 0;

static uint8_t
    COLOR_DEFAULT_FG = 0,

    // default window text
    COLOR_DEFAULT_BG = 3,

    // bright variant of background
    COLOR_BRIGHT_BG = 3 + 8,
    COLOR_DARK_BG = 1,

    // cursor/highlight
    COLOR_CURSOR_BG = 1,
    COLOR_CURSOR_FG = 3 + 8,

    // text color of "more" indicator
    COLOR_MORE_FG = 1,

    // text color of game description
    COLOR_GAME_DESCRIPTION_FG = 3 + 8,

    // should be bright version of default bg
    COLOR_SCROLL_FG = 3 + 8,

    // frame label
    COLOR_FRAME_LABEL_BG = 0,
#if defined(VGAMENU) || defined(VESAMENU)
    COLOR_FRAME_LABEL_FG = 3 + 8,
#else
    COLOR_FRAME_LABEL_FG = 0xf,
#endif

    // hotkey bar
    COLOR_HOTKEY_BG = 0,
    COLOR_HOTKEY_KEY = 3,
    COLOR_HOTKEY_LABEL = 0xf,

    // grayscale
    COLOR_DISABLED_BG = 7,
    COLOR_DISABLED_FG = 0,
    COLOR_DISABLED_SOFT_CONTRAST = 8;


static void
update_color_bindings()
{
    COLOR_BRIGHT_BG = COLOR_DEFAULT_BG + 8;
    COLOR_CURSOR_BG = COLOR_DARK_BG;
    COLOR_CURSOR_FG = COLOR_BRIGHT_BG;
    COLOR_MORE_FG = COLOR_DARK_BG;
    COLOR_GAME_DESCRIPTION_FG = COLOR_BRIGHT_BG;
    COLOR_SCROLL_FG = COLOR_BRIGHT_BG;
    COLOR_HOTKEY_KEY = COLOR_DEFAULT_BG;
}

static uint8_t
screen_attr = 7;

static int32_t
#if defined(VGAMENU) || defined(VESAMENU)
brightness = 255;
#else
brightness = 0;
#endif

static int
brightness_up_down = 1;

static int
is_fading()
{
    return ((brightness > 0 && brightness_up_down == -1) ||
            (brightness < 255 && brightness_up_down == 1));
}

static void
screen_putch(char ch)
{
    if (ch == '\n') {
        screen_x = 0;
        if (screen_y < SCREEN_HEIGHT - 1) {
            screen_y++;
        }
    } else {
        if (screen_y >= SCREEN_HEIGHT || screen_x >= SCREEN_WIDTH) {
            printf("error: y=%d, x=%d\n", screen_y, screen_x);
            getch();
        }
        SCREEN_BUFFER[SCREEN_WIDTH * screen_y + screen_x][0] = ch;
        SCREEN_BUFFER[SCREEN_WIDTH * screen_y + screen_x][1] = screen_attr;
        screen_x++;
        if (screen_x == SCREEN_WIDTH) {
            if (screen_y < SCREEN_HEIGHT - 1) {
                screen_y++;
            }
            screen_x = 0;
        }
    }
}

static void
screen_print(const char *msg)
{
    while (*msg) {
        screen_putch(*msg++);
    }
}

void
print_parents_first(char *buf, struct GameCatalog *cat, struct GameCatalogGroup *group)
{
    if (group->parent_group) {
        print_parents_first(buf, cat, group->parent_group);
        char sep[] = { ' ', 0xaf, ' ', 0x00 };
        strcat(buf, sep);
    }

    strcat(buf, cat->strings->d[group->title_idx]);
}

struct SearchState {
    int now_searching;
    char search_str[30];
    int search_pos;
    int previous_input;
    int saved_cursor;
    int saved_offset;
};

void reset_search_state(struct SearchState *state)
{
    state->now_searching = 0;
    for (int i=0; i<sizeof(state->search_str); ++i) {
        state->search_str[i] = 0;
    }
    state->search_pos = 0;
    state->previous_input = 0;
    state->saved_cursor = 0;
    state->saved_offset = 0;
}

struct ChoiceDialogState {
    struct GameCatalog *cat;
    struct GameCatalogGroup *here;
    int game;

    int cursor;
    int offset;

    struct SearchState search;
};

void any_border(int fg, int bg)
{
    int save_attr = screen_attr;
    screen_attr = (bg << 4) | fg;
    screen_putch(0xba);
    screen_attr = save_attr;
}

void right_shadow(int n)
{
    int save_attr = screen_attr;
    screen_attr = (0 << 4) | 7;
    for (int i=0; i<n; ++i) {
        // Do not "textwrap" shadow
        if (screen_x == 0) {
            break;
        }

        screen_putch(0xb0);
    }
    screen_attr = save_attr;
}

void right_border(int fg, int bg)
{
    any_border(fg, bg);
    right_shadow(2);
}

void left_border(int fg, int bg)
{
    screen_x--;
    any_border(fg, bg);
}

struct HotKey {
    const char *key;
    const char *func;
};

void draw_statusbar(const char *search_string)
{
    for (int i=0; i<SCREEN_WIDTH; ++i) {
        SCREEN_BUFFER[(SCREEN_HEIGHT-1)*SCREEN_WIDTH+i][0] = ' ';
        SCREEN_BUFFER[(SCREEN_HEIGHT-1)*SCREEN_WIDTH+i][1] =
            (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_BG;
    }

    int save = screen_attr;
    screen_y = SCREEN_HEIGHT - 1;
    screen_x = 2;

    static const struct HotKey
    KEYS[] = {
        { "ESC", "Back" },
#if !defined(VGAMENU) && !defined(VESAMENU)
        { "F2", "Color Theme" },
#endif
        { "F3", "Search" },
#if defined(VGAMENU) || defined(VESAMENU)
        { "F4", "Hide UI" },
        { "L/R", "Screenshots" },
#else
        { "F4", "Screenshots" },
#endif
        // TODO: Could hide "Video mode" on CGA/EGA
        { "F5", "Video Mode" },
        { NULL, NULL },
    };

    static const struct HotKey
    SEARCH_KEYS[] = {
        { "Up", "Previous" },
        { "Down", "Next" },
        { "Enter", "Done" },
        { NULL, NULL },
    };

    const struct HotKey *keys = search_string ? SEARCH_KEYS : KEYS;

    for (int i=0; keys[i].key != NULL; ++i) {
        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_KEY;
        screen_print(keys[i].key);
        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_LABEL;
        screen_print(" ");
        screen_print(keys[i].func);

        screen_print("  ");
    }

    if (search_string != NULL) {
        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_KEY;
        screen_print("Search: ");
        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_LABEL;
        screen_print(search_string);
        enable_blinking_cursor(screen_x, screen_y);
        screen_print("  ");
    }

    screen_attr = save;
}

static void
render_background(const char *title)
{
    memset(SCREEN_BUFFER, 0, sizeof(SCREEN_BUFFER));
    screen_x = 0;
    screen_y = 0;
    screen_attr = (COLOR_DEFAULT_BG << 4) | COLOR_DEFAULT_FG;

    for (int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; ++i) {
        SCREEN_BUFFER[i][0] = 0xb2;
        SCREEN_BUFFER[i][1] = (COLOR_DARK_BG << 4) | COLOR_DISABLED_BG;
    }

    for (int i=0; i<SCREEN_WIDTH; ++i) {
        SCREEN_BUFFER[i][0] = ' ';
        SCREEN_BUFFER[i][1] = (COLOR_FRAME_LABEL_BG << 4) | COLOR_FRAME_LABEL_FG;
    }

    if (*title) {
        screen_y = 0;
        screen_x = (SCREEN_WIDTH - strlen(title) - 2) / 2;
        if (screen_x < 0) {
            screen_x = 0;
        }
        if (screen_x >= SCREEN_WIDTH) {
            screen_x = SCREEN_WIDTH - 1;
        }

        int save = screen_attr;
        screen_attr = (COLOR_FRAME_LABEL_BG << 4) | COLOR_FRAME_LABEL_FG;
        screen_putch(' ');
        screen_print(title);
        screen_putch(' ');
        screen_attr = save;
    }

    draw_statusbar(NULL);

    int save = screen_attr;

    if (ipc_buffer) {
        const char *strings[4];
        int n_strings = 0;

        char cpu_str[32];
        sprintf(cpu_str, " CPU: %s ", CPU_TYPES[cpu_type]);
        strings[n_strings++] = cpu_str;

        if (emulator_type != EMULATOR_NONE) {
            strcat(cpu_str, "(");
            strcat(cpu_str, EMULATOR_TYPES[emulator_type]);
            strcat(cpu_str, ") ");
        }

        char video_str[16];
        sprintf(video_str, " Video: %s ", DISPLAY_ADAPTER_NAMES[display_adapter_type]);
        strings[n_strings++] = video_str;

        char memory_str[32];
        sprintf(memory_str, " Memory: %lu KiB free ", ipc_buffer->free_conventional_memory_bytes / 1024ul);
        strings[n_strings++] = memory_str;

        char mouse_str[16];
        sprintf(mouse_str, " Mouse: %s ", mouse_available ? "yes" : "no");
        strings[n_strings++] = mouse_str;

        screen_attr = (COLOR_DISABLED_BG << 4) | COLOR_DARK_BG;

        screen_x = SCREEN_WIDTH;
        screen_x -= 2 * (n_strings - 1);

        for (int i=0; i<n_strings; ++i) {
            screen_x -= strlen(strings[i]);
        }

        screen_x /= 2;
        screen_y = 1;

        for (int i=0; i<n_strings; ++i) {
            screen_print(strings[i]);
            screen_x += 2;
        }
    }

    screen_attr = save;
}

static void
choice_dialog_render(int x, int y, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *, const char **), void *get_label_func_user_data,
        const char *frame_label, int width, int color)
{
    int max_rows = 17;

    screen_x = x;
    screen_y = y; y += 1;

    int bg = COLOR_DEFAULT_BG;
    int fg = COLOR_DEFAULT_FG;
    int bg_cursor = COLOR_CURSOR_BG;
    int fg_cursor = COLOR_CURSOR_FG;
    int fg_scroll = COLOR_SCROLL_FG;
    int bg_frame_label = COLOR_FRAME_LABEL_BG;
    int fg_frame_label = COLOR_FRAME_LABEL_FG;

    if (!color) {
        bg = COLOR_DISABLED_BG;
        fg = COLOR_DISABLED_SOFT_CONTRAST;
        // reverse video
        bg_cursor = fg;
        fg_cursor = bg;
        fg_scroll = COLOR_DISABLED_SOFT_CONTRAST;
        bg_frame_label = COLOR_DISABLED_SOFT_CONTRAST;
        fg_frame_label = bg;
    }

    screen_attr = (bg << 4) | fg;

    int save = screen_attr;
    screen_x--;
    screen_putch(0xc9);
    for (int j=0; j<width+1; ++j) {
        screen_putch(0xcd);
    }
    screen_putch(0xbb);

    if (*frame_label) {
        screen_x = x + (width - strlen(frame_label) - 2) / 2;

        int save2 = screen_attr;
        screen_attr = (bg_frame_label << 4) | fg_frame_label;
        screen_putch(' ');
        screen_print(frame_label);
        screen_putch(' ');
        screen_attr = save2;
    }

    screen_attr = save;

    if (get_text_func != NULL) {
        int k = 0;
        const char *line = get_text_func(k++, get_text_func_user_data);
        while (line != NULL) {
            int save_attr = screen_attr;
            screen_x = x;
            screen_y = y; y += 1;
            left_border(fg, bg);
            int left_padding = 1;
            if (k == 1) {
                left_padding = (width + 1 - strlen(line)) / 2;
                // keep background
                screen_attr &= (0xf << 4);
                // override foreground
                screen_attr |= COLOR_GAME_DESCRIPTION_FG;
            }
            for (int pad=0; pad<left_padding; ++pad) {
                screen_putch(' ');
            }
            screen_print(line);
            screen_attr = save_attr;
            for (int j=width-left_padding-strlen(line); j>=0; --j) {
                screen_putch(' ');
            }
            right_border(fg, bg);

            line = get_text_func(k++, get_text_func_user_data);
        }

        screen_x = x;
        screen_y = y; y += 1;
        left_border(fg, bg);
        for (int j=width; j>=0; --j) {
            screen_putch(' ');
        }
        right_border(fg, bg);
    }

    screen_x = x;
    screen_y = y;

    int scrolloff = 3;

    if (n > max_rows) {
        if (state->cursor < state->offset + scrolloff) {
            state->offset = state->cursor - scrolloff;
            if (state->offset < 0) {
                state->offset = 0;
            }
        } else if (state->cursor > state->offset + max_rows - 1 - scrolloff) {
            state->offset = state->cursor - max_rows + scrolloff + 1;
            if (state->offset > n - max_rows) {
                state->offset = n - max_rows;
            }
        }
    }

    int begin = state->offset;
    int end = state->offset + max_rows;
    if (end > n) {
        end = n;
    }

    int pre_y = screen_y;

    for (int i=begin; i<end; ++i) {
        int save = screen_attr;
        if (i == state->cursor) {
            screen_attr = (bg_cursor << 4) | fg_cursor;
        }

        screen_x = x;
        screen_y = y; y += 1;

        left_border(fg, bg);
        screen_print(" ");
        const char *text_right = NULL;
        const char *line = get_label_func(i, get_label_func_user_data, &text_right);
        if (!line) {
            line = "";
        }
        screen_print(line);
        int padding = width-1-strlen(line)-1;
        if (text_right) {
            padding -= strlen(text_right) + 2;
        }
        for (int j=padding; j>=0; --j) {
            screen_putch(' ');
        }
        if (text_right) {
            screen_putch(' ');
            screen_putch(' ');

            int save2 = screen_attr;
            screen_attr &= ~7;
            if (display_adapter_type == DISPLAY_ADAPTER_CGA) {
                screen_attr |= COLOR_DISABLED_SOFT_CONTRAST;
            } else {
                if (!color) {
                    screen_attr |= COLOR_DISABLED_SOFT_CONTRAST;
                } else {
                    screen_attr |= COLOR_GAME_DESCRIPTION_FG;
                }
            }
            screen_print(text_right);
            screen_attr = save2;
        }
        screen_putch(' ');
        if (i == state->cursor) {
            screen_attr = save;
        }
        if (state->offset > 0 || end < n) {
            int save2 = screen_attr;

            int scrollbar_height = (end - begin) * max_rows / n;
            int scrollbar_offset = (max_rows - scrollbar_height) * state->offset / (n - max_rows);

            // adjust size to fit the top/bottom arrows
            scrollbar_offset += 1;
            scrollbar_height -= 2;

            if (i == begin) {
                if (begin > 0 && color) {
                    screen_attr = (fg_scroll << 4) | fg;
                } else {
                    screen_attr = (fg_scroll << 4) | bg;
                }
                screen_putch(0x18);
            } else if (i == end-1) {
                if (end < n && color) {
                    screen_attr = (fg_scroll << 4) | fg;
                } else {
                    screen_attr = (fg_scroll << 4) | bg;
                }
                screen_putch(0x19);
            } else if ((i - begin) >= scrollbar_offset &&
                       (i - begin) < scrollbar_offset + scrollbar_height) {
                screen_attr = (bg << 4) | fg_scroll;
                screen_putch(0xdb);
            } else {
                screen_attr = (bg << 4) | fg_scroll;
                screen_putch(0xb1);
            }
            screen_attr = save2;
            right_shadow(2);
        } else {
            right_border(fg, bg);
        }
    }

    int post_y = screen_y;

    screen_x = x;
    screen_y = y; y += 1;

    save = screen_attr;
    screen_attr = (bg << 4) | fg;
    screen_x--;
    screen_putch(0xc8);
    for (int j=0; j<width+1; ++j) {
        screen_putch(0xcd);
    }
    screen_putch(0xbc);
    right_shadow(2);

    screen_x = x + 1;
    screen_y = y; y += 1;
    right_shadow(width + 3);
}

static struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} DEFAULT_PALETTE[] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xaa },
    { 0x00, 0xaa, 0x00 },
    { 0x00, 0xaa, 0xaa },
    { 0xaa, 0x00, 0x00 },
    { 0xaa, 0x00, 0xaa },
    { 0xaa, 0x55, 0x00 },
    { 0xaa, 0xaa, 0xaa },
    { 0x55, 0x55, 0x55 },
    { 0x55, 0x55, 0xff },
    { 0x55, 0xff, 0x55 },
    { 0x55, 0xff, 0xff },
    { 0xff, 0x55, 0x55 },
    { 0xff, 0x55, 0xff },
    { 0xff, 0xff, 0x55 },
    { 0xff, 0xff, 0xff },
};

static void (*set_palette_entry)(uint8_t, uint8_t, uint8_t, uint8_t) = NULL;

/*
 * Color palette layout:
 *
 * First   Last    Count    Description
 *   0      15      16       Text-mode palette (text / bg colors)
 *  16      75      60       Background image palette, page 1
 *  76     135      60       Blurred background palette, page 1
 * 136     195      60       Background image palette, page 2
 * 196     255      60       Blurred background palette, page 2
 *                 256
 *
 * Palettes are switched between page 1 and page 2 as image sets
 * are loaded in (images are always loaded in pairs -- normal and
 * blurred), so that the previous image and the current image can
 * co-exist on the screen at the same time without palette flash.
 */

#define NUM_TEXTMODE_COLORS (16)
#define MAX_IMAGE_COLORS (60)

static uint8_t
palette_page = 0;

static clock_t
update_palette_last_time = 0;

static void
update_palette()
{
    clock_t time = 1000ull * clock() / CLOCKS_PER_SEC;

    if (!update_palette_last_time) {
        update_palette_last_time = time;
    } else {
        while (time > update_palette_last_time) {
            brightness += brightness_up_down * 16;
            if (brightness > 255) {
                brightness = 255;
            } else if (brightness < 0) {
                brightness = 0;
            }

            update_palette_last_time += 20;
        }
    }

    if (set_palette_entry) {
        for (int j=0; j<NUM_TEXTMODE_COLORS; ++j) {
            uint8_t r = (uint16_t)DEFAULT_PALETTE[j].r * brightness / 255;
            uint8_t g = (uint16_t)DEFAULT_PALETTE[j].g * brightness / 255;
            uint8_t b = (uint16_t)DEFAULT_PALETTE[j].b * brightness / 255;
            set_palette_entry(j, r, g, b);
        }
    }

    if (display_adapter_type == DISPLAY_ADAPTER_CGA) {
        int columns = 10 * (255 - brightness) / 255;
        for (int y=0; y<SCREEN_HEIGHT; ++y) {
            int dy = (y % 6) - 3;
            if (dy < 0) {
                dy = -dy;
            }
            if (dy * 2 > columns) {
                continue;
            }
            for (int x=0; x<SCREEN_WIDTH; ++x) {
                int dx = (x % 10) - 5;
                if (dx < 0) {
                    dx = -dx;
                }
                if (dx < columns) {
                    SCREEN_BUFFER[y*SCREEN_WIDTH+x][1] = 0;
                }
            }
        }
    }
}

#if defined(VGAMENU) || defined(VESAMENU)
static uint8_t __far VGA_BACKBUFFER[320L*200L];
static uint8_t __far VGA_BACKBUFFER_BLUR[320L*200L];
#endif

static inline uint8_t
sample_320_200(const uint8_t __far *backbuffer, long x, long y, long w, long h)
{
    return backbuffer[(y*200/h)*320 + x*320/w];
}

static void
present(bool ui)
{
    // wait for vertical retrace before copying
    while ((inp(0x3DA) & 8) != 0);
    while ((inp(0x3DA) & 8) == 0);

    update_palette();

#if defined(VGAMENU) || defined(VESAMENU)

#if defined(VESAMENU)
    uint32_t w = VESA_WIDTH;
    uint32_t h = VESA_HEIGHT;

    uint8_t huge *vga = fb;
#elif defined(VGAMENU)
    uint32_t w = VGA_WIDTH;
    uint32_t h = VGA_HEIGHT;

    uint8_t __far *vga = (uint8_t __far *)MK_FP(0xa000ul, 0x0000ul);
#endif

    if (ui) {
        for (long y=0; y<SCREEN_HEIGHT; ++y) {
            for (long x=0; x<SCREEN_WIDTH; ++x) {
                if (SCREEN_BUFFER[y*SCREEN_WIDTH+x][0] == SCREEN_BUFFER_OLD[y*SCREEN_WIDTH+x][0] &&
                        SCREEN_BUFFER[y*SCREEN_WIDTH+x][1] == SCREEN_BUFFER_OLD[y*SCREEN_WIDTH+x][1]) {
                    continue;
                }

                unsigned char ch = SCREEN_BUFFER[y*SCREEN_WIDTH+x][0];
                unsigned char attr = SCREEN_BUFFER[y*SCREEN_WIDTH+x][1];

                unsigned char colors[2];
                    colors[0]=(attr >> 4) & 0x0F;
                    colors[1]=(attr >> 0) & 0x0F;

#if defined(VGAMENU)
                unsigned char rows[6];
                    rows[0]=(VGAFONT[ch*3+0] >> 0) & 0x0F;
                    rows[1]=(VGAFONT[ch*3+0] >> 4) & 0x0F;
                    rows[2]=(VGAFONT[ch*3+1] >> 0) & 0x0F;
                    rows[3]=(VGAFONT[ch*3+1] >> 4) & 0x0F;
                    rows[4]=(VGAFONT[ch*3+2] >> 0) & 0x0F;
                    rows[5]=(VGAFONT[ch*3+2] >> 4) & 0x0F;

                for (uint16_t row=0; row<6; ++row) {
                    for (uint16_t column=0; column<4; ++column) {
                        uint16_t yy = y*6+row;
                        uint16_t xx = x*4+column;

                        if (ch == 0xb2 || ch == 0xb0) {
                            vga[yy*w+xx] = sample_320_200(VGA_BACKBUFFER, xx, yy, w, h);
                            continue;
                        }

                        unsigned char color = (rows[row] >> (3-column)) & 1;

                        if (color == 0) {
                            vga[yy*w+xx] = sample_320_200(VGA_BACKBUFFER_BLUR, xx, yy, w, h);
                        } else {
                            vga[yy*w+xx] = colors[color];
                        }
                    }
                }
#elif defined(VESAMENU)
                for (int row=0; row<16; ++row) {
                    for (int column=0; column<8; ++column) {
                        uint16_t yy = y*16+row;
                        uint16_t xx = x*8+column;

                        if (ch == 0xb2 || ch == 0xb0) {
                            vga[yy*w+xx] = sample_320_200(VGA_BACKBUFFER, xx, yy, w, h);
                            continue;
                        }

                        unsigned char color = (FONT_8x16[ch*16+row] >> (8-column)) & 1;

                        if (color == 0) {
                            vga[yy*w+xx] = sample_320_200(VGA_BACKBUFFER_BLUR, xx, yy, w, h);
                        } else {
                            vga[yy*w+xx] = colors[color];
                        }
                    }
                }
#endif
            }
        }
    }

#if defined(VESAMENU)
    /* copy to video ram */
    unsigned char huge *fbptr = fb;	/* fbptr needs to increment through multiple segments */
    long sz = fbsize;
    long winpos = 0;
    while(sz > 0) {
            unsigned char far *vmem = MK_FP(0xa000, 0); /* vmem does not, only 64k windows at a000h */
            vbe_setwin(winpos);
            for(long i=0; i<fb_winsz; i++) {
                    *vmem++ = *fbptr++;
            }
            sz -= fb_winsz;
            winpos += fb_winstep;
    }
#endif

    memcpy(SCREEN_BUFFER_OLD, SCREEN_BUFFER, sizeof(SCREEN_BUFFER_OLD));

#else
    short __far *screen = (short __far *)MK_FP(0xb800ul, 0x0000ul);
    _fmemcpy(screen, SCREEN_BUFFER, sizeof(SCREEN_BUFFER));
#endif
}

static int
choice_dialog_measure(int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *, const char **), void *get_label_func_user_data,
        const char *frame_label, int *out_height)
{
    int width = 20;
    int x_padding = 3;
    int height = 2;

    // Determine minimum width

    {
        int len = strlen(frame_label) + x_padding + 5;
        if (width < len) {
            width = len;
        }
    }

    if (get_text_func != NULL) {
        int k = 0;
        const char *line = get_text_func(k++, get_text_func_user_data);
        while (line != NULL) {
            ++height;

            int len = strlen(line) + x_padding;
            if (width < len) {
                width = len;
            }

            line = get_text_func(k++, get_text_func_user_data);
        }
    }

    for (int i=0; i<n; ++i) {
        const char *text_right = NULL;
        const char *line = get_label_func(i, get_label_func_user_data, &text_right);
        if (!line) {
            continue;
        }
        ++height;
        int len = strlen(line) + x_padding;
        if (text_right) {
            len += 2 + strlen(text_right);
        }
        if (width < len) {
            width = len;
        }
    }

    *out_height = height;

    return width + 1;
}


static void
generate_random_palette()
{
#if defined(VGAMENU) || defined(VESAMENU)
    // first, generate background/disabled colors
    DEFAULT_PALETTE[COLOR_DISABLED_BG].r = 100 + 0;
    DEFAULT_PALETTE[COLOR_DISABLED_BG].g = 100 + 20;
    DEFAULT_PALETTE[COLOR_DISABLED_BG].b = 100 + 80;

    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].r = DEFAULT_PALETTE[COLOR_DISABLED_BG].r/2;
    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].g = DEFAULT_PALETTE[COLOR_DISABLED_BG].g/2;
    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].b = DEFAULT_PALETTE[COLOR_DISABLED_BG].b/2;

    DEFAULT_PALETTE[COLOR_DISABLED_FG].r = DEFAULT_PALETTE[COLOR_DISABLED_BG].r/4;
    DEFAULT_PALETTE[COLOR_DISABLED_FG].g = DEFAULT_PALETTE[COLOR_DISABLED_BG].g/4;
    DEFAULT_PALETTE[COLOR_DISABLED_FG].b = DEFAULT_PALETTE[COLOR_DISABLED_BG].b/4;

    // then, generate a sane sub-palette
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].r = 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].g = 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].b = 150;

    DEFAULT_PALETTE[COLOR_BRIGHT_BG].r = 250;
    DEFAULT_PALETTE[COLOR_BRIGHT_BG].g = 250;
    DEFAULT_PALETTE[COLOR_BRIGHT_BG].b = 250;

    DEFAULT_PALETTE[COLOR_DARK_BG].r = 230;
    DEFAULT_PALETTE[COLOR_DARK_BG].g = 230;
    DEFAULT_PALETTE[COLOR_DARK_BG].b = 230;

    DEFAULT_PALETTE[COLOR_DEFAULT_FG].r = 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_FG].g = 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_FG].b = 150;
#else
    // first, generate background/disabled colors
    DEFAULT_PALETTE[COLOR_DISABLED_BG].r = 100 + rand() % 200;
    DEFAULT_PALETTE[COLOR_DISABLED_BG].g = 100 + rand() % 200;
    DEFAULT_PALETTE[COLOR_DISABLED_BG].b = 100 + rand() % 200;

    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].r = DEFAULT_PALETTE[COLOR_DISABLED_BG].r/2;
    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].g = DEFAULT_PALETTE[COLOR_DISABLED_BG].g/2;
    DEFAULT_PALETTE[COLOR_DISABLED_SOFT_CONTRAST].b = DEFAULT_PALETTE[COLOR_DISABLED_BG].b/2;

    DEFAULT_PALETTE[COLOR_DISABLED_FG].r = DEFAULT_PALETTE[COLOR_DISABLED_BG].r/4;
    DEFAULT_PALETTE[COLOR_DISABLED_FG].g = DEFAULT_PALETTE[COLOR_DISABLED_BG].g/4;
    DEFAULT_PALETTE[COLOR_DISABLED_FG].b = DEFAULT_PALETTE[COLOR_DISABLED_BG].b/4;

    // then, generate a sane sub-palette
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].r = 50 + rand() % 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].g = 50 + rand() % 150;
    DEFAULT_PALETTE[COLOR_DEFAULT_BG].b = 50 + rand() % 150;

#define BRIGHTER(x) (255 - ((255 - (x)) / 3))
#define DARKER(x) ((x) / 3)

    DEFAULT_PALETTE[COLOR_BRIGHT_BG].r = BRIGHTER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].r);
    DEFAULT_PALETTE[COLOR_BRIGHT_BG].g = BRIGHTER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].g);
    DEFAULT_PALETTE[COLOR_BRIGHT_BG].b = BRIGHTER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].b);

    DEFAULT_PALETTE[COLOR_DARK_BG].r = DARKER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].r);
    DEFAULT_PALETTE[COLOR_DARK_BG].g = DARKER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].g);
    DEFAULT_PALETTE[COLOR_DARK_BG].b = DARKER(DEFAULT_PALETTE[COLOR_DEFAULT_BG].b);

    DEFAULT_PALETTE[COLOR_DEFAULT_FG].r = DEFAULT_PALETTE[COLOR_DEFAULT_BG].r / 3;
    DEFAULT_PALETTE[COLOR_DEFAULT_FG].g = DEFAULT_PALETTE[COLOR_DEFAULT_BG].g / 3;
    DEFAULT_PALETTE[COLOR_DEFAULT_FG].b = DEFAULT_PALETTE[COLOR_DEFAULT_BG].b / 3;

#undef BRIGHTER
#undef DARKER
#endif
}

static void
store_color_palette()
{
    if (ipc_buffer) {
        ipc_buffer->color_palette_len = sizeof(DEFAULT_PALETTE)/sizeof(DEFAULT_PALETTE[0]);
        for (int i=0; i<ipc_buffer->color_palette_len; ++i) {
            ipc_buffer->color_palette[i][0] = DEFAULT_PALETTE[i].r;
            ipc_buffer->color_palette[i][1] = DEFAULT_PALETTE[i].g;
            ipc_buffer->color_palette[i][2] = DEFAULT_PALETTE[i].b;
        }
    }
}

/**
 * strcasestr() from musl libc
 *
 * Copyright © 2005-2012 Rich Felker
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **/
char *
strcasestr(const char *h, const char *n)
{
    size_t l = strlen(n);
    for (; *h; h++) if (!strncasecmp(h, n, l)) return (char *)h;
    return 0;
}

static int
match_game(const struct GameCatalog *cat, int game_idx, struct SearchState *search)
{
    const char *name = cat->names->d[game_idx];
    return (strcasestr(name, search->search_str) != NULL);
}

static int
match_subgroup(const struct GameCatalog *cat, int title_idx, struct SearchState *search)
{
    const char *name = cat->strings->d[title_idx];
    return (strcasestr(name, search->search_str) != NULL);
}


static void
search_forward_wrap_around(struct ChoiceDialogState *state)
{
    if (state->here->num_children) {
        // start searching current or forward (with wrapping)
        for (int i=state->cursor - 1; i<state->here->num_children; ++i) {
            int game_idx = state->here->children[i];
            if (match_game(state->cat, game_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }

        // wrap around search
        for (int i=0; i<state->cursor - 1; ++i) {
            int game_idx = state->here->children[i];
            if (match_game(state->cat, game_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    } else {
        // group search

        // start searching current or forward (with wrapping)
        for (int i=state->cursor - 1; i<state->here->num_subgroups; ++i) {
            int title_idx = state->here->subgroups[i]->title_idx;
            if (match_subgroup(state->cat, title_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }

        // wrap around search
        for (int i=0; i<state->cursor - 1; ++i) {
            int title_idx = state->here->subgroups[i]->title_idx;
            if (match_subgroup(state->cat, title_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    }
}

static void
search_next(struct ChoiceDialogState *state)
{
    if (state->here->num_children) {
        // search for next without exiting search
        for (int i=state->cursor - 1 + 1; i<state->here->num_children; ++i) {
            int game_idx = state->here->children[i];
            if (match_game(state->cat, game_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    } else {
        // group search

        // search for next without exiting search
        for (int i=state->cursor - 1 + 1; i<state->here->num_subgroups; ++i) {
            int title_idx = state->here->subgroups[i]->title_idx;
            if (match_subgroup(state->cat, title_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    }
}

static void
search_previous(struct ChoiceDialogState *state)
{
    if (state->here->num_children) {
        // search for previous without exiting search
        for (int i=state->cursor - 1 - 1; i>= 0; --i) {
            int game_idx = state->here->children[i];
            if (match_game(state->cat, game_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    } else {
        // group search

        // search for previous without exiting search
        for (int i=state->cursor - 1 - 1; i>= 0; --i) {
            int title_idx = state->here->subgroups[i]->title_idx;
            if (match_subgroup(state->cat, title_idx, &state->search)) {
                /* + 1 because of ".." navigation */
                state->cursor = i + 1;
                return;
            }
        }
    }
}

static int
choice_dialog_handle_input(struct ChoiceDialogState *state, int n)
{
    int page_size = 5;

    // only busy-loop if we're actively fading the menu, to
    // avoid extraneous redraws / "snow" in the graphics
    if (is_fading()) {
        if (!kbhit()) {
            present(true);
            return 0;
        }
    }

    int ch = getch();
    if (ch == 0) {
        ch = 0x100 | getch();
    }

    if (state->search.now_searching) {
        if (ch == KEY_DOWN) {
            search_next(state);
            screenshot_idx = 0;
        } else if (ch == KEY_UP) {
            search_previous(state);
            screenshot_idx = 0;
        } else if (ch == KEY_ENTER || ch == KEY_ESCAPE || ch == KEY_F3) {
            if (ch == KEY_ESCAPE) {
                // restore previous cursor and abort search
                state->cursor = state->search.saved_cursor;
                state->offset = state->search.saved_offset;
                state->search.search_str[0] = '\0';
                state->search.search_pos = 0;
            } else {
                // allow searching again
                state->search.previous_input = 1;
            }
            state->search.now_searching = 0;
            disable_blinking_cursor();
            return 0;
        } else if (state->search.search_pos > 0 && ch == KEY_BACKSPACE) {
            state->search.previous_input = 0;
            state->search.search_str[--state->search.search_pos] = '\0';

            // reset cursor and restart search
            state->cursor = state->search.saved_cursor;
            state->offset = state->search.saved_offset;

            search_forward_wrap_around(state);
        } else if (state->search.search_pos < sizeof(state->search.search_str) - 1 &&
                    ((ch >= 'a' && ch <= 'z') ||
                     (ch >= 'A' && ch <= 'Z') ||
                     (ch >= '0' && ch <= '9'))) {
            if (state->search.previous_input) {
                state->search.search_pos = 0;
                state->search.search_str[0] = '\0';
                state->search.previous_input = 0;
            }

            state->search.search_str[state->search.search_pos++] = ch;
            state->search.search_str[state->search.search_pos] = '\0';

            search_forward_wrap_around(state);
        }

        return 0;
    }

    if (ch == KEY_NPAGE) {
        if (state->cursor >= n - page_size) {
            state->cursor = n - 1;
        } else {
            state->cursor += page_size;
        }
        screenshot_idx = 0;
    } else if (ch == KEY_PPAGE) {
        if (state->cursor < page_size) {
            state->cursor = 0;
        } else {
            state->cursor -= page_size;
        }
        screenshot_idx = 0;
    } else if (ch == KEY_HOME) {
        state->cursor = 0;
        screenshot_idx = 0;
    } else if (ch == KEY_END) {
        state->cursor = n - 1;
        screenshot_idx = 0;
    } else if (ch == KEY_DOWN) {
        if (state->cursor < n - 1) {
            if (state->game == -1) {
                screenshot_idx = 0;
            }
            state->cursor += 1;
        }
    } else if (ch == KEY_LEFT) {
        screenshot_idx--;
    } else if (ch == KEY_RIGHT) {
        screenshot_idx++;
    } else if (ch == KEY_UP) {
        if (state->cursor > 0) {
            state->cursor -= 1;
            if (state->game == -1) {
                screenshot_idx = 0;
            }
        }
    } else if (ch == KEY_ENTER) {
        return 1;
    } else if (ch == KEY_F2) {
#if !defined(VGAMENU) && !defined(VESAMENU)
        if (display_adapter_type == DISPLAY_ADAPTER_CGA) {
            COLOR_DEFAULT_BG = 1 + (rand()%6);
            do {
                COLOR_DARK_BG = 1 + (rand()%6);
            } while (COLOR_DARK_BG == COLOR_DEFAULT_BG);

            update_color_bindings();
            update_palette();
        } else {
            brightness_up_down = -1;
            while (brightness > 0) {
                update_palette();
            }

            generate_random_palette();
            store_color_palette();

            brightness_up_down = 1;
            while (brightness < 255) {
                update_palette();
            }
        }
#endif
    } else if (ch == KEY_F3 || ch == '/') {
        if (state->game == -1 && !state->search.now_searching) {
            state->search.now_searching = 1;
            state->search.saved_cursor = state->cursor;
            state->search.saved_offset = state->offset;
        }
    } else if (ch == KEY_F5) {
        // TODO: Need to store current menu state in ipc_buffer
        // TODO: Could check which video modes are supported, and only switch
        // to the video mode that is supported (no VGA on CGA, EGA, and no VESA
        // on anything that doesn't support VESA, obviously...)
        ipc_buffer->menu_mode = (ipc_buffer->menu_mode + 1) % MENU_MODE_COUNT;
        ipc_buffer->request = IPC_SWITCH_MENU_MODE;
        textmode_reset();
        exit(0);
    } else if (ch == KEY_F4 && (display_adapter_type == DISPLAY_ADAPTER_VGA || display_adapter_type == DISPLAY_ADAPTER_VESA)) {
#if defined(VGAMENU) || defined(VESAMENU)
        while (1) {
            if (state->game != -1) {
                // on a game page
                show_screenshots(state->cat, state->game);
            } else if (state->here && state->here->num_children && state->cursor > 0) {
                // in a choice screen with a games list
                show_screenshots(state->cat, state->here->children[state->cursor-1]);
            } else {
                show_screenshots(state->cat, -1);
            }

            present(false);

            int ch = getch();
            if (ch == 0) {
                ch = 0x100 | getch();
            }

            if (ch == KEY_ESCAPE || ch == KEY_F4) {
                // F4 toggles screenshots on and off
                // ESC to get out of screenshot mode
                break;
            } else if (ch == KEY_LEFT) {
                screenshot_idx--;
            } else if (ch == KEY_RIGHT) {
                screenshot_idx++;
            }
        }
#else
        int game_id = -1;

        if (state->game != -1 && state->cat->games[state->game].num_screenshots > 0) {
            game_id = state->game;
        } else if (state->here && state->here->num_children && state->cursor > 0) {
            game_id = state->here->children[state->cursor-1];
        }

        if (game_id != -1) {
            char cmdline[128];
            sprintf(cmdline, "viewshot.exe pcx/%s", state->cat->ids->d[game_id]);
            system(cmdline);
            configure_text_mode();
        }
#endif
    } else if (ch == KEY_ESCAPE) {
        state->cursor = 0;
        if (state->game == -1) {
            screenshot_idx = 0;
        }
        return 1;
    }

    return 0;
}

static int
choice_dialog(int x, int y, const char *title, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *, const char **), void *get_label_func_user_data,
        void (*render_background_func)(void *), void *render_background_func_user_data,
        const char *frame_label)
{
    int height = -1;
    int width = choice_dialog_measure(n,
            get_text_func, get_text_func_user_data,
            get_label_func, get_label_func_user_data,
            frame_label, &height);

#if defined(VGAMENU) || defined(VESAMENU)
    if (height > 19) {
        height = 19;
    }

    y = (SCREEN_HEIGHT - height - 3);
#else
    if (y == -1) {
        y = 5;
        if (y + height > 20) {
            y = (SCREEN_HEIGHT - height - 1) / 2;
        }
    }
#endif

    if (x == -1) {
        x = (SCREEN_WIDTH - width) / 2;
    }

    while (1) {
        render_background(title);

        if (render_background_func) {
            render_background_func(render_background_func_user_data);
        }

#if defined(VGAMENU) || defined(VESAMENU)
        if (state->game != -1) {
            // on a game page
            show_screenshots(state->cat, state->game);
        } else if (state->here && state->here->num_children && state->cursor > 0) {
            // in a choice screen with a games list
            show_screenshots(state->cat, state->here->children[state->cursor-1]);
        } else {
            // default
            show_screenshots(state->cat, -1);
        }
#endif

        choice_dialog_render(x, y, state, n,
                get_text_func, get_text_func_user_data,
                get_label_func, get_label_func_user_data,
                frame_label, width, 1);

        if (state->search.now_searching) {
            draw_statusbar(state->search.search_str);
        }

        present(true);

        if (choice_dialog_handle_input(state, n)) {
            break;
        }
    }

    return state->cursor;
}

struct GetLabelGroupUserData {
    struct GameCatalog *cat;
    struct GameCatalogGroup *group;
};

static const char *
get_excuse(const struct GameCatalogGame *game)
{
    // Order these so that "hard" requirements (e.g. graphics card, which involves
    // changing hardware) are first, and "soft" requirements (e.g. mouse driver,
    // which may just involve loading one) are last. In case where the same type
    // of hardware is listed (e.g. graphics card), order oldest to newest (e.g.
    // list EGA before VGA).

    if ((game->flags & FLAG_DOSBOX_INCOMPATIBLE) && emulator_type == EMULATOR_DOSBOX) {
        return "Incompatible with DOSBox";
    }

    if ((game->flags & FLAG_IS_32_BITS) != 0 && !have_32bit_cpu) {
        return "Needs 32-bit CPU";
    }

    if ((game->flags & FLAG_REQUIRES_EGA) != 0 && display_adapter_type == DISPLAY_ADAPTER_CGA) {
        return "Needs EGA";
    }

    if ((game->flags & FLAG_REQUIRES_VGA) != 0 && (display_adapter_type == DISPLAY_ADAPTER_CGA ||
                                                   display_adapter_type == DISPLAY_ADAPTER_EGA)) {
        return "Needs VGA";
    }

    if ((game->flags & FLAG_REQUIRES_VBE1) != 0 && display_adapter_type != DISPLAY_ADAPTER_VGA && display_adapter_type != DISPLAY_ADAPTER_VESA) {
        return "Needs VBE";
    }

    if ((game->flags & FLAG_REQUIRES_VBE2) && display_adapter_type != DISPLAY_ADAPTER_VGA && display_adapter_type != DISPLAY_ADAPTER_VESA) {
        return "Needs VBE 2.x";
    }

    if ((game->flags & FLAG_MOUSE_REQUIRED) != 0 && !mouse_available) {
        return "Needs mouse";
    }

    return NULL;
}

const char *
get_label_group(int i, void *user_data, const char **text_right)
{
    static char tmp_buf[64];

    struct GetLabelGroupUserData *ud = user_data;

    if (i == 0) {
        if (ud->group->parent_group != NULL) {
            return "(..)";
        } else {
            return "Exit to DOS";
        }
    } else {
        i -= 1;

        if (i < ud->group->num_children) {
            int game_idx = ud->group->children[i];

            //const char *excuse = get_excuse(&(ud->cat->games[game_idx]));
            //*text_right = excuse;

            return ud->cat->names->d[game_idx];
        }

        if (i < ud->group->num_subgroups) {
            struct GameCatalogGroup *subgroup = ud->group->subgroups[i];

            if (subgroup->num_children) {
                sprintf(tmp_buf, "%d", subgroup->num_children);
                *text_right = tmp_buf;
            }

            return ud->cat->strings->d[subgroup->title_idx];
        }
    }

    return NULL;
}

struct GetTextGameUserData {
    struct GameCatalog *cat;
    int game;
};

const char *
get_text_game(int i, void *user_data)
{
    static char fmt_buf[80];

    struct GetTextGameUserData *ud = user_data;

    const struct GameCatalogGame *game = &(ud->cat->games[ud->game]);

    if (i == 0) {
        return ud->cat->descriptions->d[ud->game];
    }
    --i;

    if (i == 0) {
        return "";
    }
    --i;

    struct GameCatalogStringList *authors_idx = ud->cat->string_lists->d + game->author_list_idx;
    for (int j=0; j<authors_idx->n; ++j) {
        if (i == 0) {
            if (j == 0) {
                sprintf(fmt_buf, "by ");
            } else {
                sprintf(fmt_buf, "   ");
            }
            sprintf(fmt_buf + 3, "%s%s", ud->cat->strings->d[authors_idx->d[j]],
                    (j == authors_idx->n - 1) ? "" : (((j == authors_idx->n - 2) ? " and" : ",")));
            return fmt_buf;
        }
        --i;
    }

    if (i == 0) {
        return ud->cat->urls->d[ud->game];
    }
    --i;

    if (game->jam_idx && game->genre_idx) {
        if (i == 0) {
            sprintf(fmt_buf, "Jam: %s, Genre: %s",
                    ud->cat->strings->d[game->jam_idx],
                    ud->cat->strings->d[game->genre_idx]);
            return fmt_buf;
        }
        --i;
    }

    if (i == 0) {
        sprintf(fmt_buf, "%s%s",
                (game->flags & FLAG_IS_32_BITS) ? "386+" : "8088+",
                (game->flags & FLAG_IS_MULTIPLAYER) ? ", multiplayer" : "",
                ud->cat->strings->d[game->genre_idx]);

        if ((game->flags & FLAG_MOUSE_SUPPORTED) != 0) {
            if ((game->flags & FLAG_MOUSE_REQUIRED) != 0) {
                strcat(fmt_buf, ", requires mouse");
            } else {
                strcat(fmt_buf, ", supports mouse");
            }
        }

        if ((game->flags & FLAG_IS_OPEN_SOURCE) != 0) {
            strcat(fmt_buf, ", open source");
        }

        struct GameCatalogStringList *video_idx = ud->cat->string_lists->d + game->video_list_idx;
        for (int j=0; j<video_idx->n; ++j) {
            strcat(fmt_buf, ", ");
            strcat(fmt_buf, ud->cat->strings->d[video_idx->d[j]]);
        }

        return fmt_buf;
    }
    --i;

    const char *excuse = get_excuse(game);
    if (excuse) {
        if (i == 0) {
            sprintf(fmt_buf, "Not supported on this machine (%s)", excuse);
            return fmt_buf;
        }
        --i;
    }

    return NULL;
}

enum GameLaunchChoice {
    CHOICE_BACK = 0,
    CHOICE_LAUNCH = 1,
    CHOICE_README = 2,
    MAX_CHOICES,
};

static const char *
GAME_LAUNCH_CHOICE[] = {
    "(..)",
    "Launch",
    "Readme",
};

struct GameLaunchChoices {
    uint8_t choices[MAX_CHOICES]; /* enum GameLaunchChoice */
    uint8_t count;
};

const char *
get_label_game(int i, void *user_data, const char **text_right)
{
    struct GameLaunchChoices *choices = user_data;

    if (i < choices->count) {
        return GAME_LAUNCH_CHOICE[choices->choices[i]];
    }

    return NULL;
}

struct BackgroundRenderUserData {
    struct GameCatalog *cat;
    struct GameCatalogGroup *here;
    int width;
};

static void
render_group_background(void *user_data)
{
    struct BackgroundRenderUserData *ud = user_data;

    struct GetLabelGroupUserData glgud;
    glgud.cat = ud->cat;
    glgud.group = ud->here;
    int max = (ud->here->num_children ? ud->here->num_children : ud->here->num_subgroups);

    struct ChoiceDialogState cds;
    reset_search_state(&cds.search);
    cds.cat = ud->cat;
    cds.here = ud->here;
    cds.game = -1;
    cds.cursor = ud->here->cursor_index;
    cds.offset = ud->here->scroll_offset;

    const char *frame_label = ud->cat->strings->d[ud->here->title_idx];

    if (ud->width == 0) {
        ud->width = choice_dialog_measure(max + 1,
                NULL, NULL,
                get_label_group, &glgud,
                frame_label, NULL);
    }

    choice_dialog_render(2, 3, &cds, max + 1,
            NULL, NULL,
            get_label_group, &glgud,
            frame_label, ud->width, 0);
}

static void
ipc_buffer_add_menu_trail_entry(int selection, struct GameCatalogGroup *here)
{
    if (ipc_buffer) {
        struct MenuTrailEntry __far *entry = &(ipc_buffer->menu_trail[ipc_buffer->menu_trail_len++]);
        entry->selection = selection;
        entry->cursor_index = here->cursor_index;
        entry->scroll_offset = here->scroll_offset;
    }
}

static void
ipc_buffer_pop_menu_trail_entry(void)
{
    if (ipc_buffer) {
        ipc_buffer->menu_trail_len--;
    }
}

static void
fade_out()
{
#if !defined(VGAMENU) && !defined(VESAMENU)
    brightness_up_down = -1;
    while (brightness) {
        present(true);
    }
#endif
}

static void
configure_text_mode()
{
    brightness = 0;
    brightness_up_down = 1;
    update_palette_last_time = 0;

    disable_blinking_cursor();
    enable_4bit_background();
    update_palette();
}

struct PCXHeader {
    uint8_t header_field; // 0x0a
    uint8_t version; // 5
    uint8_t encoding; // 1 = RLE
    uint8_t bpp; // 8
    uint16_t min_x;
    uint16_t min_y;
    uint16_t max_x;
    uint16_t max_y;
    uint16_t dpi_x;
    uint16_t dpi_y;
    uint8_t ega_palette[16][3];
    uint8_t reserved0;
    uint8_t num_planes; // 1
    uint16_t stride_bytes; // 320
    uint16_t palette_mode; // 1 = color
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t reserved1[54];
};

static void
vga_present_pcx(const char *filename, uint8_t __far *VGA, int palette_offset)
{
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        struct PCXHeader header;
        size_t res = fread(&header, sizeof(header), 1, fp);
        if (res == 1) {
            int width = (header.max_x - header.min_x + 1);
            int height = (header.max_y - header.min_y + 1);

            if (header.header_field == 0x0a &&
                    header.version == 5 &&
                    header.encoding == 1 &&
                    header.bpp == 8 &&
                    header.num_planes == 1 &&
                    header.stride_bytes == 320 &&
                    header.palette_mode == 1 &&
                    width == 320 &&
                    height == 200) {
            } else {
                printf("Error, unsupported format:\n");
                printf("hdr: 0x%04x\n", header.header_field);
                printf("version: %d\n", header.version);
                printf("encoding: %d\n", header.encoding);
                printf("bpp: %d\n", header.bpp);
                printf("res: [%d, %d]-[%d, %d]\n", header.min_x, header.min_y, header.max_x, header.max_y);
                printf("num planes: %d\n", header.num_planes);
                printf("stride_bytes: %d\n", header.stride_bytes);
                printf("palette_mode: %d\n", header.palette_mode);
                printf("resolution: [%d, %d]\n", header.x_resolution, header.y_resolution);
            }

            long pixels_processed = 0;

            static uint8_t chunk[512];
            int len = 0;
            int pos = 0;

            while (pixels_processed < (long)width * (long)height) {
                if (pos == len) {
                    pos = 0;
                    len = fread(chunk, 1, sizeof(chunk), fp);
                }
                uint8_t ch = chunk[pos++];

                if ((ch & 0xc0) == 0xc0) {
                    uint8_t repeat = ch & 0x3f;

                    if (pos == len) {
                        pos = 0;
                        len = fread(chunk, 1, sizeof(chunk), fp);
                    }
                    ch = chunk[pos++];

                    for (int i=0; i<repeat; ++i) {
                        VGA[pixels_processed++] = ch + NUM_TEXTMODE_COLORS + palette_offset;
                    }
                } else {
                    VGA[pixels_processed++] = ch + NUM_TEXTMODE_COLORS + palette_offset;
                }
            }

            fseek(fp, -(768 + 1), SEEK_END);
            int ch = fgetc(fp);
            if (ch == 12) {
                for (int i=0; i<MAX_IMAGE_COLORS; ++i) {
                    int r = fgetc(fp);
                    int g = fgetc(fp);
                    int b = fgetc(fp);

                    set_palette_entry(i+NUM_TEXTMODE_COLORS+palette_offset, r, g, b);
                }
            } else {
                // TODO: Error message (no palette)
            }
        } else {
            printf("Error reading file\n");
        }

        fclose(fp);
    } else {
        //printf("Cannot open %s\n", filename);
        //printf("Press ESC to go back to menu.\n");
    }
}

#if defined(VGAMENU) || defined(VESAMENU)
static void
show_screenshots(struct GameCatalog *cat, int game)
{
    const char *game_name = "default";

    if (game != -1 && cat->games[game].num_screenshots > 0) {
        game_name = cat->ids->d[game];
        if (screenshot_idx < 0) {
            screenshot_idx += cat->games[game].num_screenshots;
        }
        screenshot_idx %= cat->games[game].num_screenshots;
    } else {
        screenshot_idx = 0; // FIXME: Hardcoded
    }

    char filename[128];

    bool backbuffer_changed = false;

    static char current_backbuffer_filename[128];
    sprintf(filename, "pcx/%s/shot%d.pcx", game_name, screenshot_idx);
    if (strcmp(filename, current_backbuffer_filename) != 0) {
        vga_present_pcx(filename, VGA_BACKBUFFER, palette_page * (2 * MAX_IMAGE_COLORS));
        strcpy(current_backbuffer_filename, filename);
        palette_page = 1 - palette_page;
        backbuffer_changed = true;
    }

    static char current_backbuffer_blur_filename[128];
    sprintf(filename, "pcx/%s/blur%d.pcx", game_name, screenshot_idx);
    if (strcmp(filename, current_backbuffer_blur_filename) != 0) {
        vga_present_pcx(filename, VGA_BACKBUFFER_BLUR, palette_page * (2 * MAX_IMAGE_COLORS) + MAX_IMAGE_COLORS);
        strcpy(current_backbuffer_blur_filename, filename);
        if (!backbuffer_changed) {
            palette_page = 1 - palette_page;
            backbuffer_changed = true;
        }
    }

    if (backbuffer_changed) {
        // invalidate screen buffer to force a full-screen update
        memset(SCREEN_BUFFER_OLD, 0, sizeof(SCREEN_BUFFER_OLD));
    }
}
#endif

int main(int argc, char *argv[])
{
    // Empty keyboard buffer
    while (kbhit()) {
        getch();
    }

    srand(time(NULL));

    display_adapter_type = detect_display_adapter();
    cpu_type = detect_cpu_type();
    have_32bit_cpu = (cpu_type != CPU_8086 && cpu_type != CPU_286);
    emulator_type = detect_dos_emulator();
    mouse_available = have_mouse_driver();

#if defined(VGAMENU) || defined(VESAMENU)
    set_palette_entry = vga_set_palette_entry_direct;
#else
    switch (display_adapter_type) {
        case DISPLAY_ADAPTER_CGA:
            set_palette_entry = NULL;
            break;
        case DISPLAY_ADAPTER_EGA:
            set_palette_entry = ega_set_palette_entry;
            break;
        case DISPLAY_ADAPTER_VGA:
            set_palette_entry = vga_set_palette_entry;
            break;
    }
#endif

    if (argc == 2) {
        static struct {
            int their_ds;
            int their_offset;
        } dos_ipc = { 0, 0 };

        if (sscanf(argv[1], "%x:%x", &dos_ipc.their_ds, &dos_ipc.their_offset) != 2) {
            dos_ipc.their_ds = 0;
            dos_ipc.their_offset = 0;
        }

        ipc_buffer = (struct IPCBuffer __far *)(MK_FP((uint32_t)dos_ipc.their_ds, (uint32_t)dos_ipc.their_offset));

        if (ipc_buffer && ipc_buffer->magic != IPC_BUFFER_MAGIC) {
            ipc_buffer = NULL;
        }

#if defined(VGAMENU) || defined(VESAMENU)
        // Reset color palette when entering graphics modes
        generate_random_palette();
#endif

        if (ipc_buffer) {
            if (ipc_buffer->color_palette_len == 0) {
                generate_random_palette();
#if !defined(VGAMENU) && !defined(VESAMENU)
                // Only save color palette in ipc_buffer in text mode
                store_color_palette();
#endif
            } else {
#if !defined(VGAMENU) && !defined(VESAMENU)
                // Only restore color palette from ipc_buffer in text mode
                for (int i=0; i<ipc_buffer->color_palette_len; ++i) {
                    DEFAULT_PALETTE[i].r = ipc_buffer->color_palette[i][0];
                    DEFAULT_PALETTE[i].g = ipc_buffer->color_palette[i][1];
                    DEFAULT_PALETTE[i].b = ipc_buffer->color_palette[i][2];
                }
#endif
            }
        }
    }

    if (!ipc_buffer) {
        printf("Use START.EXE to start the menu.\n");
        return 1;
    }

    char *filename = malloc(sizeof(ipc_buffer->catalog_filename));
    _fmemcpy(filename, ipc_buffer->catalog_filename, sizeof(ipc_buffer->catalog_filename));
    FILE *fp = fopen(filename, "rb");
    int did_read = 0;
    char *buf = 0;
    size_t len = 0;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = malloc(len);
        if (fread(buf, len, 1, fp) == 1) {
            did_read = 1;
        }
        fclose(fp);
    }

    if (!did_read) {
        printf("Failed to read '%s'\n", filename);
        getch();
        exit(1);
    }

#if defined(VGAMENU)
    {
        union REGS inregs, outregs;
        inregs.h.ah = 0;
        inregs.h.al = 0x13;

        int86(0x10, &inregs, &outregs);
    }
#elif defined(VESAMENU)
    if(vbe_getinfo(&vbe) == -1) {
        ipc_buffer->menu_mode = MENU_MODE_TEXT;
        ipc_buffer->request = IPC_SWITCH_MENU_MODE;
        textmode_reset();
        exit(0);
    }

    if(vbe_getmodeinfo(0x101, &minf) == -1) {
        ipc_buffer->menu_mode = MENU_MODE_TEXT;
        ipc_buffer->request = IPC_SWITCH_MENU_MODE;
        textmode_reset();
        exit(0);
    }

    /* allocate main memory framebuffer and fill it with something */
    fbsize = (long)VESA_WIDTH * (long)VESA_HEIGHT;
    if(!(fb = halloc(fbsize, 1))) {
        ipc_buffer->menu_mode = MENU_MODE_TEXT;
        ipc_buffer->request = IPC_SWITCH_MENU_MODE;
        textmode_reset();
        exit(0);
    }

    /* calc window size in bytes and window stepping in granularity units */
    fb_winsz = (unsigned long)minf.win_size << 10;
    fb_winstep = 0;
    long sz = 0;
    while(sz < fb_winsz) {
            fb_winstep++;
            sz += (unsigned long)minf.win_gran << 10;
    }


    /* set video mode 640x480 8bpp */
    vbe_setmode(0x101);

    display_adapter_type = DISPLAY_ADAPTER_VESA;
#else
    configure_text_mode();
#endif

    // cat takes ownership of "buf"
    struct GameCatalog *cat = game_catalog_parse(buf, len);

    struct GameCatalogGroup *here = cat->grouping;
    int game = -1;

    /* Restore menu state if restarted */
    if (ipc_buffer) {
        for (int i=0; i<ipc_buffer->menu_trail_len; ++i) {
            struct MenuTrailEntry entry = ipc_buffer->menu_trail[i];

            here->cursor_index = entry.cursor_index;
            here->scroll_offset = entry.scroll_offset;

            if (here->num_subgroups) {
                if (entry.selection < here->num_subgroups) {
                    here = here->subgroups[entry.selection];
                }
            } else {
                if (entry.selection < here->num_children) {
                    game = here->children[entry.selection];
                }
            }
        }
    }

    while (1) {
        char buf[512];
        strcpy(buf, "");

        if (here->parent_group) {
            print_parents_first(buf, cat, here->parent_group);
        } else {
            sprintf(buf, "%s -- Git rev %s (built %s %s)",
                    cat->strings->d[cat->grouping->title_idx],
                    VERSION, BUILDDATE, BUILDTIME);
        }

        if (game != -1) {
            struct GetTextGameUserData gtgud;
            gtgud.cat = cat;
            gtgud.game = game;

            struct BackgroundRenderUserData brud;
            brud.cat = cat;
            brud.here = here;
            brud.width = 0;

            struct ChoiceDialogState cds;
            reset_search_state(&cds.search);
            cds.cat = cat;
            cds.here = here;
            cds.game = game;
            cds.cursor = 0;
            cds.offset = 0;

            struct GameLaunchChoices glc;
            memset(&glc, 0, sizeof(glc));
            glc.choices[glc.count++] = CHOICE_BACK;

            if (get_excuse(&(cat->games[game])) == NULL) {
                // Only enable launching if supported on this machine
                glc.choices[glc.count++] = CHOICE_LAUNCH;
            }

            // TODO: Add README support
            if (false) {
                glc.choices[glc.count++] = CHOICE_README;
            }

            int selection = choice_dialog(-1, -1, buf, &cds, glc.count,
                    get_text_game, &gtgud,
                    get_label_game, &glc,
#if defined(VGAMENU) || defined(VESAMENU)
                    NULL, NULL,
#else
                    render_group_background, &brud,
#endif
                    cat->names->d[game]);

            if (glc.choices[selection] == CHOICE_BACK) {
                ipc_buffer_pop_menu_trail_entry();
                game = -1;
            } else if (glc.choices[selection] == CHOICE_README) {
                // TODO
            } else if (glc.choices[selection] == CHOICE_LAUNCH) {
                // TODO: Index 0 forced here, so that the first choice (back)
                // is pre-selected when we return from the launched game
                ipc_buffer_add_menu_trail_entry(0, here);

                // pop menu selection again
                ipc_buffer_pop_menu_trail_entry();

                if (ipc_buffer) {
                    ipc_buffer->request = IPC_RUN_GAME;
                    ipc_buffer->game_flags = cat->games[game].flags;

                    _fstrcpy(ipc_buffer->title, cat->names->d[game]);

                    _fstrcpy(ipc_buffer->cmdline, cat->strings->d[cat->games[game].prefix_idx]);
                    _fstrcat(ipc_buffer->cmdline, cat->strings->d[cat->games[game].run_idx]);

                    fade_out();
                    return 0;
                }
            } else {
                // Unknown selection
            }
        } else {
            struct GetLabelGroupUserData glgud;
            glgud.cat = cat;
            glgud.group = here;
            int max = (here->num_children ? here->num_children : here->num_subgroups);
            struct ChoiceDialogState cds;
            reset_search_state(&cds.search);
            cds.cat = cat;
            cds.here = here;
            cds.game = game;
            cds.cursor = here->cursor_index;
            cds.offset = here->scroll_offset;

#if defined(VGAMENU) || defined(VESAMENU)
            int selection = choice_dialog(2, 20, buf, &cds, max + 1,
                    NULL, NULL,
                    get_label_group, &glgud,
                    NULL, NULL,
                    cat->strings->d[here->title_idx]);
#else
            int selection = choice_dialog(2, 3, buf, &cds, max + 1,
                    NULL, NULL,
                    get_label_group, &glgud,
                    NULL, NULL,
                    cat->strings->d[here->title_idx]);
#endif
            here->cursor_index = cds.cursor;
            here->scroll_offset = cds.offset;

            if (selection == 0) {
                if (here->parent_group != NULL) {
                    ipc_buffer_pop_menu_trail_entry();
                    here = here->parent_group;
                } else {
                    break;
                }
            } else {
                selection -= 1;

                ipc_buffer_add_menu_trail_entry(selection, here);

                if (here->num_subgroups) {
                    if (selection >= 0 && selection < here->num_subgroups) {
                        here = here->subgroups[selection];
                    }
                } else {
                    if (selection >= 0 && selection < here->num_children) {
                        game = here->children[selection];
                    }
                }
            }
        }
    }

    game_catalog_free(cat);

    if (ipc_buffer) {
        ipc_buffer->request = IPC_EXIT;
    }

    fade_out();

#if defined(VESAMENU)
    hfree(fb);
    vbe_setmode(3);
#endif
    textmode_reset();

    return 0;
}
