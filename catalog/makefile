default: gamectlg.dat menu.exe start.exe

ifeq ($V,1)
PYTHON_EXTRA := --verbose
endif

extra: default pcatalog djmenu.exe

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@ $(PYTHON_EXTRA)

pcatalog: pcatalog.c gamectlg.c
	$(CC) -o $@ $^

djmenu.exe: menu.c gamectlg.c
	i586-pc-msdosdjgpp-gcc -o $@ $^

menu.exe: menu.c gamectlg.c
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c99 -Os -s

start.exe: start.cpp
	owcc -o $@ $^ -mcmodel=t -bdos -march=i86 -I$(WATCOM)/h -std=c++99 -Os -s

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o djmenu.exe menu.exe start.exe start.o menu.o

.PHONY: clean default extra
