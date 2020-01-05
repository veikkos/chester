#include "loader.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t* read_file(const char* path, uint32_t *size, const bool is_rom)
{
  if (!path) return NULL;

  long file_size;
  char *buffer = NULL;
  size_t read_size;
  FILE* file = fopen (path, "rb");

  if (file != NULL)
    {
      fseek (file, 0, SEEK_END);
      file_size = ftell (file);

      if (is_rom && file_size <= 0x014F)
        {
          gb_log(ERROR, "File too small");
          fclose (file);
          return NULL;
        }
      else if (!is_rom && file_size != 0x0100)
        {
          gb_log(ERROR, "Invalid bootloader size");
          fclose (file);
          return NULL;
        }

      rewind (file);

      buffer = (char*) malloc (sizeof(char)*file_size);
      if (buffer == NULL)
        {
          gb_log(ERROR, "Memory error");
          fclose (file);
          return NULL;
        }

      read_size = fread (buffer, 1, file_size, file);
      if ((long)read_size != file_size)
        {
          gb_log(ERROR, "Reading error");
          free (buffer);
          fclose (file);
          return NULL;
        }

      if (size) *size = (uint32_t)read_size;
      fclose (file);
    }

  return (uint8_t*)buffer;
}

mbc get_type(uint8_t* rom)
{
  switch(rom[0x0147])
    {
    case 0x00: // ROM ONLY
      return NONE;
    case 0x01: // MBC1
    case 0x02: // MBC1+RAM
      return MBC1;
    case 0x03: // MBC1+RAM+BATTERY
      return MBC1_BATTERY;
    case 0x05: // MBC2
    case 0x06: // MBC2+BATTERY
    case 0x08: // ROM+RAM
    case 0x09: // ROM+RAM+BATTERY
    case 0x0B: // MMM01
    case 0x0C: // MMM01+RAM
    case 0x0D: // MMM01+RAM+BATTERY
    case 0x0F: // MBC3+TIMER+BATTERY
    case 0x10: // MBC3+TIMER+RAM+BATTERY
      gb_log(WARNING, "Unsupported ROM type");
      return NOT_SUPPORTED;
    case 0x11: // MBC3
    case 0x12: // MBC3+RAM
      return MBC3;
    case 0x13: // MBC3+RAM+BATTERY
      return MBC3_BATTERY;
    case 0x15: // MBC4
    case 0x16: // MBC4+RAM
    case 0x17: // MBC4+RAM+BATTERY
    case 0x19: // MBC5
    case 0x1A: // MBC5+RAM
      return MBC5;
    case 0x1B: // MBC5+RAM+BATTERY
      return MBC5_BATTERY;
    case 0x1C: // MBC5+RUMBLE
    case 0x1D: // MBC5+RUMBLE+RAM
      return MBC5;
    case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
      return MBC5_BATTERY;
    case 0xFC: // POCKET CAMERA
    case 0xFD: // BANDAI TAMA5
    case 0xFE: // HuC3
    case 0xFF: // HuC1+RAM+BATTERY
    default:
      gb_log(WARNING, "Unsupported ROM type");
      return NOT_SUPPORTED;
    }
}
