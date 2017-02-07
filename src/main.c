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
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <libgen.h>
#include <stdbool.h>

bool done = false;

void sig_handler(int signum __attribute__((unused)))
{
  done = true;
}

int main(int argc, char **argv)
{
  uint8_t* rom;
  uint8_t* bootloader;
  registers cpu_reg;
  memory mem;
  gpu g;
  keys k;
  sync_timer s;
  unsigned int save_timer = 0;
  char* save_game_file = NULL;
  bool save_supported;
  int ret_val = 0;
  const char* bootloader_file = "DMG_ROM.bin";

  int keys_cumulative_ticks = 0;
  const int keys_ticks = 10000;

  if (argc != 2)
    {
      gb_log(ERROR, "Usage: %s rom-file-path", argv[0]);
      return 1;
    }

  rom = read_file(argv[1], true);

  if (!rom)
    {
      return 1;
    }

  gb_log_print_rom_info(rom);

  cpu_reset(&cpu_reg);

  if (gpu_init(&g))
    {
      return 1;
    }

  mmu_reset(&mem);
  mmu_set_rom(&mem, rom, get_type(rom));

  bootloader = read_file(bootloader_file, false);

  if (bootloader)
    {
      cpu_reg.pc = 0x0000;
      mmu_set_bootloader(&mem, bootloader);
    }
  else
    {
      gb_log (INFO, "No bootloader found at ./%s", bootloader_file);
    }

  mmu_set_keys(&mem, &k);
  keys_reset(&k);

  sync_init(&s, 100000);

  save_supported = (mem.rom.type & MBC_BATTERY_MASK);

  if (save_supported)
    {
      char* base_name_cpy = strdup(argv[1]);
      char* base_name = basename(base_name_cpy);

      const char extension[] = ".sav";
      save_game_file = malloc(strlen(base_name) + strlen(extension) + 1);
      strcpy(save_game_file, base_name);
      strcat(save_game_file, extension);

      load_game(save_game_file, &mem);
      free(base_name_cpy);
    }

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  while (!done)
    {
      if (bootloader && !mem.bootloader_running)
        {
          mmu_set_bootloader(&mem, NULL);

          free(bootloader);
          bootloader = NULL;

          cpu_reset(&cpu_reg);
        }

      cpu_debug_print(&cpu_reg, ALL);
      mmu_debug_print(&mem, ALL);
      gpu_debug_print(&g, ALL);

      if (cpu_next_command(&cpu_reg, &mem))
        {
          gb_log (ERROR, "Could not process any longer");
          cpu_debug_print(&cpu_reg, ERROR);
          mmu_debug_print(&mem, ERROR);
          ret_val =  1;
          break;
        }

      // STOP should be handled
      if (gpu_update(&g, &mem, cpu_reg.clock.last.t))
        {
          gb_log (ERROR, "GPU error");
          gpu_debug_print(&g, ERROR);
          ret_val =  1;
          break;
        }

      if (!cpu_reg.stop)
        timer_update(&cpu_reg, &mem);

      if (keys_cumulative_ticks > keys_ticks)
        {
          if (save_supported && save_timer++ >= 10000)
            {
              save_timer = 0;

              if (mem.banks.ram.written)
                {
                  mem.banks.ram.written = false;

                  save_game(save_game_file, &mem);
                }
            }

          keys_cumulative_ticks = 0;

          switch(keys_update(&k))
            {
            case -1:
              done = true;
            case 1:
              isr_set_if_flag(&mem, MEM_IF_PIN_FLAG);
              cpu_reg.halt =  false;
              cpu_reg.stop =  false;
              break;
            default:
              break;
            }
        }
      else
        {
          keys_cumulative_ticks += cpu_reg.clock.last.t;
        }

      sync(&s, cpu_reg.clock.last.t);
    }

  if (save_supported && mem.banks.ram.written)
    {
      save_game(save_game_file, &mem);
    }

  free(bootloader);

  free(rom);

  free(save_game_file);

  gpu_uninit(&g);

  gb_log_close_file();

  return ret_val;
}
