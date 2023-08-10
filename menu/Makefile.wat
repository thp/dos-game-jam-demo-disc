!ifdef __UNIX__
dosobj = src/dos/main.obj src/dos/keyb.obj src/dos/mouse.obj src/dos/timer.obj &
	src/dos/cdpmi.obj src/dos/vidsys.obj src/dos/drv_vga.obj src/dos/drv_vbe.obj &
	src/dos/drv_s3.obj
appobj = src/app.obj src/logger.obj src/menuscr.obj
rtkobj = src/rtk.obj src/rtk_draw.obj

incpath = -Isrc -Isrc/dos -Ilibs/imago/src
libpath = libpath libs/dos
libimago = libs/dos/imago.lib
!else
dosobj = src\dos\main.obj src\dos\keyb.obj src\dos\mouse.obj src\dos\timer.obj &
	src\dos\cdpmi.obj src\dos\vidsys.obj src\dos\drv_vga.obj src\dos\drv_vbe.obj &
	src\dos\drv_s3.obj
appobj = src\app.obj src\logger.obj src\menuscr.obj
rtkobj = src\rtk.obj src\rtk_draw.obj

incpath = -Isrc -Isrc\dos -Ilibs\imago\src
libpath = libpath libs\dos
libimago = libs\dos\imago.lib
!endif

obj = $(dosobj) $(appobj) $(rtkobj)
bin = menu.exe

opt = -otexan
libs = $(libimago)

AS = nasm
CC = wcc386
LD = wlink
ASFLAGS = -fobj
CFLAGS = -d3 $(opt) $(def) -s -zq -bt=dos $(incpath)
LDFLAGS = option map $(libpath) library { $(libs) }

$(bin): cflags.occ $(obj) $(libs)
	%write objects.lnk $(obj)
	%write ldflags.lnk $(LDFLAGS)
	$(LD) debug all name $@ system dos4g file { @objects } @ldflags

.c: src;src/dos
.asm: src;src/dos

cflags.occ: Makefile.wat
	%write $@ $(CFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.asm.obj:
	nasm $(ASFLAGS) -o $@ $[*.asm


!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(bin)
	rm -f cflags.occ *.lnk

$(libimago):
	cd libs/imago
	wmake -f Makefile.wat
	cd ../..
!else
clean: .symbolic
	del src\*.obj
	del src\dos\*.obj
	del src\gaw\*.obj
	del *.lnk
	del cflags.occ
	del $(bin)

$(libimago):
	cd libs\imago
	wmake -f Makefile.wat
	cd ..\..
!endif
