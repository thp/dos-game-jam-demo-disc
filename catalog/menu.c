#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__DJGPP__)
#include <sys/farptr.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>

#include <conio.h>
#define KEY_DOWN (0x100 | 80)
#define KEY_UP (0x100 | 72)
#define KEY_NPAGE (0x100 | 81)
#define KEY_PPAGE (0x100 | 73)
#define KEY_HOME (0x100 | 71)
#define KEY_END (0x100 | 79)
#define ENTERKEY (13)
#define printw(...) cprintf(__VA_ARGS__); cprintf("\r")
#else
#include <ncurses.h>
#define ENTERKEY ('\n')
#endif

#include "gamectlg.h"

void
print_parents_first(char *buf, struct GameCatalog *cat, struct GameCatalogGroup *group)
{
    if (group->parent_group) {
        print_parents_first(buf, cat, group->parent_group);
        strcat(buf, " / ");
    }

    strcat(buf, cat->strings->d[group->title_idx]);
}

struct ChoiceDialogState {
    int cursor;
    int offset;
};

static int
choice_dialog(const char *title, struct ChoiceDialogState *state, int n,
        const char *(*get_text_func)(int, void *), void *get_text_func_user_data,
        const char *(*get_label_func)(int, void *), void *get_label_func_user_data)
{
    int max_rows = 20;
    int page_size = 5;

    while (1) {
#if defined(__DJGPP__)
        clrscr();
#else
        clear();
        move(0, 0);
#endif
        printw("== %s ==\n", title);

        if (get_text_func != NULL) {
            int k = 0;
            const char *line = get_text_func(k++, get_text_func_user_data);
            while (line != NULL) {
                printw(" %s\n", line);
                line = get_text_func(k++, get_text_func_user_data);
            }
        }

        if (state->cursor < state->offset) {
            state->offset = state->cursor;
        } else if (state->cursor > state->offset + max_rows - 1) {
            state->offset = state->cursor - max_rows + 1;
        }

        int begin = state->offset;
        int end = state->offset + max_rows;
        if (end > n) {
            end = n;
        }

        if (begin > 0) {
            printw("... (more) ...\n");
        } else {
            printw("\n");
        }

        for (int i=begin; i<end; ++i) {
            if (i == state->cursor) {
#if defined(__DJGPP__)
                textcolor(BLACK);
                textbackground(WHITE);
#else
                attron(A_REVERSE);
#endif
            }
            printw("   %s   \n", get_label_func(i, get_label_func_user_data) ?: "---");
            if (i == state->cursor) {
#if defined(__DJGPP__)
                textcolor(LIGHTGRAY);
                textbackground(BLACK);
#else
                attroff(A_REVERSE);
#endif
            }
        }

        if (end < n) {
            printw("... (more) ...\n");
        } else {
            printw("\n");
        }

#if !defined(__DJGPP__)
        refresh();
#endif

        int ch = getch();
#if defined(__DJGPP__)
        if (ch == 0) {
            ch = 0x100 | getch();
        }
#endif
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
            return state->cursor;
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
            return " ^UP";
        } else {
            return " -QUIT";
        }
    } else {
        if (i <= ud->group->num_children) {
            return ud->cat->names->d[ud->group->children[i-1]];
        }

        if (i <= ud->group->num_subgroups) {
            struct GameCatalogGroup *subgroup = ud->group->subgroups[i-1];
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
    struct GetTextGameUserData *ud = user_data;

    if (i == 0) {
        return ud->cat->descriptions->d[ud->game];
    } else if (i == 1) {
        return ud->cat->urls->d[ud->game];
    } else if (i == 2) {
        return ud->cat->strings->d[ud->cat->games[ud->game].jam_idx];
    } else if (i == 3) {
        return ud->cat->strings->d[ud->cat->games[ud->game].genre_idx];
    }

    return NULL;
}

const char *
get_label_game(int i, void *user_data)
{
    if (i == 0) {
        return " ^BACK";
    } else if (i == 1) {
        return "Launch Game";
    }

    return NULL;
}

static struct {
    int their_ds;
    int their_offset;
} dos_ipc = { 0, 0 };

static void
dos_ipc_append(const char *string)
{
#if defined(__DJGPP__)
    if (dos_ipc.their_ds != 0) {
        while (*string) {
            _farpokeb(_dos_ds, dos_ipc.their_ds * 16 + dos_ipc.their_offset, *string);
            ++dos_ipc.their_offset;
            ++string;
        }
        _farpokeb(_dos_ds, dos_ipc.their_ds * 16 + dos_ipc.their_offset, '\0');
    }
#else
    // TODO
#endif
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        if (sscanf(argv[1], "%x:%x", &dos_ipc.their_ds, &dos_ipc.their_offset) != 2) {
            dos_ipc.their_ds = 0;
            dos_ipc.their_offset = 0;
        }
    }

#if !defined(__DJGPP__)
    initscr();
    keypad(stdscr, TRUE);
    curs_set(0);
#else
    _setcursortype(_NOCURSOR);
#endif

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
    while (1) {
        if (game != -1) {
            struct GetTextGameUserData gtgud = { cat, game };

            struct ChoiceDialogState cds = { 0, 0 };
            int selection = choice_dialog(cat->names->d[game], &cds, 2, get_text_game, &gtgud, get_label_game, NULL);

            if (selection == 0) {
                game = -1;
            } else if (selection == 1) {
#if defined(__DJGPP__)
                if ((cat->games[game].flags & FLAG_HAS_END_SCREEN) != 0) {
                    dos_ipc_append("@");
                } else {
                    dos_ipc_append(" ");
                }
                dos_ipc_append("DEMO2023/");
                dos_ipc_append(cat->strings->d[cat->games[game].run_idx]);

                return 0;
#endif
            }
        } else {
            char buf[512];
            strcpy(buf, "");
            print_parents_first(buf, cat, here);

            struct GetLabelGroupUserData glgud = { cat, here };
            int max = (here->num_children ? here->num_children : here->num_subgroups);
            struct ChoiceDialogState cds = { here->cursor_index, here->scroll_offset };
            int selection = choice_dialog(buf, &cds, max + 1, NULL, NULL, get_label_group, &glgud);
            here->cursor_index = cds.cursor;
            here->scroll_offset = cds.offset;

            if (selection == 0) {
                if (here->parent_group != NULL) {
                    here = here->parent_group;
                } else {
                    break;
                }
            } else {
                if (here->num_subgroups) {
                    if (selection > 0 && selection <= here->num_subgroups) {
                        here = here->subgroups[selection-1];
                    }
                } else {
                    if (selection > 0 && selection <= here->num_children) {
                        game = here->children[selection-1];
                    }
                }
            }
        }
    }

    game_catalog_free(cat);

#if !defined(__DJGPP__)
    endwin();
#else
    textmode(C80);
#endif

#if defined(__DJGPP__)
    dos_ipc_append("exit");
#endif

    return 0;
}
