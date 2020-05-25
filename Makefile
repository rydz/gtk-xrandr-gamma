CFLAGS := `pkg-config --cflags --libs gtk+-3.0`

gtk-xrandr-gamma: main.c
	cc -o gtk-xrandr-gamma main.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o gtk-xrandr-gamma

.PHONY: run
run:
	make && ./gtk-xrandr-gamma
