#include "timer.h"

#include "interrupts.h"
#include "logger.h"

static inline uint8_t read_timer_register(memory *mem, uint16_t address)
{
  // Faster than mmu_read_byte
  return mem->io_registers[address & 0x00FF];
}

void timer_update(registers *reg, memory *mem)
{
  // DIV timer
  {
    const unsigned int div = 256;

    // Reset the subcounter if timer was cleared
    if (mem->div_modified)
      {
        mem->div_modified = false;
        reg->timer.t_timer = 0;
        reg->timer.tick = 0;
      }

    reg->timer.t_timer += reg->clock.last.t
#ifdef CGB
      << reg->speed_shifter
#endif
      ;

    if (reg->timer.t_timer >= div)
      {
        reg->timer.t_timer -= div;

        ++(mem->io_registers[MEM_DIV_ADDR & 0x00FF]);
      }
  }

  const uint8_t tac = read_timer_register(mem, MEM_TAC_ADDR);

  if (tac & MEM_TAC_START)
    {
      gb_log (VERBOSE, "Timer running");

      static const unsigned int divs[] = {256, 4, 16, 64};
      const unsigned int div = divs[tac & 0x03];

      if (div != reg->timer.div)
        reg->timer.tick = 0;

      reg->timer.tick += (reg->clock.last.t
#ifdef CGB
        << reg->speed_shifter
#endif
        ) / 4;

      while (reg->timer.tick >= div)
        {
          reg->timer.tick -= div;

          const uint8_t t = read_timer_register(mem, MEM_TIMA_ADDR) + 1;

          gb_log (VERBOSE, "Timer step (%02X)", t);

          if (!t)
            {
              gb_log (DEBUG, "Timer overflow");

              isr_set_if_flag(mem, MEM_IF_TIMER_OVF_FLAG);

              mem->io_registers[MEM_TIMA_ADDR & 0x00FF] =
                read_timer_register(mem, MEM_TMA_ADDR);
            }
          else
            {
              mem->io_registers[MEM_TIMA_ADDR & 0x00FF] = t;
            }
        }

      reg->timer.div = div;
    }
}
