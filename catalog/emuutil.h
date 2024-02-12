#pragma once

#include <string.h>

enum DOSEmulator {
    EMULATOR_NONE = 0,
    EMULATOR_DOSBOX = 1,
    EMULATOR_DOSBOX_X = 2,
};

static const char *
EMULATOR_TYPES[] = {
    NULL,
    "DOSBox",
    "DOSBox-X",
};

static enum DOSEmulator
detect_dos_emulator(void)
{
    // Motherboard BIOS goes from 0x000F0000 - 0x000FFFFF
    // DOSBox and DOSBox-X place some identification strings
    // at address 0x000FE000, the string we use begins at
    // 0x000FE060 + 1 byte, which is FE06:0001 (seg:off)
    // See also: https://wiki.osdev.org/Memory_Map_(x86)
    char far *dosbox_bios_string = (char far *)(unsigned long)MK_FP(0xfe06ul, 0x0001ul);

    if (_fstrcmp(dosbox_bios_string, "DOSBox-X BIOS v1.0") == 0) {
        return EMULATOR_DOSBOX_X;
    } else if (_fstrcmp(dosbox_bios_string, "DOSBox FakeBIOS v1.0") == 0) {
        return EMULATOR_DOSBOX;
    }

    return EMULATOR_NONE;
}
