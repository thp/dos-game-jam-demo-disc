obj = fontconv.o
bin = fontconv

CFLAGS = -pedantic -Wall -g
LDFLAGS = -lGL -lglut -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
