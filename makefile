CC = gcc
CFLAGS = -O3
OBJS = vector.o image.o transform.o scale.o convolution.o misc.o draw.o

grafix: grafix.c libgrafix.a common.h
	$(CC) -o $@ $< libgrafix.a -lm 

libgrafix.a: ${OBJS}
	ar rs $@ $(OBJS)

test: test.c libgrafix.a common.h
	$(CC) -o $@ $< libgrafix.a -lm 

.PHONY: clean
clean:
	rm -f grafix *~ *.o
