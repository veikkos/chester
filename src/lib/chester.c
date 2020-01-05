#include "chester.h"
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "keys.h"
#include "mmu.h"
#include "loader.h"
#include "logger.h"
#include "timer.h"
#include "save.h"
#include "sync.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <libgen.h>
#endif

bool init(chester *chester, const char* rom, const char* save_path, const char* bootloader)
{
  chester->rom = NULL;
  chester->bootloader = NULL;

  chester->keys_cumulative_ticks = 0;
  chester->keys_ticks = 15000;

  chester->save_timer = 0;
  chester->save_game_file = NULL;
  chester->save_supported = false;

  uint32_t rom_size = 0;
  chester->rom = read_file(rom, &rom_size, true);

  if (!chester->rom)
    {
      return false;
    }

  gb_log_print_rom_info(chester->rom);

  cpu_reset(&chester->cpu_reg);

  if (!gpu_init(&chester->g, chester->gpu_init_cb))
    {
      return false;
    }

  mmu_reset(&chester->mem);

  const mbc type = get_type(chester->rom);

  if (type == NOT_SUPPORTED)
    {
      return false;
    }

  mmu_set_rom(&chester->mem, chester->rom, type, rom_size);

#ifdef CGB
  switch (chester->rom[0x0143])
  {
  case 0x80:
  case 0xC0:
    chester->mem.cgb_mode = true;
    break;
  }
#endif

  chester->bootloader = read_file(bootloader, NULL, false);

  if (chester->bootloader)
    {
      chester->cpu_reg.pc = 0x0000;
      mmu_set_bootloader(&chester->mem, chester->bootloader);
    }
  else if (bootloader)
    {
      gb_log (INFO, "No bootloader found at ./%s", bootloader);
    }

  mmu_set_keys(&chester->mem, &chester->k);
  keys_reset(&chester->k);

  sync_init(&chester->s, 100000, chester->ticks_cb);

  chester->save_supported = (chester->mem.rom.type & MBC_BATTERY_MASK);

  if (chester->save_supported)
    {
      char* base_name_cpy = strdup(rom);
#ifdef WIN32
      char base_name[128];
      _splitpath(base_name_cpy, NULL, NULL, base_name, NULL);
#else
      char* base_name = basename(base_name_cpy);
#endif

      const char extension[] = ".sav";
      const size_t save_file_path_length = strlen(base_name) + strlen(extension) + 1 +
              (save_path ? strlen(save_path) : 0);
      chester->save_game_file = malloc(save_file_path_length);
      if (save_path)
        {
          strcpy(chester->save_game_file, save_path);
          strcat(chester->save_game_file, base_name);
        }
      else
        {
          strcpy(chester->save_game_file, base_name);
        }
      strcat(chester->save_game_file, extension);

      load_game(chester->save_game_file, &chester->mem);
      free(base_name_cpy);
    }

  return true;
}

void register_keys_callback(chester *chester, keys_cb cb)
{
  chester->k_cb = cb;
}

void register_get_ticks_callback(chester *chester, get_ticks_cb cb)
{
  chester->ticks_cb = cb;
}

void register_delay_callback(chester *chester, delay_cb cb)
{
  chester->delay_cb = cb;
}

void register_gpu_init_callback(chester *chester, gpu_init_cb cb)
{
  chester->gpu_init_cb = cb;
}

void register_gpu_uninit_callback(chester *chester, gpu_uninit_cb cb)
{
  chester->gpu_uninit_cb = cb;
}

void register_gpu_alloc_image_buffer_callback(chester *chester, gpu_alloc_image_buffer_cb cb)
{
  chester->gpu_alloc_image_buffer_cb = cb;
}

void register_gpu_render_callback(chester *chester, gpu_render_cb cb)
{
  chester->gpu_render_cb = cb;
}

void register_serial_callback(chester *chester, serial_cb cb)
{
  chester->mem.serial_cb = cb;
}

void uninit(chester *chester)
{
  save_if_needed(chester);

  if (chester->bootloader)
    {
      free(chester->bootloader);
      chester->bootloader = NULL;
    }

  if (chester->rom)
    {
      free(chester->rom);
      chester->rom = NULL;
    }

  chester->gpu_uninit_cb(&chester->g);

  gb_log_close_file();
}

void save_if_needed(chester *chester)
{
    if (chester->save_supported && chester->mem.banks.ram.written)
      {
        chester->mem.banks.ram.written = false;

        save_game(chester->save_game_file, &chester->mem);
      }
}

#if CGB
bool get_color_correction(chester *chester)
{
  return chester->g.color_correction;
}

void set_color_correction(chester *chester, bool color_correction)
{
  chester->g.color_correction = color_correction;
}
#endif

int run(chester *chester)
{
  int run_cycles = 4194304 / 4;
  while(run_cycles > 0)
    {
      if (chester->bootloader && !chester->mem.bootloader_running)
        {
          mmu_set_bootloader(&chester->mem, NULL);

          free(chester->bootloader);
          chester->bootloader = NULL;

          cpu_reset(&chester->cpu_reg);
        }

      cpu_debug_print(&chester->cpu_reg, ALL);
      mmu_debug_print(&chester->mem, ALL);
      gpu_debug_print(&chester->g, ALL);

      if (cpu_next_command(&chester->cpu_reg, &chester->mem))
        {
          gb_log (ERROR, "Could not process any longer");
          cpu_debug_print(&chester->cpu_reg, ERROR);
          mmu_debug_print(&chester->mem, ERROR);
          return -1;
        }

      // STOP should be handled
      if (gpu_update(&chester->g, &chester->mem, chester->cpu_reg.clock.last.t, chester->gpu_render_cb, chester->gpu_alloc_image_buffer_cb))
        {
          gb_log (ERROR, "GPU error");
          gpu_debug_print(&chester->g, ERROR);
          return -2;
        }

      if (!chester->cpu_reg.stop)
        timer_update(&chester->cpu_reg, &chester->mem);

      if (chester->keys_cumulative_ticks > chester->keys_ticks)
        {
          if (chester->save_supported && chester->save_timer++ >= 10000)
            {
              chester->save_timer = 0;

              if (chester->mem.banks.ram.written)
                {
                  chester->mem.banks.ram.written = false;

                  save_game(chester->save_game_file, &chester->mem);
                }
            }

          chester->keys_cumulative_ticks = 0;

          switch(chester->k_cb(&chester->k))
            {
            case -1:
              return 1;
            case 1:
              isr_set_if_flag(&chester->mem, MEM_IF_PIN_FLAG);
              chester->cpu_reg.halt =  false;
              chester->cpu_reg.stop =  false;
              break;
            default:
              break;
            }
        }
      else
        {
          chester->keys_cumulative_ticks += chester->cpu_reg.clock.last.t;
        }

      sync_time(&chester->s, chester->cpu_reg.clock.last.t, chester->ticks_cb, chester->delay_cb);

      run_cycles -= chester->cpu_reg.clock.last.t;
    }

  return 0;
}
