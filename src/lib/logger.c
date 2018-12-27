#include "logger.h"

#include <stdio.h>

#ifdef LOG_FILE
FILE *f = NULL;
#endif

#ifndef NDEBUG
void gb_log(const level l, const char *fmt, ...)
{
  if (l <= ERROR_LEVEL)
    {
      va_list args;
      va_start(args, fmt);
#ifdef LOG_FILE
      if (!f)
        f = fopen("log.txt", "a");
      if (f != NULL)
        {
          vfprintf(f, fmt, args);
          fprintf(f, NEWLINE);
        }
#else
      vfprintf(stdout, fmt, args);
      printf(NEWLINE);
#endif
      va_end(args);
    }
}

void gb_log_stream(const level l, const char c)
{
  if (l <= ERROR_LEVEL)
    {
      static char buf[1024];
      static unsigned int ptr = 0;

      buf[ptr] = c;

      if (c != '\n')
        {
          if (ptr < sizeof (buf) - 1)
            ++ptr;
        }
      else
        {
          printf("%.*s", ptr + 1, buf);
          ptr = 0;
        }
    }
}

void gb_log_close_file()
{
#ifdef LOG_FILE
  if (f)
    {
      fclose(f);
      f = NULL;
    }
#endif
}
#endif

void gb_log_print_rom_info(uint8_t *rom)
{
  gb_log(INFO, "ROM info:");

  switch(rom[0x0147])
    {
    case 0x00: gb_log(INFO, " - ROM ONLY"); break;
    case 0x01: gb_log(INFO, " - MBC1"); break;
    case 0x02: gb_log(INFO, " - MBC1+RAM"); break;
    case 0x03: gb_log(INFO, " - MBC1+RAM+BATTERY"); break;
    case 0x05: gb_log(INFO, " - MBC2"); break;
    case 0x06: gb_log(INFO, " - MBC2+BATTERY"); break;
    case 0x08: gb_log(INFO, " - ROM+RAM"); break;
    case 0x09: gb_log(INFO, " - ROM+RAM+BATTERY"); break;
    case 0x0B: gb_log(INFO, " - MMM01"); break;
    case 0x0C: gb_log(INFO, " - MMM01+RAM"); break;
    case 0x0D: gb_log(INFO, " - MMM01+RAM+BATTERY"); break;
    case 0x0F: gb_log(INFO, " - MBC3+TIMER+BATTERY"); break;
    case 0x10: gb_log(INFO, " - MBC3+TIMER+RAM+BATTERY"); break;
    case 0x11: gb_log(INFO, " - MBC3"); break;
    case 0x12: gb_log(INFO, " - MBC3+RAM"); break;
    case 0x13: gb_log(INFO, " - MBC3+RAM+BATTERY"); break;
    case 0x15: gb_log(INFO, " - MBC4"); break;
    case 0x16: gb_log(INFO, " - MBC4+RAM"); break;
    case 0x17: gb_log(INFO, " - MBC4+RAM+BATTERY"); break;
    case 0x19: gb_log(INFO, " - MBC5"); break;
    case 0x1A: gb_log(INFO, " - MBC5+RAM"); break;
    case 0x1B: gb_log(INFO, " - MBC5+RAM+BATTERY"); break;
    case 0x1C: gb_log(INFO, " - MBC5+RUMBLE"); break;
    case 0x1D: gb_log(INFO, " - MBC5+RUMBLE+RAM"); break;
    case 0x1E: gb_log(INFO, " - MBC5+RUMBLE+RAM+BATTERY"); break;
    case 0xFC: gb_log(INFO, " - POCKET CAMERA"); break;
    case 0xFD: gb_log(INFO, " - BANDAI TAMA5"); break;
    case 0xFE: gb_log(INFO, " - HuC3"); break;
    case 0xFF: gb_log(INFO, " - HuC1+RAM+BATTERY"); break;
    default: break;
    }

  switch(rom[0x0148])
    {
    case 0x00: gb_log(INFO, " - 32 KByte (no ROM banking)"); break;
    case 0x01: gb_log(INFO, " - 64 KByte (4 banks)"); break;
    case 0x02: gb_log(INFO, " - 128 KByte (8 banks)"); break;
    case 0x03: gb_log(INFO, " - 256 KByte (16 banks)"); break;
    case 0x04: gb_log(INFO, " - 512 KByte (32 banks)"); break;
    case 0x05: gb_log(INFO, " - 1 MByte (64 banks) - 63 banks used by MBC1"); break;
    case 0x06: gb_log(INFO, " - 2 MByte (128 banks) - 125 banks used by MBC1"); break;
    case 0x07: gb_log(INFO, " - 4 MByte (256 banks)"); break;
    case 0x52: gb_log(INFO, " - 1.1 MByte (72 banks)"); break;
    case 0x53: gb_log(INFO, " - 1.2 MByte (80 banks)"); break;
    case 0x54: gb_log(INFO, " - 1.5 MByte (96 banks)"); break;
    default: break;
  }

  switch(rom[0x0149])
    {
    case 0x00: gb_log(INFO, " - None"); break;
    case 0x01: gb_log(INFO, " - 2 KBytes"); break;
    case 0x02: gb_log(INFO, " - 8 Kbytes"); break;
    case 0x03: gb_log(INFO, " - 32 KBytes (4 banks of 8 KBytes each)"); break;
    default: break;
    }
}
