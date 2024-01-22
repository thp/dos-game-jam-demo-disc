#pragma once

enum DisplayAdapter {
    DISPLAY_ADAPTER_CGA = 0,
    DISPLAY_ADAPTER_EGA = 1,
    DISPLAY_ADAPTER_VGA = 2,
};

static const char *
DISPLAY_ADAPTER_NAMES[] = {
    "CGA",
    "EGA",
    "VGA",
};

static enum DisplayAdapter
detect_display_adapter()
{
    // Detect CGA/EGA/VGA/VESA
    // Adapted from code by Luke McCarthy (xordust.com)
    // https://gist.github.com/ljmccarthy/3d842add94eebef4ad310a7e9fbe9fb8
    union REGS regs;
    memset(&regs, 0, sizeof(regs));
    // https://stanislavs.org/helppc/int_10-12.html
    regs.h.ah = 0x12; // INT 10,12 - Video Subsystem Configuration (EGA/VGA)
    regs.h.bl = 0x10; // BL = 10  return video configuration information

    int86(0x10, &regs, &regs);

    /*
        on return:
        BH = 0 if color mode in effect
           = 1 if mono mode in effect
        BL = 0 if 64k EGA memory
           = 1 if 128k EGA memory
           = 2 if 192k EGA memory
           = 3 if 256k EGA memory
        CH = feature bits
        CL = switch settings
    */

    if (regs.h.bl == 0x10) {
        return DISPLAY_ADAPTER_CGA;
    }

    // https://stanislavs.org/helppc/int_10-1a.html
    memset(&regs, 0, sizeof regs);
    regs.x.ax = 0x1a00; // INT 10,1A - Video Display Combination (VGA)
                        // AL = 00 get video display combination
    int86(0x10, &regs, &regs);
    if (regs.h.al != 0x1a) {
        return DISPLAY_ADAPTER_EGA;
    }

    return DISPLAY_ADAPTER_VGA;
}

// 2-bit per channel (RGB) palette -> 6 bits
static void
ega_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    union REGS regs;
    memset(&regs, 0, sizeof(regs));

    uint8_t color = ((r & 0x80) ? (1 << 2) : 0) |
                    ((r & 0x40) ? (1 << 5) : 0) |
                    ((g & 0x80) ? (1 << 1) : 0) |
                    ((g & 0x40) ? (1 << 4) : 0) |
                    ((b & 0x80) ? (1 << 0) : 0) |
                    ((b & 0x40) ? (1 << 3) : 0);

    regs.h.ah = 0x10; // EGA set palette registers
    regs.h.al = 0; // subservice 0 = set palette register
    regs.h.bl = index;
    regs.h.bh = color;

    int86(0x10, &regs, &regs);
}

// 6-bit per channel (RGB) palette -> 18 bits
static void
vga_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    static const int
    EGA_TO_VGA_INDEX[] = {
        0x00, // Black
        0x01, // Blue
        0x02, // Green
        0x03, // Cyan
        0x04, // Red
        0x05, // Magenta
        0x14, // Brown
        0x07, // Light Gray
        0x38, // Gray
        0x39, // Light Blue
        0x3A, // Light Green
        0x3B, // Light Cyan
        0x3C, // Light Red
        0x3D, // Light Magenta
        0x3E, // Light Yellow
        0x3F, // White
    };

    outp(0x3c8, EGA_TO_VGA_INDEX[index]);
    outp(0x3c9, r >> 2);
    outp(0x3c9, g >> 2);
    outp(0x3c9, b >> 2);
}

static void
vga_set_palette_entry_direct(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    outp(0x3c8, index);
    outp(0x3c9, r >> 2);
    outp(0x3c9, g >> 2);
    outp(0x3c9, b >> 2);
}

static void
textmode_reset()
{
    union REGS inregs, outregs;
    inregs.h.ah = 0;
    inregs.h.al = 3;

    int86(0x10, &inregs, &outregs);
}

static void
enable_blinking_cursor(int x, int y)
{
    union REGS inregs, outregs;
    inregs.h.ah = 1;
    inregs.h.ch = 0;
    inregs.h.cl = 15;

    int86(0x10, &inregs, &outregs);

    inregs.h.ah = 2;
    inregs.h.bh = 0;
    inregs.h.dh = y;
    inregs.h.dl = x;
    int86(0x10, &inregs, &outregs);
}

static void
disable_blinking_cursor()
{
    union REGS inregs, outregs;
    inregs.h.ah = 1;
    inregs.h.ch = 0x3f;

    int86(0x10, &inregs, &outregs);
}

static void
enable_4bit_background()
{
    switch (detect_display_adapter()) {
        case DISPLAY_ADAPTER_CGA:
            {
                // http://www.techhelpmanual.com/901-color_graphics_adapter_i_o_ports.html
                // Mirrored by BIOS at 0040:0065, which is also updated first
                uint8_t far *mode_select_register = (uint8_t far *)0x00400065ul;
                *mode_select_register &= ~(1<<5);
                outp(0x3d8, *mode_select_register);
            }
            break;
        case DISPLAY_ADAPTER_EGA:
        case DISPLAY_ADAPTER_VGA:
            {
                union REGS regs;
                memset(&regs, 0, sizeof(regs));

                regs.h.ah = 0x10;
                regs.h.al = 0x03;
                regs.h.bl = 0x00;
                regs.h.bh = 0x00;

                int86(0x10, &regs, &regs);
            }
            break;
    }
}
