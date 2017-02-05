#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "cpu.h"
#include "mmu.h"

#include <stdint.h>

void isr_set_lcdc_isr_if_enabled(memory *mem, const uint8_t flag);

void isr_compare_ly_lyc(memory *mem, const uint8_t ly, const uint8_t lyc);

void isr_set_if_flag(memory *mem, const uint8_t flag);

#endif // INTERRUPTS_H
