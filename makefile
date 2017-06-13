fixpix: fixpix.c
	gcc -O3 -o $@ $< -lm ${PIE}

.PHONY: clean
clean:
	rm -f fixpix *~
