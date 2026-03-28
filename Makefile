all:
	g++ main.cpp -o programa `sdl2-config --cflags --libs` -lSDL2_image