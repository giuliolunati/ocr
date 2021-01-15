CC = gcc
CFLAGS = -O3

grafix: grafix.c libgrafix.o common.h
	$(CC) -o $@ $< libgrafix.o -lm ${PIE}

libgrafix.o: libgrafix.c common.h
	$(CC) -c -o $@ $(CFLAGS) $<

.PHONY: clean
clean:
	rm -f grafix *~ *.o
