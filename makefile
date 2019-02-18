CC = gcc
CFLAGS = -O3

scanfix: scanfix.c lib.o
	$(CC) -o $@ $< lib.o -lm ${PIE}

.PHONY: clean
clean:
	rm -f scanfix *~ *.o
