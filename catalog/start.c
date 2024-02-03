#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include "gamectlg.h"
#include "ipc.h"
#include "vgautil.h"
#include "mcbutil.h"

#include "chain.h"

static struct IPCBuffer
ipc_buffer;

int
run_command(char *cmd, char *args)
{
    char cmdTail[64];

    int len = strlen(args);

    if (len) {
        // cmdTail = " " + args + "\r\0"
        cmdTail[0] = len + 1;
        cmdTail[1] = ' ';
        memcpy(cmdTail + 2, args, len);
        cmdTail[2 + len] = '\r';
        cmdTail[2 + len + 1] = '\0';
    } else {
        // cmdTail = "\r\0"
        cmdTail[0] = 0;
        cmdTail[1] = '\r';
        cmdTail[2] = '\0';
    }

    struct SREGS segs;
    segread(&segs);

    char fcb1[37];
    memset(fcb1, 0, sizeof(fcb1));

    char fcb2[37];
    memset(fcb2, 0, sizeof(fcb2));

    struct LoadExec {
        short wEnvSeg;
        void far *pfCmdTail;
        void far *pfrFCB_1;
        void far *pfrFCB_2;
    } le;

    le.wEnvSeg = 0;
    le.pfCmdTail = MK_FP(segs.ds, (unsigned)cmdTail);
    le.pfrFCB_1 = MK_FP(segs.ds, fcb1);
    le.pfrFCB_2 = MK_FP(segs.ds, fcb2);

    // execute subprocess
    union REGS regs;
    regs.x.ax = 0x4b00;
    regs.x.dx = (unsigned)cmd;
    segs.es = segs.ds;
    regs.x.bx = (unsigned)&le;

    intdosx(&regs, &regs, &segs);

    return (regs.x.cflag == 0) ? 0 : regs.x.ax;
}

int rungame()
{
    char *buffer = ipc_buffer.cmdline;

    struct SREGS segs;
    segread(&segs);

    char *dir = buffer;
    char *args = strchr(dir, ' ');
    if (args != NULL) {
        *args = '\0';
        ++args;
    } else {
        args = "";
    }

    char *file = buffer;
    char *here = strrchr(dir, '/');
    if (here != NULL) {
        *here = '\0';
        file = here + 1;
    }

    char oldCwd[64];
    union REGS regs;

    // save old directory
    regs.h.ah = 0x47;
    regs.h.dl = 0; // drive code (0 = default)
    regs.x.si = (unsigned)oldCwd;
    intdosx(&regs, &regs, &segs);
    if (oldCwd[0] == '\0') {
        // HACK to be able to return to cwd
        oldCwd[0] = '\\';
        oldCwd[1] = '\0';
    }

    // change directory
    regs.h.ah = 0x3b;
    regs.x.dx = (unsigned)dir;
    intdosx(&regs, &regs, &segs);

    printf("Launching %s...\n", ipc_buffer.title);
    int error = run_command(file, args);

    if (error) {
        printf("Failed to launch\n\tdir='%s'\n\tfile='%s'\n\targs='%s'\n\terror=%x\n", dir, file, args, error);
        getch();
    } else {
        if ((ipc_buffer.game_flags & FLAG_HAS_END_SCREEN) != 0) {
            // show end screen
            printf("\r\n(press any key to return to menu)");
            fflush(stdout);
            getch();
        }
    }

    // change directory
    regs.h.ah = 0x3b;
    regs.x.dx = (unsigned)oldCwd;
    intdosx(&regs, &regs, &segs);

    return (error == 0);
}

int runmenu()
{
    struct SREGS segs;
    segread(&segs);

    int my_ds = segs.ds;
    int my_offset = (unsigned)&ipc_buffer;

    const char HEXDIGITS[] = "0123456789ABCDEF";

    char args[10];

    char *dest = args;
    *dest++ = HEXDIGITS[(my_ds >> 12)&0xF];
    *dest++ = HEXDIGITS[(my_ds >> 8)&0xF];
    *dest++ = HEXDIGITS[(my_ds >> 4)&0xF];
    *dest++ = HEXDIGITS[(my_ds >> 0)&0xF];
    *dest++ = ':';
    *dest++ = HEXDIGITS[(my_offset >> 12)&0xF];
    *dest++ = HEXDIGITS[(my_offset >> 8)&0xF];
    *dest++ = HEXDIGITS[(my_offset >> 4)&0xF];
    *dest++ = HEXDIGITS[(my_offset >> 0)&0xF];
    *dest++ = '\0';

    printf("Launching menu...\n");
    return (run_command("menu.exe", args) == 0);
}

static void _interrupt _far
(*old_int21_handler)(void);

static char
original_filename_buf[64];

static char
redirect_filename_buf[64];

/**
 * This dummy function is used here to provide
 * some data storage in the code segment.
 *
 * While we are executing in hook_int21(), "cs" is
 * is set to this function's code segment, and we
 * can use its offset to read the "ds" of this
 * module and get a pointer to redirect_filename_buf.
 **/
static int
dummy_function_for_data_storage(int a)
{
    int res = 0;
    for (int i=0; i<a; ++i) {
        res += a;
    }
    return res;
}

struct DummyFunctionDataStorage {
    short ds;
    short original_filename_buf_offset;
    short redirect_filename_buf_offset;
};

static void _interrupt
hook_int21(union INTPACK r)
{
    /**
     * 3C = create/truncate file
     * 3D = open existing file (AL = 0 read only, AL = 1 write only, AL = 2 read/write)
     **/
    if (r.h.ah == 0x3c || r.h.ah == 0x3d) {
        int is_write = (r.h.ah == 0x3c || (r.h.ah == 0x3d && (r.h.al == 1 || r.h.al == 2)));

        // Store original ds/dx values here
        unsigned short orig_ds = r.w.ds;
        unsigned short orig_dx = r.w.dx;

        /* Determine DS from CS */
        struct DummyFunctionDataStorage far *dfds =
            (struct DummyFunctionDataStorage far *)dummy_function_for_data_storage;

        /* filename to open */
        char far *filename = MK_FP(r.w.ds, r.w.dx);
        char far *original_filename_far = MK_FP(dfds->ds, dfds->original_filename_buf_offset);
        char far *redirect_filename_far = MK_FP(dfds->ds, dfds->redirect_filename_buf_offset);

        char far *fn_cmp = filename;
        char far *or_cmp = original_filename_far;

        while (*fn_cmp != '\0' && *fn_cmp == *or_cmp) {
            ++fn_cmp;
            ++or_cmp;
        }

        if (*fn_cmp == '\0' && *or_cmp == '\0') {
            filename = redirect_filename_far;
            r.w.ds = FP_SEG(filename);
            r.w.dx = FP_OFF(filename);

            {
                // dump the filename to the text/CGA screen
                short __far *cga = (short __far *)(0xb8000000L);
                char __far *vga = (char __far *)(0xa0000000L);

                int i = 0;
                while (filename[i]) {
                    cga[i] = (filename[i]) | 0x0200;
                    vga[i] = filename[i];

                    ++i;
                }
            }

            _chain_intr_dsdx(old_int21_handler, orig_ds, orig_dx);
        }
    }

    // Just chain normally
    _chain_intr(old_int21_handler);
}

static void
init_fileopen_hook()
{
    /**
     * Store our current data segment and pointer
     * to the redirect filename buffer in the
     * DummyFunctionDataStorage, so we can access
     * it in situations where we just know CS.
     **/
    struct SREGS segs;
    segread(&segs);

    struct DummyFunctionDataStorage far *dfds =
        (struct DummyFunctionDataStorage far *)dummy_function_for_data_storage;

    dfds->ds = segs.ds;
    dfds->original_filename_buf_offset = (short)original_filename_buf;
    dfds->redirect_filename_buf_offset = (short)redirect_filename_buf;

    // TODO: Retrieve original and redirect file names from game catalog,
    // and only if we have figured out that we are running from CD-ROM :)

    //strcpy(original_filename_buf, "GAMECTLG.DAT");
    //strcpy(redirect_filename_buf, "C:\\GAMECTLG.DAT");

    strcpy(original_filename_buf, "loonies8.hig");
    strcpy(redirect_filename_buf, "C:\\LOON8RE.DIR");

    old_int21_handler = _dos_getvect(0x21);
    _dos_setvect(0x21, hook_int21);
}

static void
deinit_fileopen_hook()
{
    _dos_setvect(0x21, old_int21_handler);
}

int main()
{
    printf("DOS Game Jam Demo Disc START.EXE\n");
    printf("Git rev %s (%s, %s)\n", VERSION, BUILDDATE, BUILDTIME);

    //init_fileopen_hook();

    int result = 0;

    ipc_buffer.magic = IPC_BUFFER_MAGIC;

    sprintf(ipc_buffer.catalog_filename, "GAMECTLG.DAT");

    // Must determine this here, as when the menu.exe is loaded, the
    // free conventional memory will be less than what games start with.
    printf("Free conventional memory: ");
    fflush(stdout);
    ipc_buffer.free_conventional_memory_bytes = get_free_conventional_memory_bytes();
    printf("%lu bytes\n", ipc_buffer.free_conventional_memory_bytes);
    fflush(stdout);

    // show meta information for at least a second, even on fast machines
    sleep(1);

    while (1) {
        textmode_reset();

        if (!runmenu()) {
            result = 1;
            break;
        }

        textmode_reset();

        if (ipc_buffer.request == IPC_EXIT) {
            break;
        } else if (ipc_buffer.request == IPC_RUN_GAME) {
            rungame();
        }
    }

    textmode_reset();

    if (result != 0) {
        printf("Exiting, result = %d\n", result);
    }

    //deinit_fileopen_hook();

    return result;
}
