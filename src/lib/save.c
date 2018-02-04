#include "save.h"

#include "logger.h"

#include <string.h>
#include <stdio.h>

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
  size_t file_size;

  if (save_file)
  {
    fseek(save_file, 0, SEEK_END);
    file_size = ftell(save_file);
    rewind (save_file);

    size_t result = fread(&mem->banks.ram.data, 1, sizeof mem->banks.ram.data, save_file);

    if (result != file_size)
      {
        gb_log(ERROR, "Save file read error");
        memset(mem->banks.ram.data, 0, sizeof mem->banks.ram.data);
      }

    fclose(save_file);
  }
}
