CC = gcc
CFLAGS = -O3
LDFLAGS = -O3
OBJS = vector.o image.o transform.o scale.o convolution.o misc.o draw.o select.o dithering.o poisson.o
LIBS = libgrafix.a -llept -ljpeg -lpng -ltiff -lm

grafix: grafix.c libgrafix.a common.h
	$(CC) $(LDFLAGS) -o $@ $< libgrafix.a -lm 

grafix-lept: grafix-lept.c libgrafix.a common.h
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

libgrafix.a: ${OBJS}
	ar rs $@ $(OBJS)

test: test.c libgrafix.a common.h
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

.PHONY: clean
clean:
	rm -f grafix grafix-lept *~ *.o *.a
