#ifndef GPU_H
#define GPU_H

#include "mmu.h"

#include <stdint.h>

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

  void *app_data;
  void *pixel_data;

#ifdef CGB
  bool color_correction;
#endif
};

typedef struct gpu_s gpu;

typedef bool (*gpu_init_cb)(gpu*);
typedef void (*gpu_uninit_cb)(gpu*);
typedef bool (*gpu_alloc_image_buffer_cb)(gpu*);
typedef void (*gpu_render_cb)(gpu*);

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
#define OBJ_TILE_VRAM_BANK_FLAG 0x08

#define PALETTE_NUM_MASK 0x07

#define PRIORITY_COLOR 0xFF

#define WINDOW_SCALE 2

int gpu_init(gpu *g, gpu_init_cb cb);

void gpu_reset(gpu *g);

#ifndef NDEBUG
void gpu_debug_print(gpu *g, level l);
#else
#define gpu_debug_print(g, l) ;
#endif

int gpu_update(gpu *g, memory *mem, const uint8_t last_t, gpu_render_cb r_cb, gpu_alloc_image_buffer_cb l_cb);

#endif
