#pragma once

#include <dos.h>

_Packed struct MCB {
  char bSignature;
  unsigned short wOwnerID;
  unsigned short wSizeParas;
  char reserved[3];
  char szOwnerName[8];
};

unsigned long
get_free_conventional_memory_bytes()
{
    unsigned long largest = 0;

    union REGS regs;
    struct SREGS sregs;

    regs.h.ah = 0x52;

    intdosx(&regs, &regs, &sregs);

    unsigned short far *vp = MK_FP(sregs.es, regs.x.bx - 2);
    unsigned short v = *vp;

    struct MCB far *mcb = MK_FP(v, 0);

    while (1) {
        if (mcb->wOwnerID == 0) {
            unsigned long here_size = 16ul * (unsigned long)mcb->wSizeParas;
            if (here_size > largest) {
                largest = here_size;
            }
        }

        if (mcb->bSignature == 'Z') {
            break;
        }

        mcb += 1 + mcb->wSizeParas;
    }

    return largest;
}
