fixpix: fixpix.c
	gcc -o $@ $< -lm

.PHONY: clean
clean:
	rm fixpix *~
