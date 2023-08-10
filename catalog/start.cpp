#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

static char
ipc_buffer[512];

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

int rungame(char *buffer)
{
    int have_endscreen = 0;

    if (buffer[0] == '@') {
        have_endscreen = 1;
    }

    struct SREGS segs;
    segread(&segs);

    // skip over mode char (' ' or '@')
    ++buffer;

    char *dir = buffer;
    char *file = buffer;
    char *here = strrchr(dir, '/');
    if (here != NULL) {
        *here = '\0';
        file = here + 1;
    }

    char *args = strchr(file, ' ');
    if (args != NULL) {
        *args = '\0';
        ++args;
    } else {
        args = "";
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

    int error = run_command(file, args);

    if (error) {
        printf("Failed to launch dir=%s, file=%s, error=%x\n", dir, file, error);
        getch();
    } else {
        if (have_endscreen) {
            // show end screen
            printf("(press any key to return to menu)");
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
    int my_offset = (unsigned)ipc_buffer;

    const char HEXDIGITS[] = "0123456789ABCDEF";

    char args[] = {
        HEXDIGITS[(my_ds >> 12)&0xF],
        HEXDIGITS[(my_ds >> 8)&0xF],
        HEXDIGITS[(my_ds >> 4)&0xF],
        HEXDIGITS[(my_ds >> 0)&0xF],
        ':',
        HEXDIGITS[(my_offset >> 12)&0xF],
        HEXDIGITS[(my_offset >> 8)&0xF],
        HEXDIGITS[(my_offset >> 4)&0xF],
        HEXDIGITS[(my_offset >> 0)&0xF],
        '\0',
    };

    return (run_command("menu.exe", args) == 0);
}

int main()
{
    while (1) {
        if (!runmenu()) {
            printf("Cannot execute menu...\n");
            return 1;
        }

        if (strcmp(ipc_buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        rungame(ipc_buffer);
    }

    return 0;
}
