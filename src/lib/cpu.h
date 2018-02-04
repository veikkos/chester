#ifndef CPU_H
#define CPU_H

#include "mmu.h"
#include "logger.h"

#include <stdint.h>
#include <stdbool.h>

struct registers_s {
  union {
    struct {
      uint8_t f;
      uint8_t a;
    };
    uint16_t af;
  };
  union {
    struct {
      uint8_t c;
      uint8_t b;
    };
    uint16_t bc;
  };
  union {
    struct {
      uint8_t e;
      uint8_t d;
    };
    uint16_t de;
  };
  union {
    struct {
      uint8_t l;
      uint8_t h;
    };
    uint16_t hl;
  };
  uint16_t sp, pc;
  bool ime;
  struct {
    struct {
      uint8_t t;
    } last;
    uint16_t m, t;
  } clock;
  bool halt, stop;
  uint8_t halt_mask;
  struct {
    unsigned int tick, div, t_timer;
  } timer;
};

typedef struct registers_s registers;

void cpu_reset(registers *reg);

#ifdef ENABLE_DEBUG
void cpu_debug_print(registers *reg, level l);
#else
#define cpu_debug_print(r, l) ;
#endif

int cpu_next_command(registers *reg, memory *mem);

#endif // CPU_H
