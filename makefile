fixpix: fixpix.c
	gcc -O3 -o $@ $< -lm -pie

.PHONY: clean
clean:
	rm fixpix *~
