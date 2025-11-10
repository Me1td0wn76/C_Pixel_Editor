/*
C Pixel Art Editor (single-file)

Features:
- Simple pixel-editor using SDL2
- Click to paint pixels on a grid
- Right-click to erase
- Click palette to change current color, or number keys 1-9
- Save canvas as BMP with key 's' (prompts filename in console)
- Load BMP with key 'l' (prompts filename in console) and maps it into the grid
- Clear canvas with 'c'
- Toggle grid lines with 'g'
- Resize cells by +/- with '[' and ']' (recreates window)

Build (Linux/macOS/WSL):
  gcc c_pixel_editor.c -o c_pixel_editor `sdl2-config --cflags --libs`

Windows (MinGW):
  gcc c_pixel_editor.c -o c_pixel_editor -lmingw32 -lSDL2main -lSDL2

Requires: SDL2 development libraries.

Usage:
  Run the program. Use mouse to draw on the grid. Press keys for actions.

Notes:
- This is a compact educational program showing common C idioms: arrays, malloc/free, file I/O (via SDL), pointers, and simple UI loop.
- The file is intentionally single-file to make it easy to explore and modify.
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* Configuration */
static int CELLS_X = 32;
static int CELLS_Y = 32;
static int CELL_SIZE = 16;
#define PALETTE_COUNT 12 /* number of palette colors */

/* Globals */
static uint8_t *canvas = NULL; /* each cell stores palette index (0..PALETTE_COUNT-1) */
static SDL_Color palette[PALETTE_COUNT];
static int current_color = 1; /* default non-zero color */
static int show_grid = 1;

/* Helpers */
static void ensure_canvas_allocated() {
    if (canvas) return;
    canvas = (uint8_t*)calloc(CELLS_X * CELLS_Y, sizeof(uint8_t));
    if (!canvas) {
        fprintf(stderr, "Failed to allocate canvas\n");
        exit(1);
    }
}

static void init_default_palette() {
    /* A friendly palette (index 0 is transparent/erase/background) */
    SDL_Color p[PALETTE_COUNT] = {
        {255,255,255,255}, /* 0 - white (background) */
        {0,0,0,255},       /* 1 - black */
        {255,0,0,255},     /* 2 - red */
        {0,255,0,255},     /* 3 - lime */
        {0,0,255,255},     /* 4 - blue */
        {255,255,0,255},   /* 5 - yellow */
        {255,165,0,255},   /* 6 - orange */
        {128,0,128,255},   /* 7 - purple */
        {0,255,255,255},   /* 8 - cyan */
        {255,192,203,255}, /* 9 - pink */
        {128,128,128,255}, /* 10 - gray */
        {139,69,19,255}    /* 11 - brown */
    };
    for (int i=0;i<PALETTE_COUNT;i++) palette[i] = p[i];
}

static void clear_canvas() {
    ensure_canvas_allocated();
    memset(canvas, 0, CELLS_X * CELLS_Y);
}

static void draw_canvas_to_renderer(SDL_Renderer *ren) {
    ensure_canvas_allocated();
    for (int y=0;y<CELLS_Y;y++){
        for (int x=0;x<CELLS_X;x++){
            int idx = y*CELLS_X + x;
            uint8_t cidx = canvas[idx];
            SDL_Color col = palette[cidx];
            SDL_Rect r = { x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE };
            SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
            SDL_RenderFillRect(ren, &r);
            if (show_grid) {
                SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
                SDL_RenderDrawRect(ren, &r);
            }
        }
    }
}

static void draw_palette_ui(SDL_Renderer *ren, int win_w, int win_h) {
    int pal_x = CELLS_X * CELL_SIZE + 10;
    int pal_y = 10;
    int box = 24;
    for (int i=0;i<PALETTE_COUNT;i++){
        SDL_Rect r = { pal_x + (i%2)*(box+8), pal_y + (i/2)*(box+8), box, box };
        SDL_Color c = palette[i];
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 0,0,0,255);
        SDL_RenderDrawRect(ren, &r);
        if (i == current_color) {
            SDL_Rect out = { r.x-2, r.y-2, r.w+4, r.h+4 };
            SDL_SetRenderDrawColor(ren, 0,0,0,255);
            SDL_RenderDrawRect(ren, &out);
        }
    }
}

/* Save as BMP: create a surface of CELLS_X*CELL_SIZE etc and save */
static int save_canvas_as_bmp(const char *filename) {
    ensure_canvas_allocated();
    int w = CELLS_X * CELL_SIZE;
    int h = CELLS_Y * CELL_SIZE;
    /* create an RGBA32 buffer */
    uint32_t *pixels = (uint32_t*)malloc(sizeof(uint32_t) * w * h);
    if (!pixels) return -1;
    for (int y=0;y<h;y++){
        for (int x=0;x<w;x++){
            int cx = x / CELL_SIZE;
            int cy = y / CELL_SIZE;
            uint8_t cidx = canvas[cy*CELLS_X + cx];
            SDL_Color col = palette[cidx];
            uint32_t p = (col.r<<24) | (col.g<<16) | (col.b<<8) | col.a; /* ARGB in memory order depends on surface masks below */
            pixels[y*w + x] = p;
        }
    }
    /* Create a surface with 32bit masks; SDL_SaveBMP expects a surface with appropriate masks */
    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void*)pixels, w, h, 32, w*4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
    );
    if (!surf) {
        free(pixels);
        return -1;
    }
    int r = SDL_SaveBMP(surf, filename);
    SDL_FreeSurface(surf);
    free(pixels);
    return r;
}

/* Find nearest palette index by Euclidean distance in RGB space */
static int nearest_palette_index(SDL_Color c) {
    int best = 0;
    int bestd = INT32_MAX;
    for (int i=0;i<PALETTE_COUNT;i++){
        int dr = (int)c.r - palette[i].r;
        int dg = (int)c.g - palette[i].g;
        int db = (int)c.b - palette[i].b;
        int d = dr*dr + dg*dg + db*db;
        if (d < bestd) { bestd = d; best = i; }
    }
    return best;
}

/* Load BMP and map into canvas by sampling center of each cell */
static int load_bmp_to_canvas(const char *filename) {
    SDL_Surface *surf = SDL_LoadBMP(filename);
    if (!surf) return -1;
    SDL_Surface *fmt = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGB24, 0);
    SDL_FreeSurface(surf);
    if (!fmt) return -1;
    int img_w = fmt->w, img_h = fmt->h;
    uint8_t *pixels = (uint8_t*)fmt->pixels;
    int pitch = fmt->pitch;
    for (int cy=0; cy<CELLS_Y; cy++){
        for (int cx=0; cx<CELLS_X; cx++){
            /* sample at center of cell in image coords */
            int sx = (int)(( (cx + 0.5) / (double)CELLS_X) * img_w);
            int sy = (int)(( (cy + 0.5) / (double)CELLS_Y) * img_h);
            if (sx < 0) sx = 0; if (sx >= img_w) sx = img_w-1;
            if (sy < 0) sy = 0; if (sy >= img_h) sy = img_h-1;
            uint8_t *p = pixels + sy * pitch + sx*3;
            SDL_Color c = { p[0], p[1], p[2], 255 };
            int pi = nearest_palette_index(c);
            canvas[cy*CELLS_X + cx] = (uint8_t)pi;
        }
    }
    SDL_FreeSurface(fmt);
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 3) {
        CELLS_X = atoi(argv[1]);
        CELLS_Y = atoi(argv[2]);
        if (CELLS_X <= 0) CELLS_X = 32;
        if (CELLS_Y <= 0) CELLS_Y = 32;
    }
    ensure_canvas_allocated();
    init_default_palette();
    clear_canvas();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    int win_w = CELLS_X * CELL_SIZE + 200;
    int win_h = CELLS_Y * CELL_SIZE + 20;

    SDL_Window *win = SDL_CreateWindow("C Pixel Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    int running = 1;
    int mouse_down = 0;
    int mouse_button = 0;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                mouse_down = 1; mouse_button = e.button.button;
                int mx = e.button.x; int my = e.button.y;
                if (mx < CELLS_X * CELL_SIZE) {
                    int cx = mx / CELL_SIZE;
                    int cy = my / CELL_SIZE;
                    if (cx >=0 && cx < CELLS_X && cy>=0 && cy<CELLS_Y) {
                        if (mouse_button == SDL_BUTTON_LEFT) canvas[cy*CELLS_X + cx] = current_color;
                        else if (mouse_button == SDL_BUTTON_RIGHT) canvas[cy*CELLS_X + cx] = 0;
                    }
                } else {
                    /* palette click */
                    int pal_x = CELLS_X * CELL_SIZE + 10;
                    int relx = mx - pal_x;
                    int box = 24; int spacing = 8;
                    if (relx >= 0) {
                        int col = relx / (box + spacing);
                        int row = (my - 10) / (box + spacing);
                        int idx = row*2 + col;
                        if (idx >=0 && idx < PALETTE_COUNT) current_color = idx;
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                mouse_down = 0;
            } else if (e.type == SDL_MOUSEMOTION) {
                if (mouse_down) {
                    int mx = e.motion.x; int my = e.motion.y;
                    if (mx < CELLS_X * CELL_SIZE) {
                        int cx = mx / CELL_SIZE;
                        int cy = my / CELL_SIZE;
                        if (cx >=0 && cx < CELLS_X && cy>=0 && cy<CELLS_Y) {
                            if (mouse_button == SDL_BUTTON_LEFT) canvas[cy*CELLS_X + cx] = current_color;
                            else if (mouse_button == SDL_BUTTON_RIGHT) canvas[cy*CELLS_X + cx] = 0;
                        }
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = 0;
                else if (k == SDLK_c) clear_canvas();
                else if (k == SDLK_g) show_grid = !show_grid;
                else if (k == SDLK_s) {
                    char fname[256];
                    printf("Save filename (example out.bmp): ");
                    if (fgets(fname, sizeof(fname), stdin)) {
                        size_t ln = strlen(fname); if (ln && fname[ln-1]=='\n') fname[ln-1]='\0';
                        if (strlen(fname) > 0) {
                            if (save_canvas_as_bmp(fname) == 0) printf("Saved %s\n", fname);
                            else printf("Failed to save %s\n", fname);
                        }
                    }
                } else if (k == SDLK_l) {
                    char fname[256];
                    printf("Load BMP filename: ");
                    if (fgets(fname, sizeof(fname), stdin)) {
                        size_t ln = strlen(fname); if (ln && fname[ln-1]=='\n') fname[ln-1]='\0';
                        if (strlen(fname) > 0) {
                            if (load_bmp_to_canvas(fname) == 0) printf("Loaded %s\n", fname);
                            else printf("Failed to load %s\n", fname);
                        }
                    }
                } else if (k == SDLK_LEFTBRACKET) {
                    if (CELL_SIZE > 4) CELL_SIZE -= 1;
                    /* recreate window size */
                    SDL_SetWindowSize(win, CELLS_X * CELL_SIZE + 200, CELLS_Y * CELL_SIZE + 20);
                } else if (k == SDLK_RIGHTBRACKET) {
                    CELL_SIZE += 1;
                    SDL_SetWindowSize(win, CELLS_X * CELL_SIZE + 200, CELLS_Y * CELL_SIZE + 20);
                } else if (k >= SDLK_0 && k <= SDLK_9) {
                    int n = (k - SDLK_0);
                    if (n >=0 && n < PALETTE_COUNT) current_color = n;
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderClear(ren);

        draw_canvas_to_renderer(ren);
        draw_palette_ui(ren, win_w, win_h);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (canvas) free(canvas);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
