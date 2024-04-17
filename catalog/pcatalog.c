
/*
 * DOS Game Jam Demo Disc Menu
 * Copyright (c) 2023, 2024 Thomas Perl <m@thp.io>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "gamectlg.h"

// Example code how to use gamectlg's structures

void do_indent(int indent) { for (int i=0; i<indent; ++i) { putchar(' '); } }

void
print_groups_recursive(int indent, struct GameCatalog *cat, struct GameCatalogGroup *grouping)
{
    do_indent(indent); printf("Group: '%s' (%p, parent=%p)\n", cat->strings->d[grouping->title_idx], grouping, grouping->parent_group);
    for (int i=0; i<grouping->num_children; ++i) {
        uint8_t game_idx = grouping->children[i];
        do_indent(indent + 2); printf("child %d: '%s'\n", i, cat->names->d[game_idx]);
    }

    for (int i=0; i<grouping->num_subgroups; ++i) {
        print_groups_recursive(indent + 4, cat, grouping->subgroups[i]);
    }
}

void game_catalog_print(struct GameCatalog *cat)
{
    printf("Number of games: %d\n", cat->n_games);

    for (int i=0; i<cat->n_games; ++i) {
        printf("Game %3d: '%s', run='%s', kilobytes = %d\n",
                i,
                cat->names->d[i],
                cat->strings->d[cat->games[i].run_idx],
                cat->games[i].kilobytes);
        printf("        D: '%s'\n", cat->descriptions->d[i]);
        printf("        U: '%s'\n", cat->urls->d[i]);
        printf("        I: '%s'\n", cat->ids->d[i]);
        printf("        R: '%s'\n", cat->readmes->d[i]);
        printf("        loader='%s', jam='%s', genre='%s', exit='%s', type='%s', shots=%d\n",
                cat->strings->d[cat->games[i].loader_idx],
                cat->strings->d[cat->games[i].jam_idx],
                cat->strings->d[cat->games[i].genre_idx],
                cat->strings->d[cat->games[i].exit_key_idx],
                cat->strings->d[cat->games[i].type_idx],
                cat->games[i].num_screenshots);

        printf("        F: ");
        if ((cat->games[i].flags & FLAG_HAS_END_SCREEN) != 0) {
            printf("end-screen ");
        }
        if ((cat->games[i].flags & FLAG_IS_32_BITS) != 0) {
            printf("32-bit ");
        } else {
            printf("16-bit ");
        }
        if ((cat->games[i].flags & FLAG_IS_MULTIPLAYER) != 0) {
            printf("multiplayer ");
        }
        if ((cat->games[i].flags & FLAG_IS_OPEN_SOURCE) != 0) {
            printf("open-source ");
        }
        if ((cat->games[i].flags & FLAG_MOUSE_SUPPORTED) != 0) {
            printf("mouse ");
        }
        if ((cat->games[i].flags & FLAG_MOUSE_REQUIRED) != 0) {
            printf("mouse-required ");
        }
        if ((cat->games[i].flags & FLAG_KEYBOARD_SUPPORTED) != 0) {
            printf("keyboard ");
        }
        if ((cat->games[i].flags & FLAG_JOYSTICK_SUPPORTED) != 0) {
            printf("joystick ");
        }
        printf("\n");

        struct GameCatalogStringList *authors_idx = cat->string_lists->d + cat->games[i].author_list_idx;
        for (int j=0; j<authors_idx->n; ++j) {
            printf("\tAuthor %d: '%s'\n", j, cat->strings->d[authors_idx->d[j]]);
        }

        struct GameCatalogStringList *video_idx = cat->string_lists->d + cat->games[i].video_list_idx;
        for (int j=0; j<video_idx->n; ++j) {
            printf("\tVideo %d: '%s'\n", j, cat->strings->d[video_idx->d[j]]);
        }

        struct GameCatalogStringList *sound_idx = cat->string_lists->d + cat->games[i].sound_list_idx;
        for (int j=0; j<sound_idx->n; ++j) {
            printf("\tSound %d: '%s'\n", j, cat->strings->d[sound_idx->d[j]]);
        }

        struct GameCatalogStringList *toolchain_idx = cat->string_lists->d + cat->games[i].toolchain_list_idx;
        for (int j=0; j<toolchain_idx->n; ++j) {
            printf("\tToolchain %d: '%s'\n", j, cat->strings->d[toolchain_idx->d[j]]);
        }
    }

    print_groups_recursive(0, cat, cat->grouping);
}

int main(int argc, char *argv[])
{
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

    game_catalog_print(cat);

    game_catalog_free(cat);

    return 0;
}
