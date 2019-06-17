#include "save.h"

#include "logger.h"

#include <time.h>
#include <string.h>
#include <stdio.h>

static size_t get_file_size(FILE* file)
{
  fseek(file, 0, SEEK_END);
  const size_t file_size = ftell(file);
  rewind(file);
  return file_size;
}

void save_game(char* file_name, memory *mem)
{
  FILE *save_file = fopen(file_name, "wb");
  if (save_file)
  {
    fwrite(&mem->banks.ram.data, sizeof mem->banks.ram.data, 1, save_file);
    fclose(save_file);
  }
}

void load_game(char* file_name, memory *mem)
{
  FILE *save_file = fopen(file_name, "rb");

  if (save_file)
  {
    const size_t file_size = get_file_size(save_file);
    const size_t result = fread(&mem->banks.ram.data, 1, sizeof mem->banks.ram.data, save_file);

    if (result != file_size)
      {
        gb_log(ERROR, "Save file read error");
        memset(mem->banks.ram.data, 0, sizeof mem->banks.ram.data);
      }

    fclose(save_file);
  }
}

void save_rtc(char* file_name, memory *mem)
{
  FILE *rtc_file = fopen(file_name, "wb");
  if (rtc_file)
  {
    fwrite(&mem->rtc, sizeof mem->rtc, 1, rtc_file);
    fclose(rtc_file);
  }
}

void load_rtc(char* file_name, memory *mem)
{
  FILE *rtc_file = fopen(file_name, "rb");

  if (rtc_file)
  {
    const size_t file_size = get_file_size(rtc_file);
    const size_t result = fread(&mem->rtc, 1, sizeof mem->rtc, rtc_file);

    if (result == file_size)
      {
        const uint64_t current = time(NULL);
        mem->rtc.relative += (int32_t)(current - mem->rtc.system);
        mem->rtc.system = current;
      }
    else
      {
        gb_log(ERROR, "RTC file read error");

        memset(&mem->rtc, 0, sizeof mem->rtc);
        mem->rtc.system = time(NULL);
      }

    fclose(rtc_file);
  }
  else
  {
    mem->rtc.system = time(NULL);
  }
}
