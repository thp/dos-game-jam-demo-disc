all: gamectlg.dat pcatalog menu dosmenu.exe

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@

pcatalog: pcatalog.o gamectlg.o
	$(CC) -o $@ $^

menu: menu.o gamectlg.o
	$(CC) -o $@ $^ -lncurses

dosmenu.exe: menu.c gamectlg.c
	i586-pc-msdosdjgpp-gcc -o $@ menu.c gamectlg.c

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o menu.o menu dosmenu.exe

.PHONY: clean
