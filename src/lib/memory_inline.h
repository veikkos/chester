#ifndef MEMORY_INLINE_H
#define MEMORY_INLINE_H

#include "mmu.h"

static inline uint8_t read_io_byte(memory *mem, const uint16_t address)
{
  return mem->io_registers[address & 0x00FF];
}

static inline void write_io_byte(memory *mem, const uint16_t address, const uint8_t value)
{
  mem->io_registers[address & 0x00FF] = value;
}

static inline uint8_t read_oam_byte(memory *mem, const uint16_t address)
{
  return mem->oam[address & 0x01FF];
}

#endif // MEMORY_INLINE_H
