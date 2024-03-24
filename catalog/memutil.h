#pragma once

#include <dos.h>
#include <i86.h>

unsigned long
get_free_conventional_memory_bytes()
{
    union REGS regs;
    struct SREGS sregs;
    segread(&segs);

    memset(&regs, 0, sizeof(regs));
    regs.h.ah = 0x48;
    regs.x.bx = 0xffff;

    intdosx(&regs, &regs, &sregs);

    return 16ul * (unsigned long)regs.x.bx;
}
