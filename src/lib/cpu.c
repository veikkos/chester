#include "cpu.h"
#include "cpu_inline.h"

#include <string.h>

void cpu_reset(registers *reg)
{
  reg->pc = 0x0100;
  reg->sp = 0xFFFE;

#ifdef CGB
  reg->af = 0x11B0;
#else
  reg->af = 0x01B0;
#endif
  reg->bc = 0x0013;
  reg->de = 0x00D8;
  reg->hl = 0x014D;

  reg->ime = true;

  reg->clock.m = 0;
  reg->clock.t = 0;

  reg->clock.last.t = 0;

  reg->halt = false;
  reg->stop = false;

  reg->timer.tick = 0;
  reg->timer.div = 0;

#ifdef CGB
  reg->speed_shifter = 0;
#endif

  memset(&reg->timer, 0, sizeof(reg->timer));
}

#ifndef NDEBUG
void cpu_debug_print(registers *reg, level l)
{
  gb_log(l, "CPU info");
  gb_log(l, " - af: %04X", reg->af);
  gb_log(l, " - bc: %04X", reg->bc);
  gb_log(l, " - de: %04X", reg->de);
  gb_log(l, " - hl: %04X", reg->hl);
  gb_log(l, " - sp: %04X", reg->sp);
  gb_log(l, " - pc: %04X", reg->pc);
  gb_log(l, " - ime: %1X", reg->ime);
  gb_log(l, "   last c: %d", reg->clock.last.t);
#ifdef DEBUG_CPU_CLOCK
  gb_log(l, " - clock");
  gb_log(l, "   - m: %u", reg->clock.m);
  gb_log(l, "   - t: %u", reg->clock.t);
#endif
  gb_log(l, " - timer");
  gb_log(l, "   - tick: %u", reg->timer.tick);
  gb_log(l, "   - div: %u", reg->timer.div);
  gb_log(l, "   - t_timer: %u", reg->timer.t_timer);
}
#endif

static inline uint8_t read_io_register(memory *mem, const uint16_t address)
{
  // Faster than mmu_read_byte
  return mem->io_registers[address & 0x00FF];
}

static inline void check_halt_release(registers *reg, memory *mem)
{
  uint8_t if_flags = read_io_register(mem, MEM_IF_ADDR);
  if_flags &= mem->ie_register;

  if (if_flags)
    {
      reg->halt = false;
    }
}

static inline void check_isr(registers *reg, memory *mem)
{
  uint8_t if_flags = read_io_register(mem, MEM_IF_ADDR);
  if_flags &= mem->ie_register;

  if (if_flags)
    {
      if (if_flags & MEM_IF_VBLANK_FLAG)
        {
          gb_log (VERBOSE, "VBLANK ISR");

          if_flags &= ~MEM_IF_VBLANK_FLAG;
          mmu_write_byte(mem, MEM_IF_ADDR, if_flags);

          jump_to_isr_address(reg, mem, MEM_VBLANK_ISR_ADDR);
        }
      else if (if_flags & MEM_IF_LCDC_FLAG)
        {
          gb_log (VERBOSE, "LCD ISR");

          if_flags &= ~MEM_IF_LCDC_FLAG;
          mmu_write_byte(mem, MEM_IF_ADDR, if_flags);

          jump_to_isr_address(reg, mem, MEM_LCD_ISR_ADDR);
        }
      else if (if_flags & MEM_IF_TIMER_OVF_FLAG)
        {
          gb_log (VERBOSE, "TIMER ISR");

          if_flags &= ~MEM_IF_TIMER_OVF_FLAG;
          mmu_write_byte(mem, MEM_IF_ADDR, if_flags);

          jump_to_isr_address(reg, mem, MEM_TIMER_ISR_ADDR);
        }
      else if (if_flags & MEM_IF_PIN_FLAG)
        {
          gb_log (VERBOSE, "INPUT ISR");

          if_flags &= ~MEM_IF_PIN_FLAG;
          mmu_write_byte(mem, MEM_IF_ADDR, if_flags);

          jump_to_isr_address(reg, mem, MEM_PIN_ISR_ADDR);
        }
    }
}

static inline int sub_command(registers *reg, memory *mem)
{
  const uint8_t sub_op = mmu_read_byte(mem, reg->pc++);
  int i;

  switch(sub_op)
    {
    case 0x47:
      gb_log(ALL, "BIT 0, A");
      bit_b_r(reg, reg->a, 0);
      break;
    case 0x4F:
      gb_log(ALL, "BIT 1, A");
      bit_b_r(reg, reg->a, 1);
      break;
    case 0x57:
      gb_log(ALL, "BIT 2, A");
      bit_b_r(reg, reg->a, 2);
      break;
    case 0x5F:
      gb_log(ALL, "BIT 3, A");
      bit_b_r(reg, reg->a, 3);
      break;
    case 0x67:
      gb_log(ALL, "BIT 4, A");
      bit_b_r(reg, reg->a, 4);
      break;
    case 0x6F:
      gb_log(ALL, "BIT 5, A");
      bit_b_r(reg, reg->a, 5);
      break;
    case 0x77:
      gb_log(ALL, "BIT 6, A");
      bit_b_r(reg, reg->a, 6);
      break;
    case 0x7F:
      gb_log(ALL, "BIT 7, A");
      bit_b_r(reg, reg->a, 7);
      break;
    case 0x40:
      gb_log(ALL, "BIT 0, B");
      bit_b_r(reg, reg->b, 0);
      break;
    case 0x48:
      gb_log(ALL, "BIT 1, B");
      bit_b_r(reg, reg->b, 1);
      break;
    case 0x50:
      gb_log(ALL, "BIT 2, B");
      bit_b_r(reg, reg->b, 2);
      break;
    case 0x58:
      gb_log(ALL, "BIT 3, B");
      bit_b_r(reg, reg->b, 3);
      break;
    case 0x60:
      gb_log(ALL, "BIT 4, B");
      bit_b_r(reg, reg->b, 4);
      break;
    case 0x68:
      gb_log(ALL, "BIT 5, B");
      bit_b_r(reg, reg->b, 5);
      break;
    case 0x70:
      gb_log(ALL, "BIT 6, B");
      bit_b_r(reg, reg->b, 6);
      break;
    case 0x78:
      gb_log(ALL, "BIT 7, B");
      bit_b_r(reg, reg->b, 7);
      break;
    case 0x41:
      gb_log(ALL, "BIT 0, C");
      bit_b_r(reg, reg->c, 0);
      break;
    case 0x49:
      gb_log(ALL, "BIT 1, C");
      bit_b_r(reg, reg->c, 1);
      break;
    case 0x51:
      gb_log(ALL, "BIT 2, C");
      bit_b_r(reg, reg->c, 2);
      break;
    case 0x59:
      gb_log(ALL, "BIT 3, C");
      bit_b_r(reg, reg->c, 3);
      break;
    case 0x61:
      gb_log(ALL, "BIT 4, C");
      bit_b_r(reg, reg->c, 4);
      break;
    case 0x69:
      gb_log(ALL, "BIT 5, C");
      bit_b_r(reg, reg->c, 5);
      break;
    case 0x71:
      gb_log(ALL, "BIT 6, C");
      bit_b_r(reg, reg->c, 6);
      break;
    case 0x79:
      gb_log(ALL, "BIT 7, C");
      bit_b_r(reg, reg->c, 7);
      break;
    case 0x42:
      gb_log(ALL, "BIT 0, D");
      bit_b_r(reg, reg->d, 0);
      break;
    case 0x4A:
      gb_log(ALL, "BIT 1, D");
      bit_b_r(reg, reg->d, 1);
      break;
    case 0x52:
      gb_log(ALL, "BIT 2, D");
      bit_b_r(reg, reg->d, 2);
      break;
    case 0x5A:
      gb_log(ALL, "BIT 3, D");
      bit_b_r(reg, reg->d, 3);
      break;
    case 0x62:
      gb_log(ALL, "BIT 4, D");
      bit_b_r(reg, reg->d, 4);
      break;
    case 0x6A:
      gb_log(ALL, "BIT 5, D");
      bit_b_r(reg, reg->d, 5);
      break;
    case 0x72:
      gb_log(ALL, "BIT 6, D");
      bit_b_r(reg, reg->d, 6);
      break;
    case 0x7A:
      gb_log(ALL, "BIT 7 D");
      bit_b_r(reg, reg->d, 7);
      break;
    case 0x43:
      gb_log(ALL, "BIT 0, E");
      bit_b_r(reg, reg->e, 0);
      break;
    case 0x4B:
      gb_log(ALL, "BIT 1, E");
      bit_b_r(reg, reg->e, 1);
      break;
    case 0x53:
      gb_log(ALL, "BIT 2, E");
      bit_b_r(reg, reg->e, 2);
      break;
    case 0x5B:
      gb_log(ALL, "BIT 3, E");
      bit_b_r(reg, reg->e, 3);
      break;
    case 0x63:
      gb_log(ALL, "BIT 4, E");
      bit_b_r(reg, reg->e, 4);
      break;
    case 0x6B:
      gb_log(ALL, "BIT 5, E");
      bit_b_r(reg, reg->e, 5);
      break;
    case 0x73:
      gb_log(ALL, "BIT 6, E");
      bit_b_r(reg, reg->e, 6);
      break;
    case 0x7B:
      gb_log(ALL, "BIT 7 E");
      bit_b_r(reg, reg->e, 7);
      break;
    case 0x44:
      gb_log(ALL, "BIT 0, H");
      bit_b_r(reg, reg->h, 0);
      break;
    case 0x4C:
      gb_log(ALL, "BIT 1, H");
      bit_b_r(reg, reg->h, 1);
      break;
    case 0x54:
      gb_log(ALL, "BIT 2, H");
      bit_b_r(reg, reg->h, 2);
      break;
    case 0x5C:
      gb_log(ALL, "BIT 3, H");
      bit_b_r(reg, reg->h, 3);
      break;
    case 0x64:
      gb_log(ALL, "BIT 4, H");
      bit_b_r(reg, reg->h, 4);
      break;
    case 0x6C:
      gb_log(ALL, "BIT 5, H");
      bit_b_r(reg, reg->h, 5);
      break;
    case 0x74:
      gb_log(ALL, "BIT 6, H");
      bit_b_r(reg, reg->h, 6);
      break;
    case 0x7C:
      gb_log(ALL, "BIT 7 H");
      bit_b_r(reg, reg->h, 7);
      break;
    case 0x45:
      gb_log(ALL, "BIT 0, L");
      bit_b_r(reg, reg->l, 0);
      break;
    case 0x4D:
      gb_log(ALL, "BIT 1, L");
      bit_b_r(reg, reg->l, 1);
      break;
    case 0x55:
      gb_log(ALL, "BIT 2, L");
      bit_b_r(reg, reg->l, 2);
      break;
    case 0x5D:
      gb_log(ALL, "BIT 3, L");
      bit_b_r(reg, reg->l, 3);
      break;
    case 0x65:
      gb_log(ALL, "BIT 4, L");
      bit_b_r(reg, reg->l, 4);
      break;
    case 0x6D:
      gb_log(ALL, "BIT 5, L");
      bit_b_r(reg, reg->l, 5);
      break;
    case 0x75:
      gb_log(ALL, "BIT 6, L");
      bit_b_r(reg, reg->l, 6);
      break;
    case 0x7D:
      gb_log(ALL, "BIT 7 L");
      bit_b_r(reg, reg->l, 7);
      break;
    case 0x46:
      gb_log(ALL, "BIT 0, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 0);
      reg->clock.last.t += 4;
      break;
    case 0x4E:
      gb_log(ALL, "BIT 1, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 1);
      reg->clock.last.t += 4;
      break;
    case 0x56:
      gb_log(ALL, "BIT 2, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 2);
      reg->clock.last.t += 4;
      break;
    case 0x5E:
      gb_log(ALL, "BIT 3, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 3);
      reg->clock.last.t += 4;
      break;
    case 0x66:
      gb_log(ALL, "BIT 4, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 4);
      reg->clock.last.t += 4;
      break;
    case 0x6E:
      gb_log(ALL, "BIT 5, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 5);
      reg->clock.last.t += 4;
      break;
    case 0x76:
      gb_log(ALL, "BIT 6, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 6);
      reg->clock.last.t += 4;
      break;
    case 0x7E:
      gb_log(ALL, "BIT 7, (HL)");
      bit_b_r(reg, mmu_read_byte(mem, reg->hl), 7);
      reg->clock.last.t += 4;
      break;
    case 0x37:
      gb_log(ALL, "SWAP A");
      swap_n(reg, &reg->a);
      break;
    case 0x30:
      gb_log(ALL, "SWAP B");
      swap_n(reg, &reg->b);
      break;
    case 0x31:
      gb_log(ALL, "SWAP C");
      swap_n(reg, &reg->c);
      break;
    case 0x32:
      gb_log(ALL, "SWAP D");
      swap_n(reg, &reg->d);
      break;
    case 0x33:
      gb_log(ALL, "SWAP E");
      swap_n(reg, &reg->e);
      break;
    case 0x34:
      gb_log(ALL, "SWAP H");
      swap_n(reg, &reg->h);
      break;
    case 0x35:
      gb_log(ALL, "SWAP L");
      swap_n(reg, &reg->l);
      break;
    case 0x36:
      gb_log(ALL, "SWAP (HL)");
      swap_hl(reg, mem);
      break;
    case 0xC7:
      gb_log(ALL, "SET 0, A");
      set_b_r(reg, &reg->a, 0);
      break;
    case 0xCF:
      gb_log(ALL, "SET 1, A");
      set_b_r(reg, &reg->a, 1);
      break;
    case 0xD7:
      gb_log(ALL, "SET 2, A");
      set_b_r(reg, &reg->a, 2);
      break;
    case 0xDF:
      gb_log(ALL, "SET 3, A");
      set_b_r(reg, &reg->a, 3);
      break;
    case 0xE7:
      gb_log(ALL, "SET 4, A");
      set_b_r(reg, &reg->a, 4);
      break;
    case 0xEF:
      gb_log(ALL, "SET 5, A");
      set_b_r(reg, &reg->a, 5);
      break;
    case 0xF7:
      gb_log(ALL, "SET 6, A");
      set_b_r(reg, &reg->a, 6);
      break;
    case 0xFF:
      gb_log(ALL, "SET 7, A");
      set_b_r(reg, &reg->a, 7);
      break;
    case 0xC0:
      gb_log(ALL, "SET 0, B");
      set_b_r(reg, &reg->b, 0);
      break;
    case 0xC8:
      gb_log(ALL, "SET 1, B");
      set_b_r(reg, &reg->b, 1);
      break;
    case 0xD0:
      gb_log(ALL, "SET 2, B");
      set_b_r(reg, &reg->b, 2);
      break;
    case 0xD8:
      gb_log(ALL, "SET 3, B");
      set_b_r(reg, &reg->b, 3);
      break;
    case 0xE0:
      gb_log(ALL, "SET 4, B");
      set_b_r(reg, &reg->b, 4);
      break;
    case 0xE8:
      gb_log(ALL, "SET 5, B");
      set_b_r(reg, &reg->b, 5);
      break;
    case 0xF0:
      gb_log(ALL, "SET 6, B");
      set_b_r(reg, &reg->b, 6);
      break;
    case 0xF8:
      gb_log(ALL, "SET 7, B");
      set_b_r(reg, &reg->b, 7);
      break;
    case 0xC1:
      gb_log(ALL, "SET 0, C");
      set_b_r(reg, &reg->c, 0);
      break;
    case 0xC9:
      gb_log(ALL, "SET 1, C");
      set_b_r(reg, &reg->c, 1);
      break;
    case 0xD1:
      gb_log(ALL, "SET 2, C");
      set_b_r(reg, &reg->c, 2);
      break;
    case 0xD9:
      gb_log(ALL, "SET 3, C");
      set_b_r(reg, &reg->c, 3);
      break;
    case 0xE1:
      gb_log(ALL, "SET 4, C");
      set_b_r(reg, &reg->c, 4);
      break;
    case 0xE9:
      gb_log(ALL, "SET 5, C");
      set_b_r(reg, &reg->c, 5);
      break;
    case 0xF1:
      gb_log(ALL, "SET 6, C");
      set_b_r(reg, &reg->c, 6);
      break;
    case 0xF9:
      gb_log(ALL, "SET 7, C");
      set_b_r(reg, &reg->c, 7);
      break;
    case 0xC2:
      gb_log(ALL, "SET 0, D");
      set_b_r(reg, &reg->d, 0);
      break;
    case 0xCA:
      gb_log(ALL, "SET 1, D");
      set_b_r(reg, &reg->d, 1);
      break;
    case 0xD2:
      gb_log(ALL, "SET 2, D");
      set_b_r(reg, &reg->d, 2);
      break;
    case 0xDA:
      gb_log(ALL, "SET 3, D");
      set_b_r(reg, &reg->d, 3);
      break;
    case 0xE2:
      gb_log(ALL, "SET 4, D");
      set_b_r(reg, &reg->d, 4);
      break;
    case 0xEA:
      gb_log(ALL, "SET 5, D");
      set_b_r(reg, &reg->d, 5);
      break;
    case 0xF2:
      gb_log(ALL, "SET 6, D");
      set_b_r(reg, &reg->d, 6);
      break;
    case 0xFA:
      gb_log(ALL, "SET 7, D");
      set_b_r(reg, &reg->d, 7);
      break;
    case 0xC3:
      gb_log(ALL, "SET 0, E");
      set_b_r(reg, &reg->e, 0);
      break;
    case 0xCB:
      gb_log(ALL, "SET 1, E");
      set_b_r(reg, &reg->e, 1);
      break;
    case 0xD3:
      gb_log(ALL, "SET 2, E");
      set_b_r(reg, &reg->e, 2);
      break;
    case 0xDB:
      gb_log(ALL, "SET 3, E");
      set_b_r(reg, &reg->e, 3);
      break;
    case 0xE3:
      gb_log(ALL, "SET 4, E");
      set_b_r(reg, &reg->e, 4);
      break;
    case 0xEB:
      gb_log(ALL, "SET 5, E");
      set_b_r(reg, &reg->e, 5);
      break;
    case 0xF3:
      gb_log(ALL, "SET 6, E");
      set_b_r(reg, &reg->e, 6);
      break;
    case 0xFB:
      gb_log(ALL, "SET 7, E");
      set_b_r(reg, &reg->e, 7);
      break;
    case 0xC4:
      gb_log(ALL, "SET 0, H");
      set_b_r(reg, &reg->h, 0);
      break;
    case 0xCC:
      gb_log(ALL, "SET 1, H");
      set_b_r(reg, &reg->h, 1);
      break;
    case 0xD4:
      gb_log(ALL, "SET 2, H");
      set_b_r(reg, &reg->h, 2);
      break;
    case 0xDC:
      gb_log(ALL, "SET 3, H");
      set_b_r(reg, &reg->h, 3);
      break;
    case 0xE4:
      gb_log(ALL, "SET 4, H");
      set_b_r(reg, &reg->h, 4);
      break;
    case 0xEC:
      gb_log(ALL, "SET 5, H");
      set_b_r(reg, &reg->h, 5);
      break;
    case 0xF4:
      gb_log(ALL, "SET 6, H");
      set_b_r(reg, &reg->h, 6);
      break;
    case 0xFC:
      gb_log(ALL, "SET 7, H");
      set_b_r(reg, &reg->h, 7);
      break;
    case 0xC5:
      gb_log(ALL, "SET 0, L");
      set_b_r(reg, &reg->l, 0);
      break;
    case 0xCD:
      gb_log(ALL, "SET 1, L");
      set_b_r(reg, &reg->l, 1);
      break;
    case 0xD5:
      gb_log(ALL, "SET 2, L");
      set_b_r(reg, &reg->l, 2);
      break;
    case 0xDD:
      gb_log(ALL, "SET 3, L");
      set_b_r(reg, &reg->l, 3);
      break;
    case 0xE5:
      gb_log(ALL, "SET 4, L");
      set_b_r(reg, &reg->l, 4);
      break;
    case 0xED:
      gb_log(ALL, "SET 5, L");
      set_b_r(reg, &reg->l, 5);
      break;
    case 0xF5:
      gb_log(ALL, "SET 6, L");
      set_b_r(reg, &reg->l, 6);
      break;
    case 0xFD:
      gb_log(ALL, "SET 7, L");
      set_b_r(reg, &reg->l, 7);
      break;
    case 0xC6:
      gb_log(ALL, "SET 0, (HL)");
      set_b_hl(reg, mem, 0);
      break;
    case 0xCE:
      gb_log(ALL, "SET 1, (HL)");
      set_b_hl(reg, mem, 1);
      break;
    case 0xD6:
      gb_log(ALL, "SET 2, (HL)");
      set_b_hl(reg, mem, 2);
      break;
    case 0xDE:
      gb_log(ALL, "SET 3, (HL)");
      set_b_hl(reg, mem, 3);
      break;
    case 0xE6:
      gb_log(ALL, "SET 4, (HL)");
      set_b_hl(reg, mem, 4);
      break;
    case 0xEE:
      gb_log(ALL, "SET 5, (HL)");
      set_b_hl(reg, mem, 5);
      break;
    case 0xF6:
      gb_log(ALL, "SET 6, (HL)");
      set_b_hl(reg, mem, 6);
      break;
    case 0xFE:
      gb_log(ALL, "SET 7, (HL)");
      set_b_hl(reg, mem, 7);
      break;
    case 0x87:
      gb_log(ALL, "RES 0, A");
      res_b_r(reg, &reg->a, 0);
      break;
    case 0x8F:
      gb_log(ALL, "RES 1, A");
      res_b_r(reg, &reg->a, 1);
      break;
    case 0x97:
      gb_log(ALL, "RES 2, A");
      res_b_r(reg, &reg->a, 2);
      break;
    case 0x9F:
      gb_log(ALL, "RES 3, A");
      res_b_r(reg, &reg->a, 3);
      break;
    case 0xA7:
      gb_log(ALL, "RES 4, A");
      res_b_r(reg, &reg->a, 4);
      break;
    case 0xAF:
      gb_log(ALL, "RES 5, A");
      res_b_r(reg, &reg->a, 5);
      break;
    case 0xB7:
      gb_log(ALL, "RES 6, A");
      res_b_r(reg, &reg->a, 6);
      break;
    case 0xBF:
      gb_log(ALL, "RES 7, A");
      res_b_r(reg, &reg->a, 7);
      break;
    case 0x80:
      gb_log(ALL, "RES 0, B");
      res_b_r(reg, &reg->b, 0);
      break;
    case 0x88:
      gb_log(ALL, "RES 1, B");
      res_b_r(reg, &reg->b, 1);
      break;
    case 0x90:
      gb_log(ALL, "RES 2, B");
      res_b_r(reg, &reg->b, 2);
      break;
    case 0x98:
      gb_log(ALL, "RES 3, B");
      res_b_r(reg, &reg->b, 3);
      break;
    case 0xA0:
      gb_log(ALL, "RES 4, B");
      res_b_r(reg, &reg->b, 4);
      break;
    case 0xA8:
      gb_log(ALL, "RES 5, B");
      res_b_r(reg, &reg->b, 5);
      break;
    case 0xB0:
      gb_log(ALL, "RES 6, B");
      res_b_r(reg, &reg->b, 6);
      break;
    case 0xB8:
      gb_log(ALL, "RES 7, B");
      res_b_r(reg, &reg->b, 7);
      break;
    case 0x81:
      gb_log(ALL, "RES 0, C");
      res_b_r(reg, &reg->c, 0);
      break;
    case 0x89:
      gb_log(ALL, "RES 1, C");
      res_b_r(reg, &reg->c, 1);
      break;
    case 0x91:
      gb_log(ALL, "RES 2, C");
      res_b_r(reg, &reg->c, 2);
      break;
    case 0x99:
      gb_log(ALL, "RES 3, C");
      res_b_r(reg, &reg->c, 3);
      break;
    case 0xA1:
      gb_log(ALL, "RES 4, C");
      res_b_r(reg, &reg->c, 4);
      break;
    case 0xA9:
      gb_log(ALL, "RES 5, C");
      res_b_r(reg, &reg->c, 5);
      break;
    case 0xB1:
      gb_log(ALL, "RES 6, C");
      res_b_r(reg, &reg->c, 6);
      break;
    case 0xB9:
      gb_log(ALL, "RES 7, C");
      res_b_r(reg, &reg->c, 7);
      break;
    case 0x82:
      gb_log(ALL, "RES 0, D");
      res_b_r(reg, &reg->d, 0);
      break;
    case 0x8A:
      gb_log(ALL, "RES 1, D");
      res_b_r(reg, &reg->d, 1);
      break;
    case 0x92:
      gb_log(ALL, "RES 2, D");
      res_b_r(reg, &reg->d, 2);
      break;
    case 0x9A:
      gb_log(ALL, "RES 3, D");
      res_b_r(reg, &reg->d, 3);
      break;
    case 0xA2:
      gb_log(ALL, "RES 4, D");
      res_b_r(reg, &reg->d, 4);
      break;
    case 0xAA:
      gb_log(ALL, "RES 5, D");
      res_b_r(reg, &reg->d, 5);
      break;
    case 0xB2:
      gb_log(ALL, "RES 6, D");
      res_b_r(reg, &reg->d, 6);
      break;
    case 0xBA:
      gb_log(ALL, "RES 7, D");
      res_b_r(reg, &reg->d, 7);
      break;
    case 0x83:
      gb_log(ALL, "RES 0, E");
      res_b_r(reg, &reg->e, 0);
      break;
    case 0x8B:
      gb_log(ALL, "RES 1, E");
      res_b_r(reg, &reg->e, 1);
      break;
    case 0x93:
      gb_log(ALL, "RES 2, E");
      res_b_r(reg, &reg->e, 2);
      break;
    case 0x9B:
      gb_log(ALL, "RES 3, E");
      res_b_r(reg, &reg->e, 3);
      break;
    case 0xA3:
      gb_log(ALL, "RES 4, E");
      res_b_r(reg, &reg->e, 4);
      break;
    case 0xAB:
      gb_log(ALL, "RES 5, E");
      res_b_r(reg, &reg->e, 5);
      break;
    case 0xB3:
      gb_log(ALL, "RES 6, E");
      res_b_r(reg, &reg->e, 6);
      break;
    case 0xBB:
      gb_log(ALL, "RES 7, E");
      res_b_r(reg, &reg->e, 7);
      break;
    case 0x84:
      gb_log(ALL, "RES 0, H");
      res_b_r(reg, &reg->h, 0);
      break;
    case 0x8C:
      gb_log(ALL, "RES 1, H");
      res_b_r(reg, &reg->h, 1);
      break;
    case 0x94:
      gb_log(ALL, "RES 2, H");
      res_b_r(reg, &reg->h, 2);
      break;
    case 0x9C:
      gb_log(ALL, "RES 3, H");
      res_b_r(reg, &reg->h, 3);
      break;
    case 0xA4:
      gb_log(ALL, "RES 4, H");
      res_b_r(reg, &reg->h, 4);
      break;
    case 0xAC:
      gb_log(ALL, "RES 5, H");
      res_b_r(reg, &reg->h, 5);
      break;
    case 0xB4:
      gb_log(ALL, "RES 6, H");
      res_b_r(reg, &reg->h, 6);
      break;
    case 0xBC:
      gb_log(ALL, "RES 7, H");
      res_b_r(reg, &reg->h, 7);
      break;
    case 0x85:
      gb_log(ALL, "RES 0, L");
      res_b_r(reg, &reg->l, 0);
      break;
    case 0x8D:
      gb_log(ALL, "RES 1, L");
      res_b_r(reg, &reg->l, 1);
      break;
    case 0x95:
      gb_log(ALL, "RES 2, L");
      res_b_r(reg, &reg->l, 2);
      break;
    case 0x9D:
      gb_log(ALL, "RES 3, L");
      res_b_r(reg, &reg->l, 3);
      break;
    case 0xA5:
      gb_log(ALL, "RES 4, L");
      res_b_r(reg, &reg->l, 4);
      break;
    case 0xAD:
      gb_log(ALL, "RES 5, L");
      res_b_r(reg, &reg->l, 5);
      break;
    case 0xB5:
      gb_log(ALL, "RES 6, L");
      res_b_r(reg, &reg->l, 6);
      break;
    case 0xBD:
      gb_log(ALL, "RES 7, L");
      res_b_r(reg, &reg->l, 7);
      break;
    case 0x86:
      gb_log(ALL, "RES 0, (HL)");
      res_b_hl(reg, mem, 0);
      break;
    case 0x8E:
      gb_log(ALL, "RES 1, (HL)");
      res_b_hl(reg, mem, 1);
      break;
    case 0x96:
      gb_log(ALL, "RES 2, (HL)");
      res_b_hl(reg, mem, 2);
      break;
    case 0x9E:
      gb_log(ALL, "RES 3, (HL)");
      res_b_hl(reg, mem, 3);
      break;
    case 0xA6:
      gb_log(ALL, "RES 4, (HL)");
      res_b_hl(reg, mem, 4);
      break;
    case 0xAE:
      gb_log(ALL, "RES 5, (HL)");
      res_b_hl(reg, mem, 5);
      break;
    case 0xB6:
      gb_log(ALL, "RES 6, (HL)");
      res_b_hl(reg, mem, 6);
      break;
    case 0xBE:
      gb_log(ALL, "RES 7, (HL)");
      res_b_hl(reg, mem, 7);
      break;
    case 0x27:
      gb_log(ALL, "SLA A");
      sla_n(reg, &reg->a);
      break;
    case 0x20:
      gb_log(ALL, "SLA B");
      sla_n(reg, &reg->b);
      break;
    case 0x21:
      gb_log(ALL, "SLA C");
      sla_n(reg, &reg->c);
      break;
    case 0x22:
      gb_log(ALL, "SLA D");
      sla_n(reg, &reg->d);
      break;
    case 0x23:
      gb_log(ALL, "SLA E");
      sla_n(reg, &reg->e);
      break;
    case 0x24:
      gb_log(ALL, "SLA H");
      sla_n(reg, &reg->h);
      break;
    case 0x25:
      gb_log(ALL, "SLA L");
      sla_n(reg, &reg->l);
      break;
    case 0x3F:
      gb_log(ALL, "SRL A");
      srl_n(reg, &reg->a);
      break;
    case 0x38:
      gb_log(ALL, "SRL B");
      srl_n(reg, &reg->b);
      break;
    case 0x39:
      gb_log(ALL, "SRL C");
      srl_n(reg, &reg->c);
      break;
    case 0x3A:
      gb_log(ALL, "SRL D");
      srl_n(reg, &reg->d);
      break;
    case 0x3B:
      gb_log(ALL, "SRL E");
      srl_n(reg, &reg->e);
      break;
    case 0x3C:
      gb_log(ALL, "SRL H");
      srl_n(reg, &reg->h);
      break;
    case 0x3D:
      gb_log(ALL, "SRL L");
      srl_n(reg, &reg->l);
      break;
    case 0x3E:
      gb_log(ALL, "SRL (HL)");
      srl_hl(reg, mem);
      break;
    case 0x2F:
      gb_log(ALL, "SRA A");
      sra_n(reg, &reg->a);
      break;
    case 0x28:
      gb_log(ALL, "SRA B");
      sra_n(reg, &reg->b);
      break;
    case 0x29:
      gb_log(ALL, "SRA C");
      sra_n(reg, &reg->c);
      break;
    case 0x2A:
      gb_log(ALL, "SRA D");
      sra_n(reg, &reg->d);
      break;
    case 0x2B:
      gb_log(ALL, "SRA E");
      sra_n(reg, &reg->e);
      break;
    case 0x2C:
      gb_log(ALL, "SRA H");
      sra_n(reg, &reg->h);
      break;
    case 0x2D:
      gb_log(ALL, "SRA L");
      sra_n(reg, &reg->l);
      break;
    case 0x2E:
      gb_log(ALL, "SRA (HL)");
      sra_hl(reg, mem);
      break;
    case 0x1F:
      gb_log(ALL, "RR A");
      rr_n(reg, &reg->a);
      break;
    case 0x18:
      gb_log(ALL, "RR B");
      rr_n(reg, &reg->b);
      break;
    case 0x19:
      gb_log(ALL, "RR C");
      rr_n(reg, &reg->c);
      break;
    case 0x1A:
      gb_log(ALL, "RR D");
      rr_n(reg, &reg->d);
      break;
    case 0x1B:
      gb_log(ALL, "RR E");
      rr_n(reg, &reg->e);
      break;
    case 0x1C:
      gb_log(ALL, "RR H");
      rr_n(reg, &reg->h);
      break;
    case 0x1D:
      gb_log(ALL, "RR L");
      rr_n(reg, &reg->l);
      break;
    case 0x17:
      gb_log(ALL, "RL A");
      rl_n(reg, &reg->a);
      break;
    case 0x10:
      gb_log(ALL, "RL B");
      rl_n(reg, &reg->b);
      break;
    case 0x11:
      gb_log(ALL, "RL C");
      rl_n(reg, &reg->c);
      break;
    case 0x12:
      gb_log(ALL, "RL D");
      rl_n(reg, &reg->d);
      break;
    case 0x13:
      gb_log(ALL, "RL E");
      rl_n(reg, &reg->e);
      break;
    case 0x14:
      gb_log(ALL, "RL H");
      rl_n(reg, &reg->h);
      break;
    case 0x15:
      gb_log(ALL, "RL L");
      rl_n(reg, &reg->l);
      break;
    case 0x07:
      gb_log(ALL, "RLC A");
      rlc_n(reg, &reg->a);
      reg->clock.last.t += 4;
      break;
    case 0x00:
      gb_log(ALL, "RLC B");
      rlc_n(reg, &reg->b);
      reg->clock.last.t += 4;
      break;
    case 0x01:
      gb_log(ALL, "RLC C");
      rlc_n(reg, &reg->c);
      reg->clock.last.t += 4;
      break;
    case 0x02:
      gb_log(ALL, "RLC D");
      rlc_n(reg, &reg->d);
      reg->clock.last.t += 4;
      break;
    case 0x03:
      gb_log(ALL, "RLC E");
      rlc_n(reg, &reg->e);
      reg->clock.last.t += 4;
      break;
    case 0x04:
      gb_log(ALL, "RLC H");
      rlc_n(reg, &reg->h);
      reg->clock.last.t += 4;
      break;
    case 0x05:
      gb_log(ALL, "RLC L");
      rlc_n(reg, &reg->l);
      reg->clock.last.t += 4;
      break;
    case 0x06:
      gb_log(ALL, "RLC (HL)");
      rlc_hl(reg, mem);
      break;
    case 0x16:
      gb_log(ALL, "RL (HL)");
      rl_hl(reg, mem);
      break;
    case 0x1E:
      gb_log(ALL, "RR (HL)");
      rr_hl(reg, mem);
      break;
    case 0x26:
      gb_log(ALL, "SRA (HL)");
      sla_hl(reg, mem);
      break;
    case 0x0F:
      gb_log(ALL, "RRC A");
      rrc_n(reg, &reg->a);
      reg->clock.last.t += 4;
      break;
    case 0x08:
      gb_log(ALL, "RRC B");
      rrc_n(reg, &reg->b);
      reg->clock.last.t += 4;
      break;
    case 0x09:
      gb_log(ALL, "RRC C");
      rrc_n(reg, &reg->c);
      reg->clock.last.t += 4;
      break;
    case 0x0A:
      gb_log(ALL, "RRC D");
      rrc_n(reg, &reg->d);
      reg->clock.last.t += 4;
      break;
    case 0x0B:
      gb_log(ALL, "RRC E");
      rrc_n(reg, &reg->e);
      reg->clock.last.t += 4;
      break;
    case 0x0C:
      gb_log(ALL, "RRC H");
      rrc_n(reg, &reg->h);
      reg->clock.last.t += 4;
      break;
    case 0x0D:
      gb_log(ALL, "RRC L");
      rrc_n(reg, &reg->l);
      reg->clock.last.t += 4;
      break;
    case 0x0E:
      gb_log(ALL, "RRC (HL)");
      rrc_hl(reg, mem);
      break;
    default:
      gb_log(ERROR, "Unknown sub command: %02X", sub_op);
      gb_log(ERROR, "Following bytes:");
      for (i=0; i<5; ++i)
        {
          gb_log(ERROR, " - %02X",
                 mmu_read_byte(mem, reg->pc + i));
        }

      return 1;
    }

  return 0;
}

int cpu_next_command(registers *reg, memory *mem)
{
  uint8_t op;
  int i;

  if (reg->stop)
    {
      reg->clock.last.t = 4;

#ifdef CGB
      reg->clock.last.t >>= reg->speed_shifter;
#endif

      // CPU speed switch check
      uint8_t *key1 = &mem->high_empty[MEM_KEY1_ADDR - MEM_HIGH_EMPTY_START_ADDR];
      if (*key1 & MEM_KEY1_PREPARE_SPEED_SWITCH_BIT)
        {
          // Clear switch bit
          *key1 &= ~MEM_KEY1_PREPARE_SPEED_SWITCH_BIT;

#ifdef CGB
          // Toggle mode from Normal to Double or vice versa
          *key1 ^= MEM_KEY1_MODE_BIT;

          // Cache mode
          reg->speed_shifter = *key1 & MEM_KEY1_MODE_BIT ? 1 : 0;
#endif

          reg->stop = false;
        }

      return 0;
    }

  if (reg->halt)
    {
      reg->clock.last.t = 4;
#if CGB
      reg->clock.last.t >>= reg->speed_shifter;
#endif
      goto isr_handling;
    }

  op = mmu_read_byte(mem, reg->pc++);

  switch (op)
    {
    case 0x00:
      gb_log(ALL, "NOP");
      nop(reg);
      break;
    case 0x76:
      gb_log(ALL, "HALT");
      halt(reg, mem);
      break;
    case 0x10:
      gb_log(ALL, "STOP");
      stop(reg);
      break;
    case 0x06:
      gb_log(ALL, "LD B, n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->b);
      break;
    case 0x0E:
      gb_log(ALL, "LD C, n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->c);
      break;
    case 0x16:
      gb_log(ALL, "LD D n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->d);
      break;
    case 0x1E:
      gb_log(ALL, "LD E, n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->e);
      break;
    case 0x26:
      gb_log(ALL, "LD H, n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->h);
      break;
    case 0x2E:
      gb_log(ALL, "LD l, n");
      ld_nn_n(reg, mmu_read_byte(mem, reg->pc++), &reg->l);
      break;
    case 0xC3:
      gb_log(ALL, "JP nn");
      jp_nn(reg, mmu_read_word(mem, reg->pc));
      break;
    case 0xE9:
      gb_log(ALL, "JP (HL)");
      jp_hl(reg);
      break;
    case 0x18:
      gb_log(ALL, "JR n");
      jr_n(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xF3:
      gb_log(ALL, "DI");
      di(reg);
      break;
    case 0xFB:
      gb_log(ALL, "EI");
      ei(reg);
      break;
    case 0x0F:
      gb_log(ALL, "RRC A");
      rrca(reg);
      break;
    case 0x07:
      gb_log(ALL, "RLC A");
      rlca(reg);
      break;
    case 0xE0:
      gb_log(ALL, "LDH (n) A");
      ldh_n_a(reg, mem, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x01:
      gb_log(ALL, "LD BC, nn");
      ldh_bc_nn(reg, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x11:
      gb_log(ALL, "LD DE, nn");
      ldh_de_nn(reg, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x21:
      gb_log(ALL, "LD HL, nn");
      ldh_hl_nn(reg, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x31:
      gb_log(ALL, "LD SP, nn");
      ldh_sp_nn(reg, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x7F:
      gb_log(ALL, "LD A, A");
      ld_r1_r2(reg, &reg->a, reg->a);
      break;
    case 0x78:
      gb_log(ALL, "LD A, B");
      ld_r1_r2(reg, &reg->a, reg->b);
      break;
    case 0x79:
      gb_log(ALL, "LD A, C");
      ld_r1_r2(reg, &reg->a, reg->c);
      break;
    case 0x7A:
      gb_log(ALL, "LD A, D");
      ld_r1_r2(reg, &reg->a, reg->d);
      break;
    case 0x7B:
      gb_log(ALL, "LD A, E");
      ld_r1_r2(reg, &reg->a, reg->e);
      break;
    case 0x7C:
      gb_log(ALL, "LD A, H");
      ld_r1_r2(reg, &reg->a, reg->h);
      break;
    case 0x7D:
      gb_log(ALL, "LD A, L");
      ld_r1_r2(reg, &reg->a, reg->l);
      break;
    case 0x0A:
      gb_log(ALL, "LD A, (BC)");
      ld_r1_r2(reg, &reg->a, mmu_read_byte(mem, reg->bc));
      reg->clock.last.t += 4;
      break;
    case 0x1A:
      gb_log(ALL, "LD A, (DE)");
      ld_r1_r2(reg, &reg->a, mmu_read_byte(mem, reg->de));
      reg->clock.last.t += 4;
      break;
    case 0x7E:
      gb_log(ALL, "LD A, (HL)");
      ld_r1_r2(reg, &reg->a, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0xFA:
      gb_log(ALL, "LD A, (nn)");
      ld_a_nn(reg, mem, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x3E:
      gb_log(ALL, "LD A, #");
      ld_r1_r2(reg, &reg->a, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0x40:
      gb_log(ALL, "LD B, B");
      ld_r1_r2(reg, &reg->b, reg->b);
      break;
    case 0x41:
      gb_log(ALL, "LD B, C");
      ld_r1_r2(reg, &reg->b, reg->c);
      break;
    case 0x42:
      gb_log(ALL, "LD B, D");
      ld_r1_r2(reg, &reg->b, reg->d);
      break;
    case 0x43:
      gb_log(ALL, "LD B, E");
      ld_r1_r2(reg, &reg->b, reg->e);
      break;
    case 0x44:
      gb_log(ALL, "LD B, H");
      ld_r1_r2(reg, &reg->b, reg->h);
      break;
    case 0x45:
      gb_log(ALL, "LD B, L");
      ld_r1_r2(reg, &reg->b, reg->l);
      break;
    case 0x46:
      gb_log(ALL, "LD B, (HL)");
      ld_r1_r2(reg, &reg->b, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x48:
      gb_log(ALL, "LD C, B");
      ld_r1_r2(reg, &reg->c, reg->b);
      break;
    case 0x49:
      gb_log(ALL, "LD C, C");
      ld_r1_r2(reg, &reg->c, reg->c);
      break;
    case 0x4A:
      gb_log(ALL, "LD C, D");
      ld_r1_r2(reg, &reg->c, reg->d);
      break;
    case 0x4B:
      gb_log(ALL, "LD C, E");
      ld_r1_r2(reg, &reg->c, reg->e);
      break;
    case 0x4C:
      gb_log(ALL, "LD C, H");
      ld_r1_r2(reg, &reg->c, reg->h);
      break;
    case 0x4D:
      gb_log(ALL, "LD C, L");
      ld_r1_r2(reg, &reg->c, reg->l);
      break;
    case 0x4E:
      gb_log(ALL, "LD C, (HL)");
      ld_r1_r2(reg, &reg->c, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x50:
      gb_log(ALL, "LD D, B");
      ld_r1_r2(reg, &reg->d, reg->b);
      break;
    case 0x51:
      gb_log(ALL, "LD D, C");
      ld_r1_r2(reg, &reg->d, reg->c);
      break;
    case 0x52:
      gb_log(ALL, "LD D, D");
      ld_r1_r2(reg, &reg->d, reg->d);
      break;
    case 0x53:
      gb_log(ALL, "LD D, E");
      ld_r1_r2(reg, &reg->d, reg->e);
      break;
    case 0x54:
      gb_log(ALL, "LD D, H");
      ld_r1_r2(reg, &reg->d, reg->h);
      break;
    case 0x55:
      gb_log(ALL, "LD D, L");
      ld_r1_r2(reg, &reg->d, reg->l);
      break;
    case 0x56:
      gb_log(ALL, "LD D, (HL)");
      ld_r1_r2(reg, &reg->d, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x58:
      gb_log(ALL, "LD E, B");
      ld_r1_r2(reg, &reg->e, reg->b);
      break;
    case 0x59:
      gb_log(ALL, "LD E, C");
      ld_r1_r2(reg, &reg->e, reg->c);
      break;
    case 0x5A:
      gb_log(ALL, "LD E, D");
      ld_r1_r2(reg, &reg->e, reg->d);
      break;
    case 0x5B:
      gb_log(ALL, "LD E, E");
      ld_r1_r2(reg, &reg->e, reg->e);
      break;
    case 0x5C:
      gb_log(ALL, "LD E, H");
      ld_r1_r2(reg, &reg->e, reg->h);
      break;
    case 0x5D:
      gb_log(ALL, "LD E, L");
      ld_r1_r2(reg, &reg->e, reg->l);
      break;
    case 0x5E:
      gb_log(ALL, "LD E, (HL)");
      ld_r1_r2(reg, &reg->e, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x60:
      gb_log(ALL, "LD H, B");
      ld_r1_r2(reg, &reg->h, reg->b);
      break;
    case 0x61:
      gb_log(ALL, "LD H, C");
      ld_r1_r2(reg, &reg->h, reg->c);
      break;
    case 0x62:
      gb_log(ALL, "LD H, D");
      ld_r1_r2(reg, &reg->h, reg->d);
      break;
    case 0x63:
      gb_log(ALL, "LD H, E");
      ld_r1_r2(reg, &reg->h, reg->e);
      break;
    case 0x64:
      gb_log(ALL, "LD H, H");
      ld_r1_r2(reg, &reg->h, reg->h);
      break;
    case 0x65:
      gb_log(ALL, "LD H, L");
      ld_r1_r2(reg, &reg->h, reg->l);
      break;
    case 0x66:
      gb_log(ALL, "LD H, (HL)");
      ld_r1_r2(reg, &reg->h, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x68:
      gb_log(ALL, "LD L, B");
      ld_r1_r2(reg, &reg->l, reg->b);
      break;
    case 0x69:
      gb_log(ALL, "LD L, C");
      ld_r1_r2(reg, &reg->l, reg->c);
      break;
    case 0x6A:
      gb_log(ALL, "LD L, D");
      ld_r1_r2(reg, &reg->l, reg->d);
      break;
    case 0x6B:
      gb_log(ALL, "LD L, E");
      ld_r1_r2(reg, &reg->l, reg->e);
      break;
    case 0x6C:
      gb_log(ALL, "LD L, H");
      ld_r1_r2(reg, &reg->l, reg->h);
      break;
    case 0x6D:
      gb_log(ALL, "LD L, L");
      ld_r1_r2(reg, &reg->l, reg->l);
      break;
    case 0x6E:
      gb_log(ALL, "LD L, (HL)");
      ld_r1_r2(reg, &reg->l, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0x70:
      gb_log(ALL, "LD (HL), B");
      ld_r1_r2_hl(reg, mem, reg->b);
      break;
    case 0x71:
      gb_log(ALL, "LD (HL), C");
      ld_r1_r2_hl(reg, mem, reg->c);
      break;
    case 0x72:
      gb_log(ALL, "LD (HL), D");
      ld_r1_r2_hl(reg, mem, reg->d);
      break;
    case 0x73:
      gb_log(ALL, "LD (HL), E");
      ld_r1_r2_hl(reg, mem, reg->e);
      break;
    case 0x74:
      gb_log(ALL, "LD (HL), H");
      ld_r1_r2_hl(reg, mem, reg->h);
      break;
    case 0x75:
      gb_log(ALL, "LD (HL), L");
      ld_r1_r2_hl(reg, mem, reg->l);
      break;
    case 0x36:
      gb_log(ALL, "LD (HL), n");
      ld_r1_r2_hl(reg, mem, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0x47:
      gb_log(ALL, "LD B, A");
      ld_n_a(reg, &reg->b);
      break;
    case 0x4F:
      gb_log(ALL, "LD C, A");
      ld_n_a(reg, &reg->c);
      break;
    case 0x57:
      gb_log(ALL, "LD D, A");
      ld_n_a(reg, &reg->d);
      break;
    case 0x5F:
      gb_log(ALL, "LD E, A");
      ld_n_a(reg, &reg->e);
      break;
    case 0x67:
      gb_log(ALL, "LD H, A");
      ld_n_a(reg, &reg->h);
      break;
    case 0x6F:
      gb_log(ALL, "LD L, A");
      ld_n_a(reg, &reg->l);
      break;
    case 0x02:
      gb_log(ALL, "LD (BC), A");
      ld_nn_a(reg, mem, reg->bc);
      break;
    case 0x12:
      gb_log(ALL, "LD (DE), A");
      ld_nn_a(reg, mem, reg->de);
      break;
    case 0x77:
      gb_log(ALL, "LD (HL), A");
      ld_nn_a(reg, mem, reg->hl);
      break;
    case 0xEA:
      gb_log(ALL, "LD (nn), A");
      ld_nn_a_slow(reg, mem, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0x22:
      gb_log(ALL, "LDI (HL), A");
      ldi_hl_a(reg, mem);
      break;
    case 0xF2:
      gb_log(ALL, "LD A, (C)");
      ld_a_c(reg, mem);
      break;
    case 0xE2:
      gb_log(ALL, "LD (C), A");
      ld_c_a(reg, mem);
      break;
    case 0x3A:
      gb_log(ALL, "LDD A, (HL)");
      ldd_a_hl(reg, mem);
      break;
    case 0x0B:
      gb_log(ALL, "DEC bc");
      dec_nn(reg, &reg->bc);
      break;
    case 0x1B:
      gb_log(ALL, "DEC de");
      dec_nn(reg, &reg->de);
      break;
    case 0x2B:
      gb_log(ALL, "DEC hl");
      dec_nn(reg, &reg->hl);
      break;
    case 0x3B:
      gb_log(ALL, "DEC sp");
      dec_nn(reg, &reg->sp);
      break;
    case 0x3D:
      gb_log(ALL, "DEC A");
      dec_n(reg, &reg->a);
      break;
    case 0x05:
      gb_log(ALL, "DEC B");
      dec_n(reg, &reg->b);
      break;
    case 0x0D:
      gb_log(ALL, "DEC C");
      dec_n(reg, &reg->c);
      break;
    case 0x15:
      gb_log(ALL, "DEC D");
      dec_n(reg, &reg->d);
      break;
    case 0x1D:
      gb_log(ALL, "DEC E");
      dec_n(reg, &reg->e);
      break;
    case 0x25:
      gb_log(ALL, "DEC H");
      dec_n(reg, &reg->h);
      break;
    case 0x2D:
      gb_log(ALL, "DEC L");
      dec_n(reg, &reg->l);
      break;
    case 0x35:
      gb_log(ALL, "DEC (HL)");
      dec_hl(reg, mem);
      break;
    case 0x09:
      gb_log(ALL, "ADD HL, BC");
      add_hl_n(reg, reg->bc);
      break;
    case 0x19:
      gb_log(ALL, "ADD HL, DE");
      add_hl_n(reg, reg->de);
      break;
    case 0x29:
      gb_log(ALL, "ADD HL, HL");
      add_hl_n(reg, reg->hl);
      break;
    case 0x39:
      gb_log(ALL, "ADD HL, sp");
      add_hl_n(reg, reg->sp);
      break;
    case 0x03:
      gb_log(ALL, "INC BC");
      inc_nn(reg, &reg->bc);
      break;
    case 0x13:
      gb_log(ALL, "INC DE");
      inc_nn(reg, &reg->de);
      break;
    case 0x23:
      gb_log(ALL, "INC HL");
      inc_nn(reg, &reg->hl);
      break;
    case 0x33:
      gb_log(ALL, "INC SP");
      inc_nn(reg, &reg->sp);
      break;
    case 0xB7:
      gb_log(ALL, "OR A");
      or_n(reg, reg->a);
      break;
    case 0xB0:
      gb_log(ALL, "OR B");
      or_n(reg, reg->b);
      break;
    case 0xB1:
      gb_log(ALL, "OR C");
      or_n(reg, reg->c);
      break;
    case 0xB2:
      gb_log(ALL, "OR D");
      or_n(reg, reg->d);
      break;
    case 0xB3:
      gb_log(ALL, "OR E");
      or_n(reg, reg->e);
      break;
    case 0xB4:
      gb_log(ALL, "OR H");
      or_n(reg, reg->h);
      break;
    case 0xB5:
      gb_log(ALL, "OR L");
      or_n(reg, reg->l);
      break;
    case 0xB6:
      gb_log(ALL, "OR (HL)");
      or_n_slow(reg, mmu_read_byte(mem, reg->hl));
      break;
    case 0xF6:
      gb_log(ALL, "OR #");
      or_n_slow(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xBF:
      gb_log(ALL, "CP A");
      cp_n(reg, reg->a);
      break;
    case 0xB8:
      gb_log(ALL, "CP B");
      cp_n(reg, reg->b);
      break;
    case 0xB9:
      gb_log(ALL, "CP C");
      cp_n(reg, reg->c);
      break;
    case 0xBA:
      gb_log(ALL, "CP D");
      cp_n(reg, reg->d);
      break;
    case 0xBB:
      gb_log(ALL, "CP E");
      cp_n(reg, reg->e);
      break;
    case 0xBC:
      gb_log(ALL, "CP H");
      cp_n(reg, reg->h);
      break;
    case 0xBD:
      gb_log(ALL, "CP L");
      cp_n(reg, reg->l);
      break;
    case 0xBE:
      gb_log(ALL, "CP (HL)");
      cp_n_slow(reg, mmu_read_byte(mem, reg->hl));
      break;
    case 0xFE:
      gb_log(ALL, "CP #");
      cp_n_slow(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x97:
      gb_log(ALL, "SUB A");
      sub_n(reg, reg->a);
      break;
    case 0x90:
      gb_log(ALL, "SUB B");
      sub_n(reg, reg->b);
      break;
    case 0x91:
      gb_log(ALL, "SUB C");
      sub_n(reg, reg->c);
      break;
    case 0x92:
      gb_log(ALL, "SUB D");
      sub_n(reg, reg->d);
      break;
    case 0x93:
      gb_log(ALL, "SUB E");
      sub_n(reg, reg->e);
      break;
    case 0x94:
      gb_log(ALL, "SUB H");
      sub_n(reg, reg->h);
      break;
    case 0x95:
      gb_log(ALL, "SUB L");
      sub_n(reg, reg->l);
      break;
    case 0x9F:
      gb_log(ALL, "SBC A, A");
      sbc_a_n(reg, reg->a);
      break;
    case 0x98:
      gb_log(ALL, "SBC A, B");
      sbc_a_n(reg, reg->b);
      break;
    case 0x99:
      gb_log(ALL, "SBC A, C");
      sbc_a_n(reg, reg->c);
      break;
    case 0x9A:
      gb_log(ALL, "SBC A, D");
      sbc_a_n(reg, reg->d);
      break;
    case 0x9B:
      gb_log(ALL, "SBC A, E");
      sbc_a_n(reg, reg->e);
      break;
    case 0x9C:
      gb_log(ALL, "SBC A, H");
      sbc_a_n(reg, reg->h);
      break;
    case 0x9D:
      gb_log(ALL, "SBC A, L");
      sbc_a_n(reg, reg->l);
      break;
    case 0x9E:
      gb_log(ALL, "SBC A, (HL)");
      sbc_a_n(reg, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0xDE:
      gb_log(ALL, "SBC A, #");
      sbc_a_n(reg, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0x96:
      gb_log(ALL, "SUB (HL)");
      sub_n(reg, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0xD6:
      gb_log(ALL, "SUB #");
      sub_n(reg, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0xA7:
      gb_log(ALL, "AND A");
      and_n(reg, reg->a);
      break;
    case 0xA0:
      gb_log(ALL, "AND B");
      and_n(reg, reg->b);
      break;
    case 0xA1:
      gb_log(ALL, "AND C");
      and_n(reg, reg->c);
      break;
    case 0xA2:
      gb_log(ALL, "AND D");
      and_n(reg, reg->d);
      break;
    case 0xA3:
      gb_log(ALL, "AND E");
      and_n(reg, reg->e);
      break;
    case 0xA4:
      gb_log(ALL, "AND H");
      and_n(reg, reg->h);
      break;
    case 0xA5:
      gb_log(ALL, "AND L");
      and_n(reg, reg->l);
      break;
    case 0x87:
      gb_log(ALL, "ADD A, A");
      add_n(reg, reg->a);
      break;
    case 0x80:
      gb_log(ALL, "ADD A, B");
      add_n(reg, reg->b);
      break;
    case 0x81:
      gb_log(ALL, "ADD A, C");
      add_n(reg, reg->c);
      break;
    case 0x82:
      gb_log(ALL, "ADD A, D");
      add_n(reg, reg->d);
      break;
    case 0x83:
      gb_log(ALL, "ADD A, E");
      add_n(reg, reg->e);
      break;
    case 0x84:
      gb_log(ALL, "ADD A, H");
      add_n(reg, reg->h);
      break;
    case 0x85:
      gb_log(ALL, "ADD A, L");
      add_n(reg, reg->l);
      break;
    case 0x86:
      gb_log(ALL, "ADD A, (HL)");
      add_hl(reg, mem);
      break;
    case 0xC6:
      gb_log(ALL, "ADD A, #");
      add_n(reg, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0x8F:
      gb_log(ALL, "ADC A, A");
      adc_a_n(reg, reg->a);
      break;
    case 0x88:
      gb_log(ALL, "ADC A, B");
      adc_a_n(reg, reg->b);
      break;
    case 0x89:
      gb_log(ALL, "ADC A, C");
      adc_a_n(reg, reg->c);
      break;
    case 0x8A:
      gb_log(ALL, "ADC A, D");
      adc_a_n(reg, reg->d);
      break;
    case 0x8B:
      gb_log(ALL, "ADC A, E");
      adc_a_n(reg, reg->e);
      break;
    case 0x8C:
      gb_log(ALL, "ADC A, H");
      adc_a_n(reg, reg->h);
      break;
    case 0x8D:
      gb_log(ALL, "ADC A, L");
      adc_a_n(reg, reg->l);
      break;
    case 0x8E:
      gb_log(ALL, "ADC A, (HL)");
      adc_a_n(reg, mmu_read_byte(mem, reg->hl));
      reg->clock.last.t += 4;
      break;
    case 0xCE:
      gb_log(ALL, "ADC A, #");
      adc_a_n(reg, mmu_read_byte(mem, reg->pc++));
      reg->clock.last.t += 4;
      break;
    case 0xA6:
      gb_log(ALL, "AND (HL)");
      and_n_slow(reg, mmu_read_byte(mem, reg->hl));
      break;
    case 0xE6:
      gb_log(ALL, "AND #");
      and_n_slow(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xAF:
      gb_log(ALL, "XOR A");
      xor_n(reg, reg->a);
      break;
    case 0xA8:
      gb_log(ALL, "XOR B");
      xor_n(reg, reg->b);
      break;
    case 0xA9:
      gb_log(ALL, "XOR C");
      xor_n(reg, reg->c);
      break;
    case 0xAA:
      gb_log(ALL, "XOR D");
      xor_n(reg, reg->d);
      break;
    case 0xAB:
      gb_log(ALL, "XOR E");
      xor_n(reg, reg->e);
      break;
    case 0xAC:
      gb_log(ALL, "XOR H");
      xor_n(reg, reg->h);
      break;
    case 0xAD:
      gb_log(ALL, "XOR L");
      xor_n(reg, reg->l);
      break;
    case 0xAE:
      gb_log(ALL, "XOR (HL)");
      xor_n_slow(reg, mmu_read_byte(mem, reg->hl));
      break;
    case 0xEE:
      gb_log(ALL, "XOR #");
      xor_n_slow(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x20:
      gb_log(ALL, "JR NZ, *");
      jr_nz(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x28:
      gb_log(ALL, "JR Z, *");
      jr_z(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x30:
      gb_log(ALL, "JR NC, *");
      jr_nc(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0x38:
      gb_log(ALL, "JR C, *");
      jr_c(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xC2:
      gb_log(ALL, "JP NZ, nn");
      jp_nz(reg, mmu_read_word(mem, reg->pc));
      break;
    case 0xCA:
      gb_log(ALL, "JP Z, nn");
      jp_z(reg, mmu_read_word(mem, reg->pc));
      break;
    case 0xD2:
      gb_log(ALL, "JP NC, nn");
      jp_nc(reg, mmu_read_word(mem, reg->pc));
      break;
    case 0xDA:
      gb_log(ALL, "JP C, nn");
      jp_c(reg, mmu_read_word(mem, reg->pc));
      break;
    case 0xCD:
      gb_log(ALL, "CALL nn");
      call_nn(reg, mem, mmu_read_word(mem, reg->pc));
      break;
    case 0xC4:
      gb_log(ALL, "CALL NZ, nn");
      call_nz(reg, mem, mmu_read_word(mem, reg->pc));
      break;
    case 0xCC:
      gb_log(ALL, "CALL Z, nn");
      call_z(reg, mem, mmu_read_word(mem, reg->pc));
      break;
    case 0xD4:
      gb_log(ALL, "CALL NC, nn");
      call_nc(reg, mem, mmu_read_word(mem, reg->pc));
      break;
    case 0xDC:
      gb_log(ALL, "CALL C, nn");
      call_c(reg, mem, mmu_read_word(mem, reg->pc));
      break;
    case 0xC9:
      gb_log(ALL, "RET nn");
      ret(reg, mem);
      break;
    case 0xC0:
      gb_log(ALL, "RET NZ");
      ret_cc(reg, mem, !(reg->f & Z_BIT));
      break;
    case 0xC8:
      gb_log(ALL, "RET Z");
      ret_cc(reg, mem, reg->f & Z_BIT);
      break;
    case 0xD0:
      gb_log(ALL, "RET NC");
      ret_cc(reg, mem, !(reg->f & C_BIT));
      break;
    case 0xD8:
      gb_log(ALL, "RET C");
      ret_cc(reg, mem, reg->f & C_BIT);
      break;
    case 0xD9:
      gb_log(ALL, "RETI");
      reti(reg, mem);
      break;
    case 0xF0:
      gb_log(ALL, "LD A, (n)");
      ldh_a_n(reg, mem, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xF5:
      gb_log(ALL, "PUSH AF");
      push_nn(reg, mem, reg->af);
      break;
    case 0xC5:
      gb_log(ALL, "PUSH BC");
      push_nn(reg, mem, reg->bc);
      break;
    case 0xD5:
      gb_log(ALL, "PUSH DE");
      push_nn(reg, mem, reg->de);
      break;
    case 0xE5:
      gb_log(ALL, "PUSH HL");
      push_nn(reg, mem, reg->hl);
      break;
    case 0xF1:
      gb_log(ALL, "POP AF");
      pop_af(reg, mem);
      break;
    case 0xC1:
      gb_log(ALL, "POP BC");
      pop_nn(reg, mem, &reg->bc);
      break;
    case 0xD1:
      gb_log(ALL, "POP DE");
      pop_nn(reg, mem, &reg->de);
      break;
    case 0xE1:
      gb_log(ALL, "POP HL");
      pop_nn(reg, mem, &reg->hl);
      break;
    case 0x32:
      gb_log(ALL, "LDD (HL), A");
      ldd_hl_a(reg, mem);
      break;
    case 0x2A:
      gb_log(ALL, "LDI A, (HL)");
      ldi_a_hl(reg, mem);
      break;
    case 0x3C:
      gb_log(ALL, "INC A");
      inc_n(reg, &reg->a);
      break;
    case 0x04:
      gb_log(ALL, "INC B");
      inc_n(reg, &reg->b);
      break;
    case 0x0C:
      gb_log(ALL, "INC C");
      inc_n(reg, &reg->c);
      break;
    case 0x14:
      gb_log(ALL, "INC D");
      inc_n(reg, &reg->d);
      break;
    case 0x1C:
      gb_log(ALL, "INC E");
      inc_n(reg, &reg->e);
      break;
    case 0x24:
      gb_log(ALL, "INC H");
      inc_n(reg, &reg->h);
      break;
    case 0x2C:
      gb_log(ALL, "INC L");
      inc_n(reg, &reg->l);
      break;
    case 0x34:
      gb_log(ALL, "INC (HL)");
      inc_hl(reg, mem);
      break;
    case 0x27:
      gb_log(ALL, "DAA");
      daa(reg);
      break;
    case 0x2F:
      gb_log(ALL, "CPL");
      cpl(reg);
      break;
    case 0x3F:
      gb_log(ALL, "CCF");
      ccf(reg);
      break;
    case 0x37:
      gb_log(ALL, "SCF");
      scf(reg);
      break;
    case 0xC7:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x00);
      break;
    case 0xCF:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x08);
      break;
    case 0xD7:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x10);
      break;
    case 0xDF:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x18);
      break;
    case 0xE7:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x20);
      break;
    case 0xEF:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x28);
      break;
    case 0xF7:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x30);
      break;
    case 0xFF:
      gb_log(ALL, "RST n");
      rst_n(reg, mem, 0x38);
      break;
    case 0x1F:
      gb_log(ALL, "RRA");
      rra(reg);
      reg->clock.last.t -= 4;
      break;
    case 0x17:
      gb_log(ALL, "RLA");
      rla(reg);
      break;
    case 0x08:
      gb_log(ALL, "LD (nn), SP");
      ld_nn_sp(reg, mem, mmu_read_word(mem, reg->pc));
      reg->pc += 2;
      break;
    case 0xF9:
      gb_log(ALL, "LD SP, HL");
      ld_sp_hl(reg);
      break;
    case 0xE8:
      gb_log(ALL, "ADD SP, n");
      add_sp_n(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xF8:
      gb_log(ALL, "LDHL SP, n");
      ldhl_sp_n(reg, mmu_read_byte(mem, reg->pc++));
      break;
    case 0xCB:
      if (sub_command(reg, mem))
        {
          return 2;
        }
      break;
    default:
      gb_log(ERROR, "Unknown command: %02X", op);
      gb_log(ERROR, "Following bytes:");
      for (i=0; i<5; ++i)
        {
          gb_log(ERROR, " - %02X", mmu_read_byte(mem, reg->pc + i));
        }
      return 1;
    }

#if CGB
  reg->clock.last.t >>= reg->speed_shifter;
#endif

  reg->clock.t += reg->clock.last.t;

 isr_handling:

  reg->clock.m += reg->clock.last.t / 4;

  if (reg->halt)
    {
      check_halt_release(reg, mem);
    }

  if (reg->ime && !reg->halt)
    {
      check_isr(reg, mem);
    }

  return 0;
}
