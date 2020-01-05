#ifndef CHESTER_H
#define CHESTER_H

#include "chester_internal.h"

void register_keys_callback(chester *chester, keys_cb cb);

void register_get_ticks_callback(chester *chester, get_ticks_cb cb);

void register_delay_callback(chester *chester, delay_cb cb);

void register_gpu_init_callback(chester *chester, gpu_init_cb cb);

void register_gpu_uninit_callback(chester *chester, gpu_uninit_cb cb);

void register_gpu_alloc_image_buffer_callback(chester *chester, gpu_alloc_image_buffer_cb cb);

void register_gpu_render_callback(chester *chester, gpu_render_cb cb);

void register_serial_callback(chester *chester, serial_cb cb);

void save_if_needed(chester *chester);

#if CGB
bool get_color_correction(chester *chester);

void set_color_correction(chester *chester, bool color_correction);
#endif

bool init(chester *chester, const char* rom, const char* save_path, const char* bootloader);

void uninit(chester *chester);

int run(chester *chester);

#endif
