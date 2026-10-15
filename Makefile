CC = cc

all: build/main

build:
	mkdir -p build

build/main: main.c | build
	$(CC) main.c -o $@

clean:
	rm -rf ./build
