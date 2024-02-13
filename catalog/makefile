default: gamectlg.dat vgamenu.exe menu.exe vgastart.exe start.exe viewshot.exe \
	demodisc.iso

CFLAGS_VERSION += -DVERSION=\"$(shell git describe --always --dirty)\"
CFLAGS_VERSION += -DBUILDDATE=\"$(shell env LC_ALL=C date +%F)\"
CFLAGS_VERSION += -DBUILDTIME=\"$(shell env LC_ALL=C date +%R)\"

ifeq ($V,1)
PYTHON_EXTRA := --verbose
endif

extra: default pcatalog

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)
	rm -f menu.o

vgamenu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -DVGAMENU -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)
	rm -f menu.o

start.exe: start.c chain.o
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION) -fm=start.map
	rm -f start.o

vgastart.exe: start.c chain.o
	owcc -o $@ $^ -DVGAMENU -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION) -fm=start.map
	rm -f start.o

chain.o: chain.s
	owcc -c -o $@ $^

viewshot.exe: ../viewshot/viewshot.c
	cd ../viewshot && wmake
	cp ../viewshot/viewshot.exe .

demodisc.iso: gamectlg.dat menu.exe start.exe viewshot.exe
	mkisofs -V 'DGJDEMO2023' -sysid 'DOS' -A 'DOS Game Jam Demo Disc 2023' -b FD13BOOT.IMG -o $@ -f layout

refresh:
	python3 convert-to-pcx.py
	python3 update-screenshot-counts.py

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o vgamenu.exe menu.exe vgastart.exe start.exe start.o menu.o chain.o start.map

distclean: clean
	$(RM) demodisc.iso

run: start.exe menu.exe viewshot.exe
	dosbox-x $<

runvga: vgastart.exe vgamenu.exe
	dosbox-x $<

boot: demodisc.iso
	qemu-system-x86_64 -cdrom $< -device sb16

.PHONY: clean default extra distclean refresh run runvga boot
