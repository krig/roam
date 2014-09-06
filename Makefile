.PHONY: all clean

roam: src/main.c
	@gcc -o roam src/main.c \
			`pkg-config --cflags --libs glew` \
			`pkg-config --cflags --libs gl` \
			`pkg-config --cflags --libs sdl2`

all: roam

clean:
	@rm -f roam
