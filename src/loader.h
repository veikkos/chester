#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdbool.h>

#include "logger.h"
#include "mmu.h"

uint8_t* read_file(const char* path, uint32_t *size, const bool is_rom);

mbc get_type(uint8_t* rom);

#endif // LOADER_H
