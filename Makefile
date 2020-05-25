CFLAGS := `pkg-config --cflags --libs gtk+-3.0`

xredshift: main.c
	cc -o xredshift main.c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o xredshift

.PHONY: run
run:
	make && ./xredshift
