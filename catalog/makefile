default: gamectlg.dat menu.exe start.exe demodisc.iso

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

start.exe: start.c chain.o
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s $(CFLAGS_VERSION)

chain.o: chain.s
	owcc -c -o $@ $^

demodisc.iso: gamectlg.dat menu.exe start.exe
	mkisofs -V 'DGJDEMO2023' -sysid 'DOS' -A 'DOS Game Jam Demo Disc 2023' -b FD13BOOT.IMG -o $@ -f layout

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o menu.exe start.exe start.o menu.o chain.o

distclean: clean
	$(RM) demodisc.iso

.PHONY: clean default extra distclean
