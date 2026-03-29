CC     = gcc
TARGET = programa
CFLAGS = -std=c99 -Wall -O2 $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf)
LIBS   = $(shell pkg-config --libs   sdl3 sdl3-image sdl3-ttf) -lm

all:
	$(CC) $(CFLAGS) main.c -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) output_image.png
