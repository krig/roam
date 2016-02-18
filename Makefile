CC = clang
SOURCE = src/roam_linux.c
FILES = $(wildcard src/*c) $(wildcard src/*.h) $(wildcard stb/*.h)
DEBUG_CFLAGS = -O0 -g -Wall -Wextra -pedantic -Wno-unused-function -Wno-unused-parameter -fno-strict-aliasing -fomit-frame-pointer -std=gnu11
RELEASE_CFLAGS = -O2 -Wall -Wextra -pedantic -Wno-unused-function -Wno-unused-parameter -m64 -mfpmath=sse -mtune=generic -fno-strict-aliasing -fomit-frame-pointer -std=gnu11 -DNDEBUG
GL_CFLAGS = `pkg-config --cflags gl`
GL_LIBS = `pkg-config --libs gl`
GLEW_CFLAGS = `pkg-config --cflags glew`
GLEW_LIBS = `pkg-config --libs glew`
SDL2_CFLAGS = `pkg-config --cflags sdl2`
SDL2_LIBS = `pkg-config --libs sdl2`
STB_CFLAGS = -Istb

.PHONY: all clean

all: roam

roamdbg: $(FILES)
	$(CC) $(DEBUG_CFLAGS) $(STB_CFLAGS) $(GL_CFLAGS) $(GLEW_CFLAGS) $(SDL2_CFLAGS) -o roamdbg -lm -lpthread $(SOURCE) $(GL_LIBS) $(GLEW_LIBS) $(SDL2_LIBS)

roam:
	$(CC) $(RELEASE_CFLAGS) $(STB_CFLAGS) $(GL_CFLAGS) $(GLEW_CFLAGS) $(SDL2_CFLAGS) -o roam -lm -lpthread $(SOURCE) $(GL_LIBS) $(GLEW_LIBS) $(SDL2_LIBS)

run: roam
	./roam

