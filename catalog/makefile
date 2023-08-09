all: gamectlg.dat pcatalog

gamectlg.dat: csv2dat.py gamectlg.csv
	python3 $^ $@

pcatalog: pcatalog.o gamectlg.o
	$(CC) -o $@ $^

clean:
	$(RM) pcatalog gamectlg.dat pcatalog.o gamectlg.o

.PHONY: clean
