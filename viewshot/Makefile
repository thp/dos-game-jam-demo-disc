obj = viewshot.obj video.obj image.obj
bin = viewshot.exe

#opt = -otexan
dbg = -d3

AS = nasm
CC = wcc
LD = wlink

ASFLAGS = -fobj
CFLAGS = $(dbg) $(opt) $(def) -zq -bt=dos -ml
LDFLAGS = option map

$(bin): $(obj)
	%write objlist.lnk $(obj)
	$(LD) debug all name $@ system dos file { @objlist.lnk } $(LDFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*.c

.asm.obj:
	$(AS) $(ASFLAGS) -o $@ $[*.asm

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj) $(bin)
	rm -f *.map *.lnk
!else
clean: .symbolic
	del *.obj
	del *.lnk
	del *.map
	del $(bin)
!endif
