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
};

struct IPCBuffer {
    uint8_t request;
    uint8_t game_flags;

    char cmdline[128];

    struct MenuTrailEntry menu_trail[32];
    uint8_t menu_trail_len;
};