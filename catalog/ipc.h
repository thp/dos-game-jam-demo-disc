#pragma once

#include <stdint.h>

struct MenuTrailEntry {
    uint8_t selection;
    uint8_t cursor_index;
    uint8_t scroll_offset;
};

enum IPCRequest {
    IPC_EXIT = 0,
    IPC_RUN_GAME = 1,
    IPC_SWITCH_MENU_MODE = 2,
};

enum MenuMode {
    MENU_MODE_TEXT = 0,
    MENU_MODE_VGA = 1,
    MENU_MODE_VESA = 2,

    MENU_MODE_COUNT,
};

#define IPC_BUFFER_MAGIC (0xd05d2023)

struct IPCBuffer {
    uint32_t magic;
    char catalog_filename[64];

    uint32_t free_conventional_memory_bytes;

    uint8_t request;
    uint8_t game_flags;

    uint8_t color_palette[16][3];
    uint8_t color_palette_len;

    char cmdline[128];
    char title[64];

    struct MenuTrailEntry menu_trail[32];
    uint8_t menu_trail_len;

    uint8_t menu_mode;
};
