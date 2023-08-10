#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos.h>

#include <conio.h>
#define KEY_DOWN (0x100 | 80)
#define KEY_UP (0x100 | 72)
#define KEY_NPAGE (0x100 | 81)
#define KEY_PPAGE (0x100 | 73)
#define KEY_HOME (0x100 | 71)
#define KEY_END (0x100 | 79)
#define ENTERKEY (13)

#include "gamectlg.h"
#include "ipc.h"
#include "vgautil.h"

#define SCREEN_WIDTH (80)
#define SCREEN_HEIGHT (25)

static uint8_t
SCREEN_BUFFER[80*25][2];

static uint8_t
screen_x = 0;

static uint8_t
screen_y = 0;

static uint8_t
screen_attr = 7;

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
    screen_attr = (3 << 4) | 0;

    for (int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; ++i) {
        SCREEN_BUFFER[i][0] = 0xb2;
        SCREEN_BUFFER[i][1] = (1 << 4) | 7;
    }

    for (int i=0; i<SCREEN_WIDTH; ++i) {
        SCREEN_BUFFER[i][0] = ' ';
        SCREEN_BUFFER[i][1] = (1 << 4) | 0;
    }

    if (*title) {
        screen_y = 0;
        screen_x = (SCREEN_WIDTH - strlen(title) - 2) / 2;

        int save = screen_attr;
        screen_attr = (0 << 4) | 0xf;
        screen_putch(' ');
        screen_print(title);
        screen_putch(' ');
        screen_attr = save;
    }
}

static void
choice_dialog_render(int x, int y, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *), void *get_label_func_user_data,
        const char *frame_label, int width, int color)
{
    int max_rows = 18;

    screen_x = x;
    screen_y = y; y += 1;

    int bg = 3;
    int fg = 0;
    int bg_cursor = 1;
    int fg_cursor = 0xf;
    int bg_more = 3;
    int fg_more = 8;

    if (!color) {
        bg = 7;
        fg = 0;
        bg_cursor = 0;
        fg_cursor = 7;
        bg_more = 7;
        fg_more = 8;
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
        screen_attr = (0 << 4) | 0xf;
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
                screen_attr &= (0xf << 4);
                screen_attr |= 0xb;
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

    if (state->cursor <= state->offset) {
        state->offset = state->cursor;
        if (state->cursor > 0) {
            state->offset--;
        }
    } else if (state->cursor >= state->offset + max_rows - 1) {
        state->offset = state->cursor - max_rows + 1;
        if (state->offset < n - max_rows) {
            state->offset++;
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
        for (int j=width-1-strlen(line); j>=0; --j) {
            screen_putch(' ');
        }
        if (i == state->cursor) {
            screen_attr = save;
        }
        right_border(fg, bg);
    }

    int post_y = screen_y;

    if (begin > 0) {
        int save_y = screen_y;
        const char *pre = "(^^)";
        screen_x = x + width - strlen(pre);
        screen_y = pre_y - 1;
        int save = screen_attr;
        screen_attr = (bg_more << 4) | fg_more;
        screen_print(pre);
        screen_attr = save;
        screen_x = x;
        screen_y = save_y;
    }

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

    if (end < n) {
        const char *post = "(vv)";
        screen_x = x + width - strlen(post);
        int save = screen_attr;
        screen_attr = (bg_more << 4) | fg_more;
        screen_print(post);
        screen_x = x;
    }

    screen_x = x + 1;
    screen_y = y; y += 1;
    right_shadow(width + 3);
}

static void
present()
{
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

    return width;
}

static int
choice_dialog_handle_input(struct ChoiceDialogState *state, int n)
{
    int page_size = 5;

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
    cds.cursor = ud->here->cursor_index;
    cds.offset = ud->here->scroll_offset;

    const char *frame_label = ud->cat->strings->d[ud->here->title_idx];

    if (ud->width == 0) {
        ud->width = choice_dialog_measure(max + 1,
                NULL, NULL,
                get_label_group, &glgud,
                frame_label);
    }

    choice_dialog_render(2, 2, &cds, max + 1,
            NULL, NULL,
            get_label_group, &glgud,
            frame_label, ud->width, 0);
}

static struct IPCBuffer __far *
ipc_buffer = NULL;

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

int main(int argc, char *argv[])
{
    // Empty keyboard buffer
    while (kbhit()) {
        getch();
    }

    disable_blinking_cursor();

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
    }

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

            struct ChoiceDialogState cds = { 0, 0 };
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
            cds.cursor = here->cursor_index;
            cds.offset = here->scroll_offset;
            int selection = choice_dialog(2, 2, buf, &cds, max + 1,
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

    textmode_reset();

    return 0;
}
