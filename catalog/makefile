TARGETS := gamectlg.dat vgamenu.exe menu.exe start.exe vesamenu.exe viewshot.exe

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

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)
	rm -f menu.o

vgamenu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -DVGAMENU -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)
	rm -f menu.o

vesamenu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -DVESAMENU -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)
	rm -f menu.o

start.exe: start.c chain.o
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION) -fm=start.map
	rm -f start.o

chain.o: chain.s
	owcc -c -o $@ $^

viewshot.exe: ../viewshot/viewshot.c
	cd ../viewshot && env INCLUDE=$(WATCOM)/h wmake
	cp ../viewshot/viewshot.exe .

demodisc.iso: $(TARGETS)
	mkisofs -V 'DGJDEMO2023' -sysid 'DOS' -A 'DOS Game Jam Demo Disc 2023' -b FD13BOOT.IMG -o $@ -f layout

refresh:
	python3 convert-to-pcx.py
	python3 update-screenshot-counts.py

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o vgamenu.exe menu.exe start.exe start.o menu.o chain.o start.map vesamenu.exe

distclean: clean
	$(RM) demodisc.iso

run: start.exe menu.exe viewshot.exe vgamenu.exe vesamenu.exe
	$(DOSBOX) $<

boot: demodisc.iso
	qemu-system-x86_64 -cdrom $< -device sb16

.PHONY: clean default extra distclean refresh iso run boot
