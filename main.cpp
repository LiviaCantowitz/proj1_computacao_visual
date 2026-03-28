#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

// ================= HISTOGRAMA =================
vector<int> computeHistogram(SDL_Surface* img) {
    vector<int> hist(256, 0);
    Uint32* pixels = (Uint32*)img->pixels;
    int total = img->w * img->h;

    for (int i = 0; i < total; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], img->format, &r, &g, &b);
        hist[r]++;
    }

    return hist;
}

void drawHistogram(SDL_Renderer* renderer, vector<int>& hist, int width, int height) {
    int maxVal = *max_element(hist.begin(), hist.end());
    if (maxVal == 0) maxVal = 1;

    int barWidth = max(1, width / 256);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < 256; i++) {
        int barHeight = (hist[i] * height) / maxVal;

        SDL_Rect bar = {
            i * barWidth,
            height - barHeight,
            barWidth,
            barHeight
        };

        SDL_RenderFillRect(renderer, &bar);
    }
}

// ================= CONVERSÃO =================
SDL_Surface* toGray(SDL_Surface* img) {
    SDL_Surface* result = SDL_ConvertSurface(img, img->format);

    Uint32* pixels = (Uint32*)result->pixels;
    int total = result->w * result->h;

    for (int i = 0; i < total; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], result->format, &r, &g, &b);

        Uint8 y = 0.2125*r + 0.7154*g + 0.0721*b;
        pixels[i] = SDL_MapRGB(result->format, y, y, y);
    }

    return result;
}

// ================= EQUALIZAÇÃO =================
SDL_Surface* equalize(SDL_Surface* img) {
    vector<int> hist = computeHistogram(img);
    vector<float> cdf(256, 0);

    int total = img->w * img->h;

    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++)
        cdf[i] = cdf[i - 1] + hist[i];

    for (int i = 0; i < 256; i++)
        cdf[i] = (cdf[i] - cdf[0]) / (total - 1) * 255;

    SDL_Surface* result = SDL_ConvertSurface(img, img->format);
    Uint32* pixels = (Uint32*)result->pixels;

    for (int i = 0; i < total; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], result->format, &r, &g, &b);

        Uint8 newVal = (Uint8)cdf[r];
        pixels[i] = SDL_MapRGB(result->format, newVal, newVal, newVal);
    }

    return result;
}

// ================= MAIN =================
int main(int argc, char* argv[]) {

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_Surface* original = IMG_Load(argv[1]);
    SDL_Surface* gray = toGray(original);
    SDL_Surface* equalized = nullptr;

    bool isEqualized = false;

    SDL_Window* window = SDL_CreateWindow("Imagem", gray->w, gray->h, 0);
    SDL_Window* window2 = SDL_CreateWindow("Histograma", 400, 300, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Renderer* renderer2 = SDL_CreateRenderer(window2, NULL);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, gray);

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT)
                running = false;

            if (event.type == SDL_EVENT_KEY_DOWN) {

                if (event.key.keysym.sym == SDLK_e) {
                    if (!isEqualized) {
                        equalized = equalize(gray);
                        texture = SDL_CreateTextureFromSurface(renderer, equalized);
                        isEqualized = true;
                    } else {
                        texture = SDL_CreateTextureFromSurface(renderer, gray);
                        isEqualized = false;
                    }
                }

                if (event.key.keysym.sym == SDLK_s) {
                    IMG_SavePNG(isEqualized ? equalized : gray, "output_image.png");
                }
            }
        }

        // HISTOGRAMA
        SDL_SetRenderDrawColor(renderer2, 0, 0, 0, 255);
        SDL_RenderClear(renderer2);

        vector<int> hist = computeHistogram(isEqualized ? equalized : gray);
        drawHistogram(renderer2, hist, 400, 300);

        SDL_RenderPresent(renderer2);

        // IMAGEM
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}