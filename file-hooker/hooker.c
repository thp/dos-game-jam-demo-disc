#include <stdio.h>
#include <dos.h>
#include <stdint.h>
#include <conio.h>

void _interrupt _far (*old_int21_handler)(void);

void far *stack_root = 0;

#include "chain.h"

void dumpstack()
{
    uint32_t stv = 0xf00dface;

    uint8_t far *end = (uint8_t far *)stack_root;
    uint8_t far *ptr = (uint8_t far *)&stv;

    printf("dumping stack... %lx to %lx", (unsigned long)end, (unsigned long)ptr);

    printf("\n...........  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");

    if (((unsigned long)ptr % 16) != 0) {
        uint8_t far *start = ptr;
        ptr -= ((unsigned long)ptr % 16);
        printf("\n0x%08lx: ", (unsigned long)ptr);
        while (ptr < start) {
            printf(" ..");
            ++ptr;
        }
    }

    int i = 0;
    while (ptr < end) {
        if (((unsigned int)ptr) % 16 == 0) {
            printf("\n0x%08lx: ", (unsigned long)ptr);
        }

        printf(" %02x", (int)*ptr);

        ++i;
        ++ptr;
    }

    printf("\n");
}

void end_of_dumpstack() {}

void _interrupt
hook_int21(union INTPACK r)
{
    uint32_t marker = 0xcafebabe;

    if (r.h.ah == 0x3d || r.h.ah == 0x3c) {
        unsigned short orig_ds = r.w.ds;
        unsigned short orig_dx = r.w.dx;

        printf("address of r: 0x%08lx -> 0x%08lx\n", (unsigned long)(void far *)&r, (unsigned long)(void far *)(&r + 1));
        char far *str = MK_FP(r.w.ds, r.w.dx);
        printf("file open hook: '%s' marker @ 0x%08lx\n", str, (unsigned long)(void far *)&marker);
        dumpstack();

        // redirect filename
        const char far *other_filename = "chain.s";
        r.w.ds = FP_SEG(other_filename);
        r.w.dx = FP_OFF(other_filename);

        _chain_intr_dsdx(old_int21_handler, orig_ds, orig_dx);
    }

    _chain_intr(old_int21_handler);
}

void end_of_hook_int21() {}

void end_of_main();

int
main(int argc, char *argv[])
{
    uint32_t stv = 0xdeadbeef;

    static struct SREGS segs;

    printf("address of stackvar: 0x%08lx (value = 0x%08lx)\n", (unsigned long)(void far *)&stv, stv);
    printf("address of segs: 0x%08lx\n", (unsigned long)(void far *)&segs);
    printf("address of main: 0x%08lx -> 0x%08lx\n", (unsigned long)(void far *)&main, (unsigned long)(void far *)&end_of_main);
    printf("address of hook_int21: 0x%08lx -> 0x%08lx\n", (unsigned long)(void far *)&hook_int21, (unsigned long)(void far *)&end_of_hook_int21);
    printf("address of dumpstack: 0x%08lx -> 0x%08lx\n", (unsigned long)(void far *)&dumpstack, (unsigned long)(void far *)&end_of_dumpstack);

    stack_root = ((uint8_t far *)&stv) + sizeof(stv);

    segread(&segs);

    printf("ss = 0x%04x, ", segs.ss);
    printf("ds = 0x%04x, ", segs.ds);
    printf("cs = 0x%04x\n", segs.cs);

    //printf("calling dumpstack from main...\n");
    //dumpstack();

    old_int21_handler = _dos_getvect(0x21);
    _dos_setvect(0x21, hook_int21);

    const char *filename = "hooker.c";
    printf("address of filename ptr: 0x%08lx\n", (unsigned long)(void far *)&filename);
    printf("filename ptr value: 0x%08lx\n", (unsigned long)filename);

    printf("opening file %s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp) {
        char buf[16];
        int len = fread(buf, 1, 16, fp);
        buf[15] = '\0';
        printf("got %d bytes: %s\n", len, buf);
        fclose(fp);
    }

    printf("restoring handler...\n");
    _dos_setvect(0x21, old_int21_handler);

    return 0;
}

void
end_of_main() {}
