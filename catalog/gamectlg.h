#pragma once

/**
 * DOS Game Jam Demo Disc Game Catalog Format
 * 2023-08-09 Thomas Perl <m@thp.io>
 **/

#include <stdint.h>

enum GameCatalogFlags {
    FLAG_HAS_END_SCREEN = (1 << 0), // game shows an ANSI art end screen, pause before returning to menu
    FLAG_IS_32_BITS = (1 << 1), // game is 32-bit (if this flag is not set, the game is 16-bit)
    FLAG_IS_MULTIPLAYER = (1 << 2), // game supports local multiplayer (2-player or more)
    FLAG_IS_OPEN_SOURCE = (1 << 3), // game has source code available
    FLAG_MOUSE_SUPPORTED = (1 << 4), // game can use the mouse as input device
    FLAG_MOUSE_REQUIRED = (1 << 5), // a mouse is strictly required to play the game
    FLAG_KEYBOARD_SUPPORTED = (1 << 6), // game can use the keyboard as input device
    FLAG_JOYSTICK_SUPPORTED = (1 << 7), // game can use the joystick as input device
};

struct GameCatalogGame {
    uint32_t kilobytes; /* size, number of kilobytes */
    uint8_t flags; /* enum GameCatalogFlags */

    /* the following indices are into cat->strings */
    uint8_t run_idx; /* the "run" command line (path + exe + args) */
    uint8_t loader_idx; /* path to loader program that needs to be executed first, or empty string (index 0) */
    uint8_t jam_idx; /* name of the game jam this game was (first) submitted to */
    uint8_t genre_idx; /* genre of the game */
    uint8_t exit_key_idx; /* text description what key to press / how to exit the game */
    uint8_t type_idx; /* type of game release (e.g. full version, shareware, demo, etc... */

    /* the following indices are into cat->string_lists */
    uint8_t author_list_idx; /* list of game authors, artists, collaborators, etc.. */
    uint8_t video_list_idx; /* list of supported video modes (e.g. CGA, VGA, EGA, ...) */
    uint8_t sound_list_idx; /* list of supported sound cards (e.g. PC Speaker, Adlib, ...) */
    uint8_t toolchain_list_idx; /* list of used toolchains/sdks/libraries (e.g. DJGPP, ...) */

    uint8_t _reserved; // UNUSED
};

struct GameCatalogGroup {
    uint8_t title_idx; /* string index */
    uint8_t num_children; /* how many direct children (=games) are in this group */
    uint8_t num_subgroups; /* how many sub-groups this group has */
    const uint8_t *children; /* <num_children> game indices, for indexing into cat->games */
    struct GameCatalogGroup *parent_group; /* parent group, or NULL if this is the root group */
    struct GameCatalogGroup *subgroups[]; /* <num_subgroups> pointers to subgroups */
};

/* string table */
struct GameCatalogStrings {
    uint8_t n; // how many strings are in this table
    const char *d[]; // array of pointers to zero-terminated strings
};

/* list of string indices */
struct GameCatalogStringList {
    uint8_t n; // how many string indices are in this table
    const uint8_t *d; // pointing to a list of string list indices
};

/* table of string lists */
struct GameCatalogStringListTable {
    uint8_t n; // how many lists exist
    struct GameCatalogStringList d[]; // array of string lists
};

struct GameCatalog {
    /* internal file buffer */
    char *buf;
    const char *buf_ptr;
    int buf_len;

    /* public */
    uint8_t n_games;
    const struct GameCatalogGame *games;

    /* per-game strings (indexed like games above) */
    struct GameCatalogStrings *names;
    struct GameCatalogStrings *descriptions;
    struct GameCatalogStrings *urls;

    /* string bucket (for _idx) */
    struct GameCatalogStrings *strings;

    /* list of strings (for _list_idx) */
    struct GameCatalogStringListTable *string_lists;

    /* root of group hierarchy */
    struct GameCatalogGroup *grouping;
};

struct GameCatalog *
game_catalog_parse(char *buf, int len);

void
game_catalog_free(struct GameCatalog *cat);
