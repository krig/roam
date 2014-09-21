.PHONY: all clean

HEADERS=src/common.h src/math3d.h src/easing.h src/shaders.h src/ui.h src/objfile.h
SRC=src/main.c src/math3d.c src/ui.c src/objfile.c

roam: $(SRC) $(HEADERS)
	@gcc -g -O0 -std=c11 -Istb -o roam $(SRC) -lm \
			`pkg-config --cflags --libs glew` \
			`pkg-config --cflags --libs gl` \
			`pkg-config --cflags --libs sdl2`

all: roam

clean:
	@rm -f roam
