#include "loader.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t* read_file(const char* path, const bool is_rom)
{
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

      fclose (file);
    }
  else
    {
      gb_log(ERROR, "Could not open file");
    }

  return (uint8_t*)buffer;
}
