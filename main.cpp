#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// ================= HISTOGRAMA =================
vector<int> computeHistogram(SDL_Surface* img) {
    vector<int> hist(256, 0);

    Uint32* pixels = (Uint32*)img->pixels;
    int total = img->w * img->h;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(img->format);

    for (int i = 0; i < total; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], fmt, NULL, &r, &g, &b);
        int gray = (r + g + b) / 3;
        hist[gray]++;
    }

    return hist;
}

// ================= DESENHAR HISTOGRAMA =================
void drawHistogram(SDL_Renderer* renderer, vector<int>& hist, int width, int height) {
    int maxVal = *max_element(hist.begin(), hist.end());
    float barWidth = (float)width / 256.0f;

    for (int i = 0; i < 256; i++) {
        float barHeight = ((float)hist[i] / maxVal) * height;

        SDL_FRect bar = {
            i * barWidth,
            height - barHeight,
            barWidth,
            barHeight
        };

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &bar);
    }
}

// ================= GRAYSCALE =================
SDL_Surface* toGray(SDL_Surface* img) {
    SDL_Surface* result = SDL_ConvertSurface(img, img->format);

    Uint32* pixels = (Uint32*)result->pixels;
    int total = result->w * result->h;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(result->format);

    for (int i = 0; i < total; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], fmt, NULL, &r, &g, &b);

        Uint8 y = (r + g + b) / 3;
        pixels[i] = SDL_MapRGB(fmt, NULL, y, y, y);
    }

    return result;
}

// ================= EQUALIZAÇÃO =================
SDL_Surface* equalize(SDL_Surface* img) {
    vector<int> hist = computeHistogram(img);

    int total = img->w * img->h;
    vector<float> cdf(256, 0);

    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++)
        cdf[i] = cdf[i - 1] + hist[i];

    for (int i = 0; i < 256; i++)
        cdf[i] = cdf[i] / total;

    SDL_Surface* result = SDL_ConvertSurface(img, img->format);

    Uint32* pixels = (Uint32*)result->pixels;
    int totalPixels = result->w * result->h;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(result->format);

    for (int i = 0; i < totalPixels; i++) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], fmt, NULL, &r, &g, &b);

        int gray = (r + g + b) / 3;
        Uint8 newVal = (Uint8)(cdf[gray] * 255);

        pixels[i] = SDL_MapRGB(fmt, NULL, newVal, newVal, newVal);
    }

    return result;
}

// ================= MAIN =================
int main(int argc, char* argv[]) {

    if (argc < 2) {
        cout << "Uso: ./programa imagem.png\n";
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Surface* img = IMG_Load(argv[1]);
    if (!img) {
        cout << "Erro ao carregar imagem\n";
        return 1;
    }

    SDL_Surface* gray = toGray(img);

    // Estatísticas
    vector<int> hist = computeHistogram(gray);

    double sum = 0;
    int total = gray->w * gray->h;

    for (int i = 0; i < 256; i++)
        sum += i * hist[i];

    double mean = sum / total;

    double variance = 0;
    for (int i = 0; i < 256; i++)
        variance += hist[i] * pow(i - mean, 2);

    variance /= total;
    double stddev = sqrt(variance);

    cout << "Media: " << mean << endl;
    cout << "Desvio padrao: " << stddev << endl;

    // ================= JANELA IMAGEM =================
    SDL_Window* window = SDL_CreateWindow("Imagem", gray->w, gray->h, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, gray);

    // ================= JANELA HISTOGRAMA =================
    SDL_Window* histWindow = SDL_CreateWindow("Histograma", 512, 400, 0);
    SDL_Renderer* histRenderer = SDL_CreateRenderer(histWindow, NULL);

    bool running = true;
    bool isEqualized = false;
    SDL_Surface* equalizedImg = nullptr;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;

            if (event.type == SDL_EVENT_KEY_DOWN) {

                if (event.key.key == SDLK_E) {
                    if (!isEqualized) {
                        equalizedImg = equalize(gray);
                        texture = SDL_CreateTextureFromSurface(renderer, equalizedImg);
                        isEqualized = true;
                    } else {
                        texture = SDL_CreateTextureFromSurface(renderer, gray);
                        isEqualized = false;
                    }
                }

                if (event.key.key == SDLK_S) {
                    IMG_SavePNG(isEqualized ? equalizedImg : gray, "output.png");
                    cout << "Imagem salva!\n";
                }
            }
        }

        // Render imagem
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Render histograma
        SDL_RenderClear(histRenderer);
        drawHistogram(histRenderer, hist, 512, 400);
        SDL_RenderPresent(histRenderer);
    }

    SDL_Quit();
    return 0;
}
