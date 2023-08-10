#pragma once

static void
textmode_reset()
{
    union REGS inregs, outregs;
    inregs.h.ah = 0;
    inregs.h.al = 3;

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
