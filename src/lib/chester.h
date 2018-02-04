#include "cpu.h"
#include "gpu.h"
#include "keys.h"
#include "mmu.h"
#include "sync.h"

struct chester_s {
  registers cpu_reg;
  memory mem;
  gpu g;
  keys k;
  sync_timer s;
  uint8_t* bootloader;
  uint8_t* rom;
  int keys_cumulative_ticks;
  int keys_ticks;
  unsigned int save_timer;
  char* save_game_file;
  bool save_supported;

  // callbacks
  keys_cb k_cb;
  get_ticks_cb ticks_cb;
  delay_cb delay_cb;
  gpu_init_cb gpu_init_cb;
  gpu_uninit_cb gpu_uninit_cb;
  gpu_lock_texture_cb gpu_lock_texture_cb;
  gpu_render_cb gpu_render_cb;
};

typedef struct chester_s chester;

void register_keys_callback(chester *chester, keys_cb cb);

void register_get_ticks_callback(chester *chester, get_ticks_cb cb);

void register_delay_callback(chester *chester, delay_cb cb);

void register_gpu_init_callback(chester *chester, gpu_init_cb cb);

void register_gpu_uninit_callback(chester *chester, gpu_uninit_cb cb);

void register_gpu_lock_texture_callback(chester *chester, gpu_lock_texture_cb cb);

void register_gpu_render_callback(chester *chester, gpu_render_cb cb);

bool init(chester *chester, const char* rom, const char* save_path, const char* bootloader);

void uninit(chester *chester);

int run(chester *chester);
