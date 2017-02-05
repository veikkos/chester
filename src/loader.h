#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdbool.h>

#include "logger.h"

uint8_t* read_file(const char* path, const bool is_rom);

#endif // LOADER_H
