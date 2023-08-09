all: gamectlg.dat pcatalog dosmenu.exe

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@

pcatalog: pcatalog.o gamectlg.o
	$(CC) -o $@ $^

dosmenu.exe: menu.c gamectlg.c
	i586-pc-msdosdjgpp-gcc -o $@ $^

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o dosmenu.exe

.PHONY: clean
