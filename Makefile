CC = cc

all: build/main

build:
	mkdir -p build

build/main: build/main.o build/server.o build/html_renderer.o build/base.o
	$(CC) $^ -o $@

build/main.o: src/main.c | build
	$(CC) -c $< -o $@

build/server.o: src/server.c src/server.h | build
	$(CC) -c $< -o $@

build/html_renderer.o: src/html_renderer.c src/html_renderer.h | build
	$(CC) -c $< -o $@

build/base.o: src/base.c src/base.h | build
	$(CC) -c $< -o $@

clean:
	rm -rf ./build
