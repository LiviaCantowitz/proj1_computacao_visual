/* ---- Projeto 1 - Computação Visual ---- */
/* ---- Alunos: Alexandre Ribeiro de Souza  RA: 10417845
                Livia Negrucci Cantowitz RA:10389419 ---- */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ---- Tamanho fixo da janela secundária ---- */
#define SEC_W 340
#define SEC_H 540
#define HIST_SIZE 256

/* ---- Layout da janela secundária ---- */
#define HX 20
#define HY 50
#define HW (SEC_W - 40)
#define HH 200
#define BX 20
#define BY (HY + HH + 130)
#define BW (SEC_W - 40)
#define BH 44
#define FSIZE 14

/* ---- Estado global ---- */
typedef struct {
    SDL_Surface  *orig, *gray, *eq;
    SDL_Texture  *tex;
    SDL_Window   *wMain, *wSec;
    SDL_Renderer *rMain, *rSec;
    TTF_Font     *font;
    int   hist[HIST_SIZE];
    double mean, stddev;
    bool  equalized;
    int   btnState; /* 0=normal 1=hover 2=press */
} App;

/* ---- Utilitários ---- */
static Uint32 *px(SDL_Surface *s, int x, int y) {
    return (Uint32*)((Uint8*)s->pixels + y*s->pitch + x*4);
}

static void fillRect(SDL_Renderer *r, int x, int y, int w, int h, Uint8 R, Uint8 G, Uint8 B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_FRect f = {(float)x,(float)y,(float)w,(float)h};
    SDL_RenderFillRect(r, &f);
}

static void drawRect(SDL_Renderer *r, int x, int y, int w, int h, Uint8 R, Uint8 G, Uint8 B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_FRect f = {(float)x,(float)y,(float)w,(float)h};
    SDL_RenderRect(r, &f);
}

static void drawText(SDL_Renderer *r, TTF_Font *f, const char *t, int x, int y, SDL_Color c) {
    if (!f) return;
    SDL_Surface *s = TTF_RenderText_Blended(f, t, 0, c);
    if (!s) return;
    SDL_Texture *tx = SDL_CreateTextureFromSurface(r, s);
    if (tx) {
        SDL_FRect dst = {(float)x,(float)y,(float)s->w,(float)s->h};
        SDL_RenderTexture(r, tx, NULL, &dst);
        SDL_DestroyTexture(tx);
    }
    SDL_DestroySurface(s);
}

/* ---- Histograma ---- */
static void calcHist(SDL_Surface *gray, int hist[HIST_SIZE]) {
    memset(hist, 0, HIST_SIZE * sizeof(int));
    SDL_Surface *c = SDL_ConvertSurface(gray, SDL_PIXELFORMAT_RGBA8888);
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(c->format);
    for (int y = 0; y < c->h; y++)
        for (int x = 0; x < c->w; x++) {
            Uint8 r,g,b,a;
            SDL_GetRGBA(*px(c,x,y), fmt, NULL, &r,&g,&b,&a);
            hist[r]++;
        }
    SDL_DestroySurface(c);
}

static void analyzeHist(int hist[HIST_SIZE], int total, double *mean, double *stddev) {
    *mean = 0;
    for (int i = 0; i < HIST_SIZE; i++) *mean += (double)i * hist[i];
    *mean /= total;
    double var = 0;
    for (int i = 0; i < HIST_SIZE; i++) { double d = i - *mean; var += d*d*hist[i]; }
    *stddev = sqrt(var / total);
}

/* ---- Conversão para cinza ---- */
static bool isGray(SDL_Surface *s) {
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA8888);
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(c->format);
    bool gray = true;
    for (int y = 0; y < c->h && gray; y++)
        for (int x = 0; x < c->w && gray; x++) {
            Uint8 r,g,b,a; SDL_GetRGBA(*px(c,x,y), fmt, NULL, &r,&g,&b,&a);
            if (r!=g || g!=b) gray = false;
        }
    SDL_DestroySurface(c);
    return gray;
}

static SDL_Surface *toGray(SDL_Surface *s) {
    SDL_Surface *src = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA8888);
    SDL_Surface *dst = SDL_CreateSurface(src->w, src->h, SDL_PIXELFORMAT_RGBA8888);
    const SDL_PixelFormatDetails *fs = SDL_GetPixelFormatDetails(src->format);
    const SDL_PixelFormatDetails *fd = SDL_GetPixelFormatDetails(dst->format);
    for (int y = 0; y < src->h; y++)
        for (int x = 0; x < src->w; x++) {
            Uint8 r,g,b,a; SDL_GetRGBA(*px(src,x,y), fs, NULL, &r,&g,&b,&a);
            Uint8 l = (Uint8)(0.2125*r + 0.7154*g + 0.0721*b);
            *px(dst,x,y) = SDL_MapRGBA(fd, NULL, l,l,l,a);
        }
    SDL_DestroySurface(src);
    return dst;
}

/* ---- Equalização ---- */
static SDL_Surface *equalize(SDL_Surface *gray, int hist[HIST_SIZE]) {
    int total = gray->w * gray->h;
    int cdf[HIST_SIZE]; cdf[0] = hist[0];
    for (int i = 1; i < HIST_SIZE; i++) cdf[i] = cdf[i-1] + hist[i];
    int cmin = 0;
    for (int i = 0; i < HIST_SIZE; i++) { if (cdf[i]>0){cmin=cdf[i];break;} }
    Uint8 lut[HIST_SIZE];
    for (int i = 0; i < HIST_SIZE; i++) {
        int v = (int)(((double)(cdf[i]-cmin)/(total-cmin))*255+0.5);
        lut[i] = (Uint8)(v<0?0:v>255?255:v);
    }
    SDL_Surface *src = SDL_ConvertSurface(gray, SDL_PIXELFORMAT_RGBA8888);
    SDL_Surface *dst = SDL_CreateSurface(src->w, src->h, SDL_PIXELFORMAT_RGBA8888);
    const SDL_PixelFormatDetails *fs = SDL_GetPixelFormatDetails(src->format);
    const SDL_PixelFormatDetails *fd = SDL_GetPixelFormatDetails(dst->format);
    for (int y = 0; y < src->h; y++)
        for (int x = 0; x < src->w; x++) {
            Uint8 r,g,b,a; SDL_GetRGBA(*px(src,x,y), fs, NULL, &r,&g,&b,&a);
            Uint8 e = lut[r];
            *px(dst,x,y) = SDL_MapRGBA(fd, NULL, e,e,e,a);
        }
    SDL_DestroySurface(src);
    return dst;
}

/* ---- Atualiza textura e histograma ---- */
static void updateDisplay(App *a) {
    if (a->tex) { SDL_DestroyTexture(a->tex); a->tex = NULL; }
    SDL_Surface *cur = a->equalized ? a->eq : a->gray;
    a->tex = SDL_CreateTextureFromSurface(a->rMain, cur);
    calcHist(cur, a->hist);
    analyzeHist(a->hist, cur->w*cur->h, &a->mean, &a->stddev);
}

/* ---- Renderização ---- */
static void renderMain(App *a) {
    SDL_SetRenderDrawColor(a->rMain, 20,20,20,255);
    SDL_RenderClear(a->rMain);
    if (a->tex) SDL_RenderTexture(a->rMain, a->tex, NULL, NULL);
    SDL_RenderPresent(a->rMain);
}

static void renderSec(App *a) {
    SDL_Renderer *r = a->rSec;
    SDL_SetRenderDrawColor(r, 28,28,34,255);
    SDL_RenderClear(r);

    SDL_Color white  = {255,255,255,255};
    SDL_Color yellow = {255,215,60,255};
    SDL_Color cyan   = {70,200,220,255};
    SDL_Color lgray  = {160,160,170,255};

    drawText(r, a->font, "Histograma", HX, 12, white);

    /* Histograma */
    int hmax = 1;
    for (int i = 0; i < HIST_SIZE; i++) if (a->hist[i]>hmax) hmax=a->hist[i];
    fillRect(r, HX, HY, HW, HH, 18,18,24);
    drawRect(r, HX, HY, HW, HH, 70,70,85);
    SDL_SetRenderDrawColor(r, 90,170,255,255);
    for (int i = 0; i < HW; i++) {
        int idx = (int)((double)i/HW*HIST_SIZE);
        if (idx>=HIST_SIZE) idx=HIST_SIZE-1;
        int bh = (int)((double)a->hist[idx]/hmax*(HH-2));
        SDL_RenderLine(r, (float)(HX+i),(float)(HY+HH-1-bh),(float)(HX+i),(float)(HY+HH-1));
    }
    drawText(r, a->font, "0",   HX,           HY+HH+2, lgray);
    drawText(r, a->font, "128", HX+HW/2-8,    HY+HH+2, lgray);
    drawText(r, a->font, "255", HX+HW-22,     HY+HH+2, lgray);

    /* Análise */
    char buf[128];
    const char *bright = a->mean<85?"escura":a->mean<170?"media":"clara";
    const char *contr  = a->stddev<40?"baixo":a->stddev<80?"medio":"alto";
    snprintf(buf, sizeof(buf), "Media:  %.1f  (%s)", a->mean, bright);
    drawText(r, a->font, buf, HX, HY+HH+28, yellow);
    snprintf(buf, sizeof(buf), "Desvio: %.1f  (contraste %s)", a->stddev, contr);
    drawText(r, a->font, buf, HX, HY+HH+52, cyan);

    /* Botão */
    Uint8 br=38,bg=115,bb=210;
    if (a->btnState==1) { br=80;  bg=160; bb=255; }
    if (a->btnState==2) { br=20;  bg=75;  bb=180; }
    fillRect(r, BX,BY,BW,BH, br,bg,bb);
    drawRect(r, BX,BY,BW,BH, 180,210,255);
    const char *lbl = a->equalized ? "Ver original" : "Equalizar";
    int lx = BX + BW/2 - (int)(strlen(lbl)*FSIZE*0.30f);
    drawText(r, a->font, lbl, lx, BY+BH/2-FSIZE/2, white);

    SDL_RenderPresent(r);
}

/* ================================================================== */
int main(int argc, char *argv[])
{
    if (argc < 2) { fprintf(stderr, "Uso: %s imagem.ext\n", argv[0]); return 1; }

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    App a; memset(&a, 0, sizeof(App));

    /* Carrega imagem */
    a.orig = IMG_Load(argv[1]);
    if (!a.orig) { fprintf(stderr, "Erro: %s\n", SDL_GetError()); return 1; }

    a.gray = isGray(a.orig) ? SDL_ConvertSurface(a.orig, SDL_PIXELFORMAT_RGBA8888)
                             : toGray(a.orig);

    int W = a.gray->w, H = a.gray->h;

    /* Janela principal */
    a.wMain = SDL_CreateWindow("Processamento de Imagens", W, H, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowPosition(a.wMain, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    a.rMain = SDL_CreateRenderer(a.wMain, NULL);

    /* Janela secundária */
    int wx, wy; SDL_GetWindowPosition(a.wMain, &wx, &wy);
    a.wSec = SDL_CreateWindow("Analise", SEC_W, SEC_H, 0);
    SDL_SetWindowParent(a.wSec, a.wMain);
    SDL_SetWindowPosition(a.wSec, wx+W+6, wy);
    a.rSec = SDL_CreateRenderer(a.wSec, NULL);

    /* Fonte */
    const char *fonts[] = {"assets/font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf", NULL};
    for (int i = 0; fonts[i] && !a.font; i++)
        a.font = TTF_OpenFont(fonts[i], FSIZE);

    updateDisplay(&a);

    /* Loop principal */
    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_EVENT_QUIT || e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = false;

            if (e.type==SDL_EVENT_KEY_DOWN) {
                if (e.key.key==SDLK_S) {
                    SDL_Surface *cur = a.equalized ? a.eq : a.gray;
                    IMG_SavePNG(cur, "output_image.png");
                    printf("Salvo em output_image.png\n");
                }
                if (e.key.key==SDLK_ESCAPE) running = false;
            }

            /* Mouse na janela secundária */
            if (e.type==SDL_EVENT_MOUSE_MOTION &&
                e.motion.windowID==SDL_GetWindowID(a.wSec)) {
                float mx=e.motion.x, my=e.motion.y;
                bool over = mx>=BX&&mx<=BX+BW&&my>=BY&&my<=BY+BH;
                if (a.btnState!=2) a.btnState = over?1:0;
            }
            if (e.type==SDL_EVENT_MOUSE_BUTTON_DOWN &&
                e.button.windowID==SDL_GetWindowID(a.wSec)) {
                float mx=e.button.x, my=e.button.y;
                if (mx>=BX&&mx<=BX+BW&&my>=BY&&my<=BY+BH) a.btnState=2;
            }
            if (e.type==SDL_EVENT_MOUSE_BUTTON_UP &&
                e.button.windowID==SDL_GetWindowID(a.wSec)) {
                float mx=e.button.x, my=e.button.y;
                bool over = mx>=BX&&mx<=BX+BW&&my>=BY&&my<=BY+BH;
                if (over && a.btnState==2) {
                    if (!a.equalized) {
                        if (a.eq) SDL_DestroySurface(a.eq);
                        int hg[HIST_SIZE]; calcHist(a.gray, hg);
                        a.eq = equalize(a.gray, hg);
                        a.equalized = true;
                    } else {
                        a.equalized = false;
                    }
                    updateDisplay(&a);
                    a.btnState = 1;
                } else a.btnState = 0;
            }
        }
        renderMain(&a);
        renderSec(&a);
        SDL_Delay(16);
    }

    /* Cleanup */
    if (a.font) TTF_CloseFont(a.font);
    if (a.tex)  SDL_DestroyTexture(a.tex);
    SDL_DestroyRenderer(a.rSec);  SDL_DestroyWindow(a.wSec);
    SDL_DestroyRenderer(a.rMain); SDL_DestroyWindow(a.wMain);
    if (a.eq)   SDL_DestroySurface(a.eq);
    SDL_DestroySurface(a.gray);
    SDL_DestroySurface(a.orig);
    TTF_Quit(); SDL_Quit();
    return 0;
}
