CC = gcc
CFLAGS = -O3

grafix: grafix.c libgrafix.o
	$(CC) -o $@ $< libgrafix.o -lm ${PIE}

.PHONY: clean
clean:
	rm -f grafix *~ *.o
