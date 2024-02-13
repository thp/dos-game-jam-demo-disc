all: plasma

plasma: plasma.c
	i586-pc-msdosdjgpp-gcc plasma.c ../../src/vesa.c ../../src/pcx.c -I../../src -o plasma.exe -O2

clean:
	del plasma.exe
