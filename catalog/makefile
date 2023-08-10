default: gamectlg.dat menu.exe start.exe

ifeq ($V,1)
PYTHON_EXTRA := --verbose
endif

extra: default pcatalog

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s

start.exe: start.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o menu.exe start.exe start.o menu.o

.PHONY: clean default extra
