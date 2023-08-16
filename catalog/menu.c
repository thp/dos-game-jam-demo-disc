#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos.h>
#include <time.h>

#include <conio.h>
#define KEY_DOWN (0x100 | 80)
#define KEY_UP (0x100 | 72)
#define KEY_NPAGE (0x100 | 81)
#define KEY_PPAGE (0x100 | 73)
#define KEY_HOME (0x100 | 71)
#define KEY_END (0x100 | 79)
#define KEY_F2 (0x100 | 60)
#define KEY_F3 (0x100 | 61)
#define ENTERKEY (13)

#include "gamectlg.h"
#include "ipc.h"
#include "vgautil.h"

static struct IPCBuffer __far *
ipc_buffer = NULL;

static enum DisplayAdapter
display_adapter_type;

static void
show_screenshots(struct GameCatalog *cat, int game);

static void
configure_text_mode();

#define SCREEN_WIDTH (80)
#define SCREEN_HEIGHT (25)

static uint8_t
SCREEN_BUFFER[80*25][2];

static uint8_t
screen_x = 0;

static uint8_t
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
    COLOR_FRAME_LABEL_FG = 0xf,

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
brightness = 0;

static int
brightness_up_down = 1;

static void
screen_putch(char ch)
{
    if (ch == '\n') {
        screen_x = 0;
        if (screen_y <= SCREEN_HEIGHT) {
            screen_y++;
        }
    } else {
        SCREEN_BUFFER[SCREEN_WIDTH * screen_y + screen_x][0] = ch;
        SCREEN_BUFFER[SCREEN_WIDTH * screen_y + screen_x][1] = screen_attr;
        screen_x++;
        if (screen_x == SCREEN_WIDTH) {
            if (screen_y <= SCREEN_HEIGHT) {
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

struct ChoiceDialogState {
    struct GameCatalog *cat;
    struct GameCatalogGroup *here;
    int game;

    int cursor;
    int offset;
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

        SCREEN_BUFFER[(SCREEN_HEIGHT-1)*SCREEN_WIDTH+i][0] = ' ';
        SCREEN_BUFFER[(SCREEN_HEIGHT-1)*SCREEN_WIDTH+i][1] =
            (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_BG;
    }

    if (*title) {
        screen_y = 0;
        screen_x = (SCREEN_WIDTH - strlen(title) - 2) / 2;

        int save = screen_attr;
        screen_attr = (COLOR_FRAME_LABEL_BG << 4) | COLOR_FRAME_LABEL_FG;
        screen_putch(' ');
        screen_print(title);
        screen_putch(' ');
        screen_attr = save;
    }

    screen_y = SCREEN_HEIGHT - 1;
    screen_x = 2;
    int save = screen_attr;

    static const struct {
        uint8_t only_vga;
        const char *key;
        const char *func;
    } KEYS[] = {
        { 0, "ESC", "Back" },
        { 0, "F2", "Color Theme" },
        { 1, "F3", "Screenshots" },
        { 0, "Enter", "Go" },
        { 0, "Arrows", "Move" },
    };

    for (int i=0; i<sizeof(KEYS)/sizeof(KEYS[0]); ++i) {
        if (KEYS[i].only_vga && display_adapter_type != DISPLAY_ADAPTER_VGA) {
            continue;
        }

        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_KEY;
        screen_print(KEYS[i].key);
        screen_attr = (COLOR_HOTKEY_BG << 4) | COLOR_HOTKEY_LABEL;
        screen_print(" ");
        screen_print(KEYS[i].func);

        screen_print("  ");
    }

    if (ipc_buffer) {
        char tmp[48];
        sprintf(tmp, " Graphics: %s ", DISPLAY_ADAPTER_NAMES[display_adapter_type]);

        char tmp2[48];
        sprintf(tmp2, " Memory: %lu KiB free ", ipc_buffer->free_conventional_memory_bytes / 1024ul);

        screen_attr = (COLOR_DISABLED_BG << 4) | COLOR_DARK_BG;

        screen_x = (SCREEN_WIDTH - 2 - strlen(tmp) - strlen(tmp2)) / 2;
        screen_y = 1;
        screen_print(tmp);

        screen_x += 2;
        screen_print(tmp2);
    }

    screen_attr = save;
}

static void
choice_dialog_render(int x, int y, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *), void *get_label_func_user_data,
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
        const char *line = get_label_func(i, get_label_func_user_data);
        if (!line) {
            line = "";
        }
        screen_print(line);
        for (int j=width-1-strlen(line)-1; j>=0; --j) {
            screen_putch(' ');
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
        for (int j=0; j<16; ++j) {
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

static void
present()
{
    update_palette();

    short __far *screen = (short __far *)(0xb8000000L);
    _fmemcpy(screen, SCREEN_BUFFER, sizeof(SCREEN_BUFFER));
}

static int
choice_dialog_measure(int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *), void *get_label_func_user_data,
        const char *frame_label)
{
    int width = 20;
    int x_padding = 3;

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
            int len = strlen(line) + x_padding;
            if (width < len) {
                width = len;
            }

            line = get_text_func(k++, get_text_func_user_data);
        }
    }

    for (int i=0; i<n; ++i) {
        const char *line = get_label_func(i, get_label_func_user_data);
        if (!line) {
            continue;
        }
        int len = strlen(line) + x_padding;
        if (width < len) {
            width = len;
        }
    }

    return width + 1;
}


static void
generate_random_palette()
{
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

static int
choice_dialog_handle_input(struct ChoiceDialogState *state, int n)
{
    int page_size = 5;

    if (!kbhit()) {
        present();
        return 0;
    }

    int ch = getch();
    if (ch == 0) {
        ch = 0x100 | getch();
    }

    if (ch == KEY_NPAGE) {
        if (state->cursor >= n - page_size) {
            state->cursor = n - 1;
        } else {
            state->cursor += page_size;
        }
    } else if (ch == KEY_PPAGE) {
        if (state->cursor < page_size) {
            state->cursor = 0;
        } else {
            state->cursor -= page_size;
        }
    } else if (ch == KEY_HOME) {
        state->cursor = 0;
    } else if (ch == KEY_END) {
        state->cursor = n - 1;
    } else if (ch == KEY_DOWN) {
        if (state->cursor < n - 1) {
            state->cursor += 1;
        }
    } else if (ch == KEY_UP) {
        if (state->cursor > 0) {
            state->cursor -= 1;
        }
    } else if (ch == ENTERKEY) {
        return 1;
    } else if (ch == KEY_F2) {
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
    } else if (ch == KEY_F3 && display_adapter_type == DISPLAY_ADAPTER_VGA) {
        if (state->game != -1) {
            // on a game page
            show_screenshots(state->cat, state->game);
        } else if (state->here && state->here->num_children && state->cursor > 0) {
            // in a choice screen with a games list
            show_screenshots(state->cat, state->here->children[state->cursor-1]);
        }
    } else if (ch == 27) {
        state->cursor = 0;
        return 1;
    }

    return 0;
}

static int
choice_dialog(int x, int y, const char *title, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *), void *get_label_func_user_data,
        void (*render_background_func)(void *), void *render_background_func_user_data,
        const char *frame_label)
{
    int width = choice_dialog_measure(n,
            get_text_func, get_text_func_user_data,
            get_label_func, get_label_func_user_data,
            frame_label);

    if (x == -1) {
        x = (SCREEN_WIDTH - width) / 2;
    }

    while (1) {
        render_background(title);

        if (render_background_func) {
            render_background_func(render_background_func_user_data);
        }

        choice_dialog_render(x, y, state, n,
                get_text_func, get_text_func_user_data,
                get_label_func, get_label_func_user_data,
                frame_label, width, 1);

        present();

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

const char *
get_label_group(int i, void *user_data)
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
            return ud->cat->names->d[ud->group->children[i]];
        }

        if (i < ud->group->num_subgroups) {
            struct GameCatalogGroup *subgroup = ud->group->subgroups[i];
            sprintf(tmp_buf, "%s (%d)",
                    ud->cat->strings->d[subgroup->title_idx],
                    subgroup->num_children + subgroup->num_subgroups);
            return tmp_buf;
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

    if (i == 0) {
        sprintf(fmt_buf, "Jam: %s, Genre: %s",
                ud->cat->strings->d[game->jam_idx],
                ud->cat->strings->d[game->genre_idx]);
        return fmt_buf;
    }
    --i;

    if (i == 0) {
        sprintf(fmt_buf, "%s, %s",
                (game->flags & FLAG_IS_32_BITS) ? "386+" : "8088+",
                (game->flags & FLAG_IS_MULTIPLAYER) ? "multiplayer" : "single-player",
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

    return NULL;
}

const char *
get_label_game(int i, void *user_data)
{
    if (i == 0) {
        return "(..)";
    } else if (i == 1) {
        return "Launch Game";
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
                frame_label);
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
    brightness_up_down = -1;
    while (brightness) {
        present();
    }
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
vga_present_pcx(const char *filename)
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
            uint8_t __far *VGA = (uint8_t __far *)(0xa0000000L);

            static uint8_t chunk[512];
            int len = 0;
            int pos = 0;

            for (int i=0; i<256; ++i) {
                vga_set_palette_entry_direct(i, 0, 0, 0);
            }

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
                        VGA[pixels_processed++] = ch;
                    }
                } else {
                    VGA[pixels_processed++] = ch;
                }
            }

            fseek(fp, -(768 + 1), SEEK_END);
            int ch = fgetc(fp);
            if (ch == 12) {
                for (int i=0; i<256; ++i) {
                    int r = fgetc(fp);
                    int g = fgetc(fp);
                    int b = fgetc(fp);
                    vga_set_palette_entry_direct(i, r, g, b);
                }
            } else {
                // TODO: Error message (no palette)
            }
        } else {
            printf("Error reading file\n");
        }

        fclose(fp);
    } else {
        printf("Cannot open %s\n", filename);
        printf("Press ESC to go back to menu.\n");
    }
}

static void
show_screenshots(struct GameCatalog *cat, int game)
{
    if (cat->games[game].num_screenshots == 0) {
        return;
    }

    fade_out();

    int last_idx = -1;
    int current_idx = 0;

    // switch to mode 0x13
    {
        union REGS inregs, outregs;
        inregs.h.ah = 0;
        inregs.h.al = 0x13;

        int86(0x10, &inregs, &outregs);
    }

    while (1) {
        char filename[128];
        sprintf(filename, "pcx/%s/shot%d.pcx", cat->ids->d[game], current_idx);

        if (current_idx != last_idx) {
            vga_present_pcx(filename);
            last_idx = current_idx;
        }

        int ch = getch();
        if (ch == 0) {
            ch = 0x100 | getch();
        }

        if (ch == KEY_DOWN) {
            if (current_idx < cat->games[game].num_screenshots - 1) {
                ++current_idx;
            }
        } else if (ch == KEY_UP) {
            if (current_idx > 0) {
                --current_idx;
            }
        } else if (ch == 27 || ch == KEY_F3) {
            // F3 toggles screenshots on and off
            // ESC to get out of screenshot mode
            break;
        }
    }

    // .. and back to the menu
    textmode_reset();
    configure_text_mode();
}

int main(int argc, char *argv[])
{
    // Empty keyboard buffer
    while (kbhit()) {
        getch();
    }

    srand(time(NULL));

    display_adapter_type = detect_display_adapter();

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

    if (argc == 2) {
        static struct {
            int their_ds;
            int their_offset;
        } dos_ipc = { 0, 0 };

        if (sscanf(argv[1], "%x:%x", &dos_ipc.their_ds, &dos_ipc.their_offset) != 2) {
            dos_ipc.their_ds = 0;
            dos_ipc.their_offset = 0;
        }

        ipc_buffer = (struct IPCBuffer __far *)(((uint32_t)dos_ipc.their_ds << 16) |
                                                ((uint32_t)dos_ipc.their_offset));

        if (ipc_buffer->color_palette_len == 0) {
            generate_random_palette();
            store_color_palette();
        } else {
            for (int i=0; i<ipc_buffer->color_palette_len; ++i) {
                DEFAULT_PALETTE[i].r = ipc_buffer->color_palette[i][0];
                DEFAULT_PALETTE[i].g = ipc_buffer->color_palette[i][1];
                DEFAULT_PALETTE[i].b = ipc_buffer->color_palette[i][2];
            }
        }
    } else {
        generate_random_palette();
    }

    configure_text_mode();

    FILE *fp = fopen("gamectlg.dat", "rb");
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(len);
    if (fread(buf, len, 1, fp) != 1) {
        printf("failed to read\n");
        exit(1);
    }
    fclose(fp);

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
            cds.cat = cat;
            cds.here = here;
            cds.game = game;
            cds.cursor = 0;
            cds.offset = 0;

            int selection = choice_dialog(-1, 5, buf, &cds, 2,
                    get_text_game, &gtgud,
                    get_label_game, NULL,
                    render_group_background, &brud,
                    cat->names->d[game]);

            if (selection == 0) {
                ipc_buffer_pop_menu_trail_entry();
                game = -1;
            } else {
                selection -= 1;

                ipc_buffer_add_menu_trail_entry(selection, here);

                if (selection == 0) {
                    // pop menu selection again
                    ipc_buffer_pop_menu_trail_entry();

                    if (ipc_buffer) {
                        ipc_buffer->request = IPC_RUN_GAME;
                        ipc_buffer->game_flags = cat->games[game].flags;

                        _fstrcpy(ipc_buffer->cmdline, "demo2023/");
                        _fstrcat(ipc_buffer->cmdline, cat->strings->d[cat->games[game].run_idx]);

                        fade_out();
                        return 0;
                    }
                }
            }
        } else {
            struct GetLabelGroupUserData glgud;
            glgud.cat = cat;
            glgud.group = here;
            int max = (here->num_children ? here->num_children : here->num_subgroups);
            struct ChoiceDialogState cds;
            cds.cat = cat;
            cds.here = here;
            cds.game = game;
            cds.cursor = here->cursor_index;
            cds.offset = here->scroll_offset;

            int selection = choice_dialog(2, 3, buf, &cds, max + 1,
                    NULL, NULL,
                    get_label_group, &glgud,
                    NULL, NULL,
                    cat->strings->d[here->title_idx]);
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

    textmode_reset();

    return 0;
}
