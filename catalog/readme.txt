DOS Game Jam Demo Disc 2023
===========================

Thank you for downloading and (hopefully burning) this demonstration disc
of DOS Game Jam entries and assorted DOS games and utilities.

Released in April 2024, the 2023 Edition of this disc has been worked on
and off since August 2023, and has been tested successfully on real retro
hardware as well as software emulations thereof.

We hope you like this selection of fresh, great content targetting the DOS
family of operating systems. The disc is even bootable using FreeDOS - on
a compatible x86 BIOS machine (or UEFI machine with CSM, although YMMV)
you should be able to boot directly into the disc menu by booting from CD.


The Menu
--------

To start the menu, run START.EXE from the CD folder, e.g. assuming your
CD-ROM drive is assigned the letter "D:":

 d:
 cd \
 start

This will detect your CPU and launch the right menu (16-bit or 32-bit)
for your system, and also stay resident, so that after playing games
you'll return to the menu to continue browsing.

If for some reason you want to run the 16-bit version of the menu, you can
do so by supplying the command-line argument "16bit" to START.EXE, e.g. by
using "start 16bit" on the command line.

The menu starts in text mode by default, you can use F5 to switch from
text mode to VGA mode. Hitting F5 in VGA mode will switch to SVGA (VESA)
mode, and hitting it one more time in SVGA mode will get you back to the
original text mode.

Advantages of text mode are speed, color schemes (F2 to pick another scheme)
and easily readable text, plus compatibility. Advantages of the VGA menu are
speed and the ability to look at screenshots of the games while browsing,
with the disadvantage that it's only a single color scheme (boo!) and the
text is not quite readable. Finally, SVGA (VESA) mode gives you readable text
and high-resolution screenshots (well, most games are VGA or less, so the
screenshots are just upscaled, not "high resolution" as such) at the expense
of not being optimized, but if you have a beefy machine, it can run even this
inefficient code at acceptable frame rates (plus, patches accepted!).

Pro tip: If you like the text mode, but like to occassionally look at
screenshots, F4 works there as well (as long as you are the proud owner of
a VGA adapter).

EGA and CGA text modes are also supported for the menu, but graphics modes
are not for these adapter types. If you have a 286 with EGA or CGA graphics
and a CD-ROM drive, more power to you!

Navigate in style! Press F3 in the "All Games" list (or anywhere in the menu,
really) and you can do search and quickly jump to the right game.

Oh, and the source code for the menu is available in MENU_SRC.ZIP, so if you
want to study or improve it (patches welcome!), be my guest.


Development Tools
-----------------

Wonder how all those games have been made? Want to create one yourself?
You came to the right place in this README! Included as added special bonus
thingie on the disc are various frameworks, compilers, toolchains and
runtimes that will enable you to create games by just investing time, the
most valuable resource you have.

Want to try Allegro? Or more of a CGALIB person? We have both! There's also
the venerable DJGPP (a GCC-based compiler) for creating 32-bit protected
mode applications, and OpenWatcom for both 16-bit and 32-bit applications.
C/C++ bringing you down? FreePascal is also available! High level languages
not your style? Maybe the Netwide Assembler (NASM) is for you then! And if
you just want to create fancy text-based worlds, I hear ZZT is the thing.
For the new kids who like their daily dose of JavaScript or Lua, we have
DOjS and LOVEDOS as frameworks for creating games without having to compile
anything - and DOjS even comes with an integrated text editor!

And speaking of text editors, we included two versions of Vim for DOS on
the disc, as some people reported Vim 5.8 is a bit faster than Vim 7.3 on
older PCs.


Drivers and Tools
-----------------

DOS Navigator (think Norton Commander or Midnight Commander, but grey) is
the file manager of choice. CuteMouse acts as mouse driver (there's even
multiple versions on it, as sometimes BIOS routines vs talking directly to
the PS/2 port can be beneficial!). We got SBEMU to emulate Sound Blaster
and OPL-3 hardware with modern AC-7/HDA sound cards. ADLiPT for all your
OPL2LPT needs, UniSound and UniVBE to initialize your sound and graphics
hardware and a bunch of other assorted utilities from DOS Game Jammers.


Get in Touch
------------

We hang out on Discord in the "DOS Shareware Zone" server, this link might
or might not work in the future: https://discord.gg/D9Zb6MtqtK

There's also the DOS Game Jams on Itch.io, just google for it.

The official link for the DOS Game Jam Demo Disc is, and that link will
hopefully still work in the future (if not, and if the Internet Archive
is still around by that time -- chances are it is -- then just visit the
page on the Wayback machine and see how far this gets you):

    https://thp.io/2024/demodisc/



Credits
-------

For the games, you will find information for each game in the game menu,
and any accompanying README files.

8x16 font used in the VESA Menu:

facename Bm437 IBM PS/2thin2
copyright FON conversion  2015 VileR, license: CC BY-SA 4.0

Readme display in the menu currently calls out to:
"LIST" by Vernon D. Buerg.
