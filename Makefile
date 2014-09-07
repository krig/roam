.PHONY: all clean

HEADERS=src/common.h src/math3d.h src/easing.h
SRC=src/main.c src/math3d.c

roam: $(SRC) $(HEADERS)
	@gcc -g -O2 -std=c11 -o roam $(SRC) -lm \
			`pkg-config --cflags --libs glew` \
			`pkg-config --cflags --libs gl` \
			`pkg-config --cflags --libs sdl2`

all: roam

clean:
	@rm -f roam
