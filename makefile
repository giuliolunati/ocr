CC = gcc
CFLAGS = -O3

grafix: grafix.c libgrafix.o common.h
	$(CC) -o $@ $< libgrafix.o -lm ${PIE}

libgrafix.o: libgrafix.c common.h
	$(CC) -c -o $@ $(CFLAGS) $<

test: test.c libgrafix.a common.h
	$(CC) -o $@ $< libgrafix.a -lm 

.PHONY: clean
clean:
	rm -f grafix *~ *.o
