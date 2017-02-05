#ifndef GPU_H
#define GPU_H

#include "mmu.h"

#include <stdint.h>

#include <SDL2/SDL.h>

typedef enum state_e {
  READ_OAM = 0x02,
  READ_VRAM = 0x03,
  HBLANK = 0x00,
  VBLANK = 0x01
} state;

struct gpu_s {
  struct {
    uint16_t t;
  } clock;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  void *locked_pixel_data;
};

typedef struct gpu_s gpu;

#define X_RES 160
#define Y_RES 144

#define READ_OAM_CYCLES 80
#define READ_VRAM_CYCLES 172
#define HBLANK_CYCLES 204
#define VBLANK_CYCLES 456

#define LCD_STAT_MODE_MASK 0x03

#define OBJ_PALETTE_FLAG 0x10
#define OBJ_PRIORITY_FLAG 0x80
#define OBJ_Y_FLIP_FLAG 0x40
#define OBJ_X_FLIP_FLAG 0x20

#define WINDOW_SCALE 2

int gpu_init(gpu *g);

void gpu_uninit(gpu *g);

void gpu_reset(gpu *g);

#ifdef ENABLE_DEBUG
void gpu_debug_print(gpu *g, level l);
#else
#define gpu_debug_print(g, l) ;
#endif

int gpu_update(gpu *g, memory *mem, const uint8_t last_t);

#endif
