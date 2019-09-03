CC = gcc
SOURCE = src/roam_linux.c
FILES = $(wildcard src/*.c) $(wildcard src/*.h)

OPTFLAGS = -Og
PROFFLAGS = -Og -pg
CFLAGS = -Wall -Wextra -pedantic -Wno-unused-function -Wno-unused-parameter -mtune=native -fno-strict-aliasing -std=gnu11 -DNDEBUG
GL_CFLAGS = `pkg-config --cflags gl`
GL_LIBS = `pkg-config --libs gl`
GLEW_CFLAGS = `pkg-config --cflags glew`
GLEW_LIBS = `pkg-config --libs glew`
SDL2_CFLAGS = `pkg-config --cflags sdl2`
SDL2_LIBS = `pkg-config --libs sdl2`
STB_CFLAGS = -isystem stb

.PHONY: all clean profile

all: roam

roam: $(FILES)
	$(CC) $(OPTFLAGS) $(CFLAGS) $(STB_CFLAGS) $(GL_CFLAGS) $(GLEW_CFLAGS) $(SDL2_CFLAGS) -o roam -lm -lpthread $(SOURCE) $(GL_LIBS) $(GLEW_LIBS) $(SDL2_LIBS)

run: roam
	./roam

profile: $(FILES)
	$(CC) $(PROFFLAGS) $(CFLAGS) $(STB_CFLAGS) $(GL_CFLAGS) $(GLEW_CFLAGS) $(SDL2_CFLAGS) -o roam -lm -lpthread $(SOURCE) $(GL_LIBS) $(GLEW_LIBS) $(SDL2_LIBS)

clean:
	rm -f ./roam ./roam.prof
