#include "cpu.h"

#define Z_BIT 0x80
#define N_BIT 0x40
#define H_BIT 0x20
#define C_BIT 0x10

static inline void or_flags(registers *reg)
{
  if(reg->a)
    reg->f = 0;
  else
    reg->f = Z_BIT;
}

#define xor_flags(r) or_flags(r)

static inline void and_flags(registers *reg)
{
  if(reg->a)
    reg->f = 0;
  else
    reg->f = Z_BIT;
  reg->f |= H_BIT;
}

static inline void cp_flags(registers *reg, uint8_t val)
{
  if(reg->a == val)
    reg->f = Z_BIT;
  else
    reg->f = 0;
  reg->f |= N_BIT;
  if((0x0F & reg->a) < (0x0F & val))
    reg->f |= H_BIT;
  if(reg->a < val)
    reg->f |= C_BIT;
}

#define sub_flags(r, val) cp_flags(r, val)

static inline void dec_flags(registers* reg, uint8_t val)
{
  if(val)
    reg->f &= ~Z_BIT;
  else
    reg->f |= Z_BIT;
  reg->f |= N_BIT;
  if((val & 0x0F) == 0x0F)
    reg->f |= H_BIT;
  else
    reg->f &= ~H_BIT;
}

static inline void inc_flags(registers *reg, uint8_t val)
{
  if(val)
    reg->f &= ~Z_BIT;
  else
    reg->f |= Z_BIT;
  reg->f &= ~N_BIT;
  if(val & 0x0F)
    reg->f &= ~H_BIT;
  else
    reg->f |= H_BIT;
}

static inline void add_flags_1(registers *reg, uint16_t val, uint16_t n)
{
  if(((val & 0x0F) + (n & 0x0F)) > 0x0F)
    reg->f = H_BIT;
  else
    reg->f = 0;
  if((val + n) > 0x00FF)
    reg->f |= C_BIT;
}

static inline void add_flags_c(registers *reg, uint16_t val, uint16_t n,
                               uint8_t c)
{
  if(((val & 0x0F) + (n & 0x0F) + c) > 0x0F)
    reg->f = H_BIT;
  else
    reg->f = 0;
  if((val + n + c) > 0x00FF)
    reg->f |= C_BIT;
}

static inline void add_flags_16bit_1(registers *reg, uint16_t val, uint16_t n)
{
  reg->f &= ~N_BIT;
  if(((val & 0x0FFF) + (n & 0x0FFF)) > 0x0FFF)
    reg->f |= H_BIT;
  else
    reg->f &= ~H_BIT;
  if(((uint32_t)val + (uint32_t)n) > 0xFFFF)
    reg->f |= C_BIT;
  else
    reg->f &= ~C_BIT;
}

static inline void add_flags_2(registers *reg, uint8_t val)
{
  if(!val)
    reg->f |= Z_BIT;
}

static inline void bit_flags(registers *reg, uint8_t val, uint8_t n)
{
  if(val & (0x01 << n))
    reg->f &= ~Z_BIT;
  else
    reg->f |= Z_BIT;
  reg->f &= ~N_BIT;
  reg->f |= H_BIT;
}

static inline void nop(registers *reg)
{
  reg->clock.last.t = 4;
}

static inline void halt(registers *reg, memory *mem)
{
  reg->halt = true;

  reg->clock.last.t = 4;
}

static inline void stop(registers *reg)
{
  reg->stop = true;

  reg->clock.last.t = 4;
}

static inline void jp_nn(registers *reg, uint16_t address)
{
  reg->pc = address;

  reg->clock.last.t = 16;
}

static inline void di(registers *reg)
{
  reg->ime = false;

  reg->clock.last.t = 4;
}

static inline void ei(registers *reg)
{
  reg->ime = true;

  reg->clock.last.t = 4;
}

static inline void rrc_n(registers *reg, uint8_t *n)
{
  *n = (*n << 7) | (*n >> 1);

  if(*n)
    reg->f = 0;
  else
    reg->f = Z_BIT;

  if(*n & 0x80)
    reg->f |= C_BIT;

  reg->clock.last.t = 4;
}

static inline void rrca(registers *reg)
{
  reg->a = (reg->a << 7) | (reg->a >> 1);

  reg->f = reg->a & 0x80 ? C_BIT : 0;

  reg->clock.last.t = 4;
}

static inline void rrc_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);

  n = (n << 7) | (n >> 1);

  reg->f = n ? 0 : Z_BIT;

  if(n & 0x80)
    reg->f |= C_BIT;

  reg->clock.last.t = 16;

  mmu_write_byte(mem, reg->hl, n);
}

static inline void rlca(registers *reg)
{
  reg->a = (reg->a >> 7) | (reg->a << 1);

  reg->f = reg->a & 0x01 ? C_BIT : 0;

  reg->clock.last.t = 4;
}

static inline void rlc_n(registers *reg, uint8_t *n)
{
  *n = (*n >> 7) | (*n << 1);

  reg->f = *n ? 0 : Z_BIT;

  if(*n & 0x01)
    reg->f |= C_BIT;

  reg->clock.last.t = 4;
}

static inline void rlc_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);

  n = (n >> 7) | (n << 1);

  reg->f = n ? 0 : Z_BIT;

  if(n & 0x01)
    reg->f |= C_BIT;

  mmu_write_byte(mem, reg->hl, n);

  reg->clock.last.t = 16;
}

static inline void xor_n(registers *reg, uint8_t n)
{
  reg->a ^= n;

  xor_flags(reg);

  reg->clock.last.t = 4;
}

static inline void xor_n_slow(registers *reg, uint8_t n)
{
  reg->a ^= n;

  xor_flags(reg);

  reg->clock.last.t = 8;
}

static inline void ldh_n_a(registers *reg, memory *mem, uint8_t n)
{
  mmu_write_byte(mem, 0xFF00 + n, reg->a);

  reg->clock.last.t = 12;
}

static inline void ldh_bc_nn(registers *reg, uint16_t nn)
{
  reg->bc = nn;

  reg->clock.last.t = 12;
}

static inline void ldh_de_nn(registers *reg, uint16_t nn)
{
  reg->de = nn;

  reg->clock.last.t = 12;
}

static inline void ldh_hl_nn(registers *reg, uint16_t nn)
{
  reg->hl = nn;

  reg->clock.last.t = 12;
}

static inline void ldh_sp_nn(registers *reg, uint16_t nn)
{
  reg->sp = nn;

  reg->clock.last.t = 12;
}

static inline void ld_n_a(registers *reg, uint8_t *n)
{
  *n = reg->a;

  reg->clock.last.t = 4;
}

static inline void ld_nn_a(registers *reg, memory *mem, uint16_t nn)
{
  mmu_write_byte(mem, nn, reg->a);

  reg->clock.last.t = 8;
}

static inline void ld_nn_a_slow(registers *reg, memory *mem, uint16_t nn)
{
  mmu_write_byte(mem, nn, reg->a);

  reg->clock.last.t = 16;
}

static inline void ldi_hl_a(registers *reg, memory *mem)
{
  mmu_write_byte(mem, reg->hl++, reg->a);

  reg->clock.last.t = 8;
}

static inline void dec_nn(registers *reg, uint16_t *nn)
{
  --(*nn);

  reg->clock.last.t = 8;
}

static inline void dec_n(registers *reg, uint8_t *n)
{
  --(*n);

  dec_flags(reg, *n);

  reg->clock.last.t = 4;
}

static inline void dec_hl(registers *reg, memory *mem)
{
  uint8_t val = mmu_read_byte(mem, reg->hl);
  mmu_write_byte(mem, reg->hl, --val);

  dec_flags(reg, val);

  reg->clock.last.t = 12;
}

static inline void or_n(registers *reg, uint8_t n)
{
  reg->a |= n;

  or_flags(reg);

  reg->clock.last.t = 4;
}

static inline void or_n_slow(registers *reg, uint8_t n)
{
  reg->a |= n;

  or_flags(reg);

  reg->clock.last.t = 8;
}

static inline void and_n(registers *reg, uint8_t n)
{
  reg->a &= n;

  and_flags(reg);

  reg->clock.last.t = 4;
}

static inline void and_n_slow(registers *reg, uint8_t n)
{
  reg->a &= n;

  and_flags(reg);

  reg->clock.last.t = 8;
}

static inline void sub_n(registers *reg, uint8_t n)
{
  sub_flags(reg, n);

  reg->a -= n;

  reg->clock.last.t = 4;
}

static inline void cp_n(registers *reg, uint8_t n)
{
  cp_flags(reg, n);

  reg->clock.last.t = 4;
}

static inline void cp_n_slow(registers *reg, uint8_t n)
{
  cp_flags(reg, n);

  reg->clock.last.t = 8;
}

static inline void jr_if(registers *reg, const int8_t n, const bool jump)
{
  if (jump)
    {
      reg->pc += n;
      reg->clock.last.t = 12;
    }
  else
    {
      reg->clock.last.t = 8;
    }
}

static inline void jr_nc(registers *reg, const int8_t n)
{
  jr_if(reg, n, !(reg->f & C_BIT));
}

static inline void jr_c(registers *reg, const int8_t n)
{
  jr_if(reg, n, reg->f & C_BIT);
}

static inline void jr_nz(registers *reg, const int8_t n)
{
  jr_if(reg, n, !(reg->f & Z_BIT));
}

static inline void jr_z(registers *reg, const int8_t n)
{
  jr_if(reg, n, reg->f & Z_BIT);
}

static inline void jp_if(registers *reg, const uint16_t nn, const bool jump)
{
  if (jump)
    {
      reg->pc = nn;
      reg->clock.last.t = 16;
    }
  else
    {
      reg->pc += 2;
      reg->clock.last.t = 12;
    }
}

static inline void jp_nc(registers *reg, const uint16_t nn)
{
  jp_if(reg, nn, !(reg->f & C_BIT));
}

static inline void jp_c(registers *reg, const uint16_t nn)
{
  jp_if(reg, nn, reg->f & C_BIT);
}

static inline void jp_nz(registers *reg, const uint16_t nn)
{
  jp_if(reg, nn, !(reg->f & Z_BIT));
}

static inline void jp_z(registers *reg, const uint16_t nn)
{
  jp_if(reg, nn, reg->f & Z_BIT);
}

static inline void call_nn(registers *reg, memory *mem, const uint16_t nn)
{
  reg->sp -= 2;
  mmu_write_word(mem, reg->sp, reg->pc + 2);
  reg->pc = nn;

  gb_log(VERBOSE, "Calling %04X", nn);

  reg->clock.last.t = 24;
}

static inline void call_if(registers *reg, memory *mem, const uint16_t nn,
                           const bool call)
{
  if (call)
    {
      call_nn(reg, mem, nn);
    }
  else
    {
      reg->pc += 2;
      reg->clock.last.t = 12;
    }
}

static inline void call_nc(registers *reg, memory *mem, const uint16_t nn)
{
  call_if(reg, mem, nn, !(reg->f & C_BIT));
}

static inline void call_c(registers *reg, memory *mem, const uint16_t nn)
{
  call_if(reg, mem, nn, reg->f & C_BIT);
}

static inline void call_nz(registers *reg, memory *mem, const uint16_t nn)
{
  call_if(reg, mem, nn, !(reg->f & Z_BIT));
}

static inline void call_z(registers *reg, memory *mem, const uint16_t nn)
{
  call_if(reg, mem, nn, reg->f & Z_BIT);
}

static inline void ret(registers *reg, memory *mem)
{
  reg->pc = mmu_read_word(mem, reg->sp);
  reg->sp += 2;

  reg->clock.last.t = 16;
}

static inline void ret_cc(registers *reg, memory *mem, const bool cond)
{
  if (cond)
    {
      ret(reg, mem);
      reg->clock.last.t += 4;
    }
  else
    {
      reg->clock.last.t = 8;
    }
}

static inline void reti(registers *reg, memory *mem)
{
  reg->pc = mmu_read_word(mem, reg->sp);
  reg->sp += 2;

  reg->ime = true;

  reg->clock.last.t = 16;
}

static inline void ldh_a_n(registers *reg, memory *mem, uint8_t n)
{
  reg->a = mmu_read_byte(mem, 0xFF00 + n);

  reg->clock.last.t = 12;
}

static inline void ld_nn_n(registers *reg, uint8_t n, uint8_t *nn)
{
  *nn = n;

  reg->clock.last.t = 8;
}

static inline void push_nn(registers *reg, memory *mem, uint16_t nn)
{
  reg->sp -= 2;
  mmu_write_word(mem, reg->sp, nn);

  reg->clock.last.t = 16;
}

static inline void pop_nn(registers *reg, memory *mem, uint16_t *nn)
{
  *nn = mmu_read_word(mem, reg->sp);
  reg->sp += 2;

  reg->clock.last.t = 12;
}

static inline void pop_af(registers *reg, memory *mem)
{
  reg->af = mmu_read_word(mem, reg->sp);
  reg->sp += 2;

  reg->f &= 0xF0;

  reg->clock.last.t = 12;
}

static inline void bit_b_r(registers *reg, uint8_t r, uint8_t b)
{
  bit_flags(reg, r, b);

  reg->clock.last.t = 8;
}

static inline void ld_a_c(registers *reg, memory *mem)
{
  reg->a = mmu_read_byte(mem, 0xFF00 + reg->c);

  reg->clock.last.t = 8;
}

static inline void ld_c_a(registers *reg, memory *mem)
{
  mmu_write_byte(mem, 0xFF00 + reg->c, reg->a);

  reg->clock.last.t = 8;
}

static inline void ldd_a_hl(registers *reg, memory *mem)
{
  reg->a = mmu_read_byte(mem, reg->hl--);

  reg->clock.last.t = 8;
}

static inline void ldd_hl_a(registers *reg, memory *mem)
{
  mmu_write_byte(mem, reg->hl--, reg->a);

  reg->clock.last.t = 8;
}

static inline void ldi_a_hl(registers *reg, memory *mem)
{
  reg->a = mmu_read_byte(mem, reg->hl++);

  reg->clock.last.t = 8;
}

static inline void ld_r1_r2(registers *reg, uint8_t *r1, const uint8_t r2)
{
  *r1 = r2;

  reg->clock.last.t = 4;
}

static inline void ld_r1_r2_hl(registers *reg, memory *mem, const uint8_t r2)
{
  mmu_write_byte(mem, reg->hl, r2);

  reg->clock.last.t = 8;
}

static inline void ld_a_nn(registers *reg, memory *mem, const uint16_t nn)
{
  reg->a = mmu_read_byte(mem, nn);

  reg->clock.last.t = 16;
}

static inline void inc_n(registers *reg, uint8_t *n)
{
  ++(*n);

  inc_flags(reg, *n);

  reg->clock.last.t = 4;
}

static inline void inc_hl(registers *reg, memory *mem)
{
  uint8_t val = mmu_read_byte(mem, reg->hl);
  mmu_write_byte(mem, reg->hl, ++val);

  inc_flags(reg, val);

  reg->clock.last.t = 12;
}

static inline void daa(registers *reg)
{
  uint16_t a = reg->a;

  if (!(reg->f & N_BIT))
    {
      if (reg->f & H_BIT || (a & 0xF) > 9)
        {
          a += 0x06;
        }

      if (reg->f & C_BIT || a > 0x9F)
        {
          a += 0x60;
        }
    }
  else
    {
      if (reg->f & H_BIT)
        {
          a = (a - 6) & 0xFF;
        }

      if (reg->f & C_BIT)
        {
          a -= 0x60;
        }
    }

  reg->f &= ~(H_BIT | Z_BIT);

  if (a & 0x100)
    reg->f |= C_BIT;

  a &= 0xFF;

  if (a == 0)
    reg->f |= Z_BIT;

  reg->a = (uint8_t)a;

  reg->clock.last.t = 4;
}

static inline void cpl(registers *reg)
{
  reg->a = ~reg->a;

  reg->f |= N_BIT + H_BIT;

  reg->clock.last.t = 4;
}

static inline void ccf(registers *reg)
{
  reg->f ^= C_BIT;

  reg->f &= ~(N_BIT + H_BIT);

  reg->clock.last.t = 4;
}

static inline void scf(registers *reg)
{
  reg->f |= C_BIT;

  reg->f &= ~(N_BIT + H_BIT);

  reg->clock.last.t = 4;
}

static inline void swap_n(registers *reg, uint8_t *n)
{
  *n = (*n << 4) | (*n >> 4);

  reg->f = *n ? 0x00 : Z_BIT;

  reg->clock.last.t = 8;
}

static inline void swap_hl(registers *reg, memory *mem)
{
  uint8_t val = mmu_read_byte(mem, reg->hl);
  val = (val << 4) | (val >> 4);
  mmu_write_byte(mem, reg->hl, val);

  reg->f = val ? 0x00 : Z_BIT;

  reg->clock.last.t = 16;
}

static inline void rst_n(registers *reg, memory *mem, const uint8_t n)
{
  reg->sp -= 2;
  mmu_write_word(mem, reg->sp, reg->pc);

  reg->pc = n;

  reg->clock.last.t = 16;
}

static inline void add_n(registers *reg, const uint8_t n)
{
  add_flags_1(reg, reg->a, n);

  reg->a += n;

  add_flags_2(reg, reg->a);

  reg->clock.last.t = 4;
}

static inline void add_hl(registers *reg, memory *mem)
{
  const uint8_t n = mmu_read_byte(mem, reg->hl);

  add_flags_1(reg, reg->a, n);

  reg->a += n;

  add_flags_2(reg, reg->a);

  reg->clock.last.t = 8;
}

static inline void add_hl_n(registers *reg, uint16_t n)
{
  add_flags_16bit_1(reg, reg->hl, n);

  reg->hl += n;

  reg->clock.last.t = 8;
}

static inline void inc_nn(registers *reg, uint16_t *nn)
{
  ++(*nn);

  reg->clock.last.t = 8;
}

static inline void jp_hl(registers *reg)
{
  reg->pc = reg->hl;

  reg->clock.last.t = 4;
}

static inline void jr_n(registers *reg, int8_t n)
{
  reg->pc += n;

  reg->clock.last.t = 12;
}

static inline void adc_a_n(registers *reg, uint16_t n)
{
  const uint8_t carry = (reg->f & C_BIT) ? 1 : 0;

  add_flags_c(reg, reg->a, n, carry);

  reg->a += n + carry;

  add_flags_2(reg, reg->a);

  reg->clock.last.t = 4;
}

static inline void sbc_a_n(registers *reg, uint16_t n)
{
  const int carry = (reg->f & C_BIT) ? 1 :0;
  const int res = reg->a - (n + carry);

  reg->f = (res < 0) ? C_BIT : 0;

  if (((reg->a & 0x0F) - (n & 0x0F) - carry) < 0)
    {
      reg->f |= H_BIT;
    }

  reg->a = res;

  reg->f |= N_BIT;

  if (reg->a == 0)
    {
      reg->f |= Z_BIT;
    }

  reg->clock.last.t = 4;
}

static inline void res_b_r(registers *reg, uint8_t *r, const uint8_t b)
{
  *r &= ~(0x01 << b);

  reg->clock.last.t = 8;
}

static inline void res_b_hl(registers *reg, memory *mem, const uint8_t b)
{
  uint8_t r = mmu_read_byte(mem, reg->hl);
  r &= ~(0x01 << b);
  mmu_write_byte(mem, reg->hl, r);

  reg->clock.last.t = 16;
}

static inline void set_b_r(registers *reg, uint8_t *r, const uint8_t b)
{
  *r |= (0x01 << b);

  reg->clock.last.t = 8;
}

static inline void set_b_hl(registers *reg, memory *mem, const uint8_t b)
{
  uint8_t r = mmu_read_byte(mem, reg->hl);
  r |= (0x01 << b);
  mmu_write_byte(mem, reg->hl, r);

  reg->clock.last.t = 16;
}

static inline void sla_n(registers *reg, uint8_t *n)
{
  reg->f = (*n & 0x80) ? C_BIT : 0x00;

  *n = *n << 1;

  if (!(*n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 8;
}

static inline void sla_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);

  reg->f = (n & 0x80) ? C_BIT : 0x00;

  n = n << 1;

  if (!(n))
    reg->f |= Z_BIT;

  mmu_write_byte(mem, reg->hl, n);

  reg->clock.last.t = 16;
}

static inline void srl_n(registers *reg, uint8_t *n)
{
  reg->f = (*n & 0x01) ? C_BIT : 0x00;

  *n = *n >> 1;

  if(!(*n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 8;
}

static inline void srl_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);

  reg->f = (n & 0x01) ? C_BIT : 0x00;

  n = n >> 1;

  mmu_write_byte(mem, reg->hl, n);

  if(!(n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 16;
}

static inline void sra_n(registers *reg, uint8_t *n)
{
  reg->f = (*n & 0x01) ? C_BIT : 0x00;

  *n = (*n & 0x80) | (*n >> 1);

  if(!(*n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 8;
}

static inline void sra_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);

  reg->f = (n & 0x01) ? C_BIT : 0x00;

  n = (n & 0x80) | (n >> 1);

  mmu_write_byte(mem, reg->hl, n);

  if(!(n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 16;
}

static inline void rr_n(registers *reg, uint8_t *n)
{
  uint8_t old_carry = (reg->f & C_BIT) << 3;

  reg->f = (*n & 0x01) ? C_BIT : 0x00;

  *n = old_carry | (*n >> 1);

  if(!(*n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 8;
}

static inline void rra(registers *reg)
{
  uint8_t old_carry = (reg->f & C_BIT) << 3;

  reg->f = (reg->a & 0x01) ? C_BIT : 0x00;

  reg->a = old_carry | (reg->a >> 1);

  reg->clock.last.t = 8;
}

static inline void rr_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);
  uint8_t old_carry = (reg->f & C_BIT) << 3;

  reg->f = (n & 0x01) ? C_BIT : 0x00;

  n = old_carry | (n >> 1);

  if(!(n))
    reg->f |= Z_BIT;

  mmu_write_byte(mem, reg->hl, n);

  reg->clock.last.t = 16;
}

static inline void rl_n(registers *reg, uint8_t *n)
{
  uint8_t old_carry = (reg->f & C_BIT) >> 4;

  reg->f = (*n & 0x80) ? C_BIT : 0x00;

  *n = old_carry | (*n << 1);

  if(!(*n))
    reg->f |= Z_BIT;

  reg->clock.last.t = 8;
}

static inline void rla(registers *reg)
{
  uint8_t old_carry = (reg->f & C_BIT) >> 4;

  reg->f = (reg->a & 0x80) ? C_BIT : 0x00;

  reg->a = old_carry | (reg->a << 1);

  reg->clock.last.t = 4;
}

static inline void rl_hl(registers *reg, memory *mem)
{
  uint8_t n = mmu_read_byte(mem, reg->hl);
  uint8_t old_carry = (reg->f & C_BIT) >> 4;

  reg->f = (n & 0x80) ? C_BIT : 0x00;

  n = old_carry | (n << 1);

  if(!(n))
    reg->f |= Z_BIT;

  mmu_write_byte(mem, reg->hl, n);

  reg->clock.last.t = 16;
}

static inline void ld_nn_sp(registers *reg, memory *mem, uint16_t nn)
{
  mmu_write_word(mem, nn, reg->sp);

  reg->clock.last.t = 20;
}

static inline void ld_sp_hl(registers *reg)
{
  reg->sp = reg->hl;

  reg->clock.last.t = 8;
}

static inline uint16_t sp_n(registers *reg, int8_t n)
{
  const uint16_t res = (reg->sp + n);

  reg->f = 0;

  if ((res & 0xFF) < (reg->sp & 0xFF))
    {
      reg->f |= C_BIT;
    }

  if ((res & 0xF) < (reg->sp & 0xF))
    {
      reg->f |= H_BIT;
    }

  return res;
}

static inline void add_sp_n(registers *reg, int8_t n)
{
  reg->sp = sp_n(reg, n);

  reg->clock.last.t = 16;
}

static inline void ldhl_sp_n(registers *reg, int8_t n)
{
  reg->hl = sp_n(reg, n);

  reg->clock.last.t = 12;
}

static inline void jump_to_isr_address(registers *reg, memory *mem, uint16_t addr)
{
  reg->ime = false;

  reg->sp -= 2;
  mmu_write_word(mem, reg->sp, reg->pc);

  reg->pc = addr;
}
