all:
	gcc -O3 -o bin/benchmark benchmark.c
clean:
	rm -f bin/*
