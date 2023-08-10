src = $(wildcard src/*.c) $(wildcard src/dos/*.c)
asmsrc = $(wildcard src/*.asm) $(wildcard src/dos/*.asm)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = menu.exe

ifeq ($(findstring COMMAND.COM, $(SHELL)), COMMAND.COM)
	hostsys = dos
else
	hostsys = unix
	TOOLPREFIX = i586-pc-msdosdjgpp-
endif

#opt = -O3
dbg = -g
warn = -pedantic -Wall
inc = -Isrc/dos

CC = $(TOOLPREFIX)gcc
AR = $(TOOLPREFIX)ar
CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def) -MMD

$(bin): $(obj)
	$(CC) -o $@ -Wl,-Map=menu.map $(obj) $(LDFLAGS)

-include $(dep)

%.cof: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.cof: %.asm
	nasm -f coff -o $@ $<


.PHONY: clean
.PHONY: cleandep

ifeq ($(hostsys), dos)
clean:
	del src\*.cof
	del src\dos\*.cof
	del $(bin)
	del menu.map

cleandep:
	del src\*.dep
	del src\dos\*.dep
else
clean:
	rm -f $(obj) $(bin)

cleandep:
	rm -f $(dep)
endif
