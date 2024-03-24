TARGETS := gamectlg.dat menu.exe menu32.exe start.exe menu_src.zip

default: $(TARGETS)

iso: demodisc.iso

VERSION := $(shell git describe --always --dirty)
BUILDDATE := $(shell env LC_ALL=C date +%F)
BUILDTIME := $(shell env LC_ALL=C date +%R)

ifeq ($V,1)
PYTHON_EXTRA := --verbose
endif

DOSBOX ?= dosbox-x

SAVEGAMES += DEMO2023/CHRMN242/CHROMONO.SAV
SAVEGAMES += DEMO2023/CATS10/GAME.SAV
SAVEGAMES += DEMO2023/DEMOGR11/GAMESAVE.DAT
SAVEGAMES += DEMO2023/EVILDEMO/DOSBALL.SAV
SAVEGAMES += DEMO2023/LOON8R66/LOONIES8.HIG
SAVEGAMES += DEMO2023/LOONXR66/LOONIES8.HIG
SAVEGAMES += DEMO2023/NEUTTOWR/SAVE.SAV
SAVEGAMES += DEMO2023/STELD06/STELLAR.SCO
SAVEGAMES += DEMO2023/STARLITE/STARLITE.CON
SAVEGAMES += DEMO2023/PARCHB51/PARCHIS.CFG
SAVEGAMES += DEMO2023/PARCHB51/PARCHIS.HPT
SAVEGAMES += DEMO2023/FATUMA/HISCORE.DAT

extra: default pcatalog

menu_src.zip: makefile $(wildcard *.c *.h *.s) csv2dat.py gamectlg.csv credits.txt png2dat.py cntshots.py gamectlg.dat config.h makefile.dos
	zip $@ $^

config.h: config.h.in
	sed \
		-e 's/%VERSION%/$(VERSION)/g' \
		-e 's/%BUILDTIME%/$(BUILDTIME)/g' \
		-e 's/%BUILDDATE%/$(BUILDDATE)/g' \
		$^ >$@

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

menu.c: config.h
start.c: config.h

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -O2 -s
	rm -f menu.o

menu32.exe: menu.c gamectlg.c
	i586-pc-msdosdjgpp-gcc -o $@ $^ -O2 -Wall -Wextra -Wpedantic -Werror -fanalyzer
	i586-pc-msdosdjgpp-strip -s -x $@

start.exe: start.c chain.o
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s -fm=start.map
	rm -f start.o

chain.o: chain.s
	owcc -c -o $@ $^

demodisc.iso: $(TARGETS) clean-saves
	mkisofs -V 'DGJDEMO2023' -sysid 'DOS' -A 'DOS Game Jam Demo Disc 2023' -b FD13BOOT.IMG -o $@ -f layout

refresh:
	python3 png2dat.py
	python3 cntshots.py

clean:
	$(RM) pcatalog pcatalog.o gamectlg.o start.o menu.o chain.o start.map config.h $(TARGETS)

clean-saves:
	$(RM) $(SAVEGAMES)

distclean: clean
	$(RM) demodisc.iso

run: start.exe menu.exe menu32.exe
	$(DOSBOX) $<

boot: demodisc.iso
	qemu-system-x86_64 -cdrom $< -device sb16

.PHONY: clean default extra distclean refresh iso run boot config.h clean-saves
