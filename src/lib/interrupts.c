#include "interrupts.h"

#include "logger.h"
#include "memory_inline.h"

void isr_set_lcdc_isr_if_enabled(memory *mem, const uint8_t flag)
{
  const uint8_t stat = read_io_byte(mem, MEM_LCD_STAT);

  if (stat & flag)
    {
      isr_set_if_flag(mem, MEM_IF_LCDC_FLAG);
    }
}

void isr_compare_ly_lyc(memory *mem, const uint8_t ly, const uint8_t lyc)
{
  uint8_t stat = read_io_byte(mem, MEM_LCD_STAT);
  if (ly == lyc)
    {
      gb_log(VERBOSE, "LY == LYC (%d)", ly);

      isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_LYC_LY_ISR_ENABLED_FLAG);

      stat |= MEM_LCDC_LYC_LY_COINCIDENCE_FLAG;
    }
  else
    {
      stat &= ~MEM_LCDC_LYC_LY_COINCIDENCE_FLAG;
    }

  write_io_byte(mem, MEM_LCD_STAT, stat);
}

void isr_set_if_flag(memory *mem, const uint8_t flag)
{
  const uint8_t if_flag = read_io_byte(mem, MEM_IF_ADDR);
  write_io_byte(mem, MEM_IF_ADDR, if_flag | flag);
}
