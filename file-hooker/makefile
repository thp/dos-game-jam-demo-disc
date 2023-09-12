all: hooker.exe

hooker.o: hooker.c
	owcc -c -o $@ $< -bdos -I$(WATCOM)/h -mcmodel=t -std=c99

chain.o: chain.s
	owcc -c -o $@ $<

hooker.exe: hooker.o chain.o
	owcc -o hooker.exe hooker.o chain.o -bdos -mcmodel=t -std=c99

clean:
	rm -f hooker.o chain.o

distclean: clean
	rm -f hooker.exe
