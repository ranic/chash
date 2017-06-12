all:
	gcc -std=c99 -O3 -o bin/benchmark benchmark.c
clean:
	rm -f bin/*
