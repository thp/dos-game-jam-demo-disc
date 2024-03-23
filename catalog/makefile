TARGETS := gamectlg.dat menu.exe menu32.exe start.exe menu_src.zip

default: $(TARGETS)

iso: demodisc.iso

CFLAGS_VERSION += -DVERSION=\"$(shell git describe --always --dirty)\"
CFLAGS_VERSION += -DBUILDDATE=\"$(shell env LC_ALL=C date +%F)\"
CFLAGS_VERSION += -DBUILDTIME=\"$(shell env LC_ALL=C date +%R)\"

ifeq ($V,1)
PYTHON_EXTRA := --verbose
endif

DOSBOX ?= dosbox-x

extra: default pcatalog

menu_src.zip: makefile $(wildcard *.c *.h *.s) csv2dat.py gamectlg.csv credits.txt png2dat.py cntshots.py
	zip $@ $^

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -O2 -s $(CFLAGS_VERSION)
	rm -f menu.o

menu32.exe: menu.c gamectlg.c
	i586-pc-msdosdjgpp-gcc -o $@ menu.c gamectlg.c -O2 $(CFLAGS_VERSION) -Wall -Wextra -Wpedantic -Werror -fanalyzer
	i586-pc-msdosdjgpp-strip -s -x $@

start.exe: start.c chain.o
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION) -fm=start.map
	rm -f start.o

chain.o: chain.s
	owcc -c -o $@ $^

demodisc.iso: $(TARGETS)
	mkisofs -V 'DGJDEMO2023' -sysid 'DOS' -A 'DOS Game Jam Demo Disc 2023' -b FD13BOOT.IMG -o $@ -f layout

refresh:
	python3 png2dat.py
	python3 cntshots.py

clean:
	$(RM) pcatalog pcatalog.o gamectlg.o start.o menu.o chain.o start.map $(TARGETS)

distclean: clean
	$(RM) demodisc.iso

run: start.exe menu.exe menu32.exe
	$(DOSBOX) $<

boot: demodisc.iso
	qemu-system-x86_64 -cdrom $< -device sb16

.PHONY: clean default extra distclean refresh iso run boot
