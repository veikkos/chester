#include "mmu.h"
#include "interrupts.h"
#include "logger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void mmu_reset(memory *mem)
{
  mem->ie_register = 0x00;

  memset(mem->working_ram, 0, sizeof mem->working_ram);
  memset(mem->high_empty, 0, sizeof mem->high_empty);
  memset(mem->io_registers, 0, sizeof mem->io_registers);
  memset(mem->low_empty, 0, sizeof mem->low_empty);
  memset(mem->internal_8k_ram, 0, sizeof mem->internal_8k_ram);
  memset(mem->oam, 0, sizeof mem->oam);
  memset(mem->video_ram, 0, sizeof mem->video_ram);
#if CGB
  memset(mem->palette, 0, sizeof mem->palette);
  mem->cgb_mode = false;

  mem->dma.h_blank.src = 0;
  mem->dma.h_blank.dst = 0;
  mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] = 0xFF;
#endif

  mem->rom.data = NULL;
  mem->rom.type = NONE;

  mem->k = NULL;

  mem->bootloader = NULL;
  mem->bootloader_running = false;

  mem->banks.mode = 0;

  mem->banks.rom.selected = 1;
  mem->banks.rom.offset = 0;
  mem->banks.rom.blocks = 1;

  mem->banks.ram.selected = 0;
  mem->banks.ram.enabled = true;
  mem->banks.ram.written = false;
  memset(mem->banks.ram.data, 0, sizeof mem->banks.ram.data);

  mem->div_modified = false;
  mem->lcd_stopped = false;

  mmu_write_byte(mem, 0xFF10, 0x80);
  mmu_write_byte(mem, 0xFF11, 0xBF);
  mmu_write_byte(mem, 0xFF12, 0xF3);
  mmu_write_byte(mem, 0xFF14, 0xBF);
  mmu_write_byte(mem, 0xFF16, 0x3F);
  mmu_write_byte(mem, 0xFF19, 0xBF);
  mmu_write_byte(mem, 0xFF1A, 0x7F);
  mmu_write_byte(mem, 0xFF1B, 0xFF);
  mmu_write_byte(mem, 0xFF1C, 0x9F);
  mmu_write_byte(mem, 0xFF1E, 0xBF);
  mmu_write_byte(mem, 0xFF20, 0xFF);
  mmu_write_byte(mem, 0xFF23, 0xBF);
  mmu_write_byte(mem, 0xFF24, 0x77);
  mmu_write_byte(mem, 0xFF25, 0xF3);
  mmu_write_byte(mem, 0xFF26, 0xF1);
  mem->io_registers[MEM_LCD_STAT & 0x00FF] = 0x85;
  mmu_write_byte(mem, MEM_LCDC_ADDR, 0x91);
  mmu_write_byte(mem, MEM_BGP_ADDR, 0xFC);
  mmu_write_byte(mem, MEM_OBP0_ADDR, 0xFF);
  mmu_write_byte(mem, MEM_OBP1_ADDR, 0xFF);
#if CGB
  mmu_write_byte(mem, MEM_SVBK_ADDR, 0x01);
#endif
}

#ifndef NDEBUG
void mmu_debug_print(memory *mem, level l)
{
  gb_log(l, "Memory info");
  gb_log(l, " - Mode: %d", mem->banks.mode);
  gb_log(l, " - ROM");
  gb_log(l, "   - bank: %d", mem->banks.rom.selected);
  gb_log(l, "   - offset: %04X", mem->banks.rom.offset);
  gb_log(l, " - RAM");
  gb_log(l, "   - enabled: %d", mem->banks.ram.enabled);
  gb_log(l, "   - bank: %d", mem->banks.ram.selected);

  gb_log(l, " - Disp");
  gb_log(l, "   - LY: 0x%02X", mmu_read_byte(mem, MEM_LY_ADDR));
  gb_log(l, "   - LYC: 0x%02X", mmu_read_byte(mem, MEM_LYC_ADDR));
  gb_log(l, "   - STAT: 0x%02X", mmu_read_byte(mem, MEM_LCD_STAT));
  gb_log(l, "   - LCDC: 0x%02X", mmu_read_byte(mem, MEM_LCDC_ADDR));
}
#endif

void mmu_set_rom(memory *mem, uint8_t *rom, mbc type, uint32_t rom_size)
{
  mem->rom.type = type;
  mem->rom.data = rom;
  mem->banks.rom.blocks = rom_size / 0x4000;
}

void mmu_set_bootloader(memory *mem, uint8_t *bootloader)
{
  mem->bootloader_running = bootloader ? true : false;
  mem->bootloader = bootloader;
}

void mmu_set_keys(memory *mem, keys *k)
{
  mem->k = k;
}

static inline void mmu_select_rom_bank(memory *mem, const uint16_t bank)
{
  mem->banks.rom.selected = bank;
  mem->banks.rom.selected &= mem->banks.rom.blocks - 1;
  mem->banks.rom.offset = 0x4000 * (mem->banks.rom.selected - 1);

  gb_log(VERBOSE, "Selected ROM bank %d", mem->banks.rom.selected);
}

#if CGB
static inline uint8_t get_video_ram_bank(memory* mem)
{
  return mem->high_empty[MEM_VBK_ADDR - MEM_HIGH_EMPTY_START_ADDR];
}

static inline uint16_t get_internal_bank_offset(memory* mem)
{
  const uint8_t bank = mem->high_empty[MEM_SVBK_ADDR - MEM_HIGH_EMPTY_START_ADDR];
  return (bank - 1) * 4096;
}

static void color_palette_data(memory* mem, const uint16_t index_addr, const uint8_t palette_index, const uint8_t input)
{
  uint8_t* bcps_bgpi = &mem->high_empty[index_addr - MEM_HIGH_EMPTY_START_ADDR];
  const uint8_t index = *bcps_bgpi & 0x3F;
  mem->palette[palette_index][index] = input;
  if (*bcps_bgpi & MEM_PALETTE_INDEX_INCREMENT_FLAG)
    (*bcps_bgpi)++;
}
#endif

static uint8_t* get_dma_input_addr(memory* mem, const uint16_t input_addr)
{
  switch (input_addr & 0xF000)
  {
  case 0x0000:
  case 0x1000:
  case 0x2000:
  case 0x3000:
    return &mem->rom.data[input_addr];
  case 0x4000:
  case 0x5000:
  case 0x6000:
  case 0x7000:
    return &mem->rom.data[input_addr + mem->banks.rom.offset];
  case 0xA000:
  case 0xB000:
    return &mem->banks.ram.data[mem->banks.ram.selected][input_addr - 0xA000];
  case 0xC000:
#ifndef CGB
  case 0xD000:
#endif
    return &mem->internal_8k_ram[input_addr - 0xC000];
#ifdef CGB
  case 0xD000:
    return &mem->internal_8k_ram[input_addr - 0xC000 + get_internal_bank_offset(mem)];
#endif
  default:
    assert(!"Unexpected DMA input address");
    return NULL;
  }
}

static inline void dma(memory* mem, const uint8_t input)
{
  const uint16_t input_addr = (uint16_t)input << 8;
  const uint8_t *input_ptr = get_dma_input_addr(mem, input_addr);

  memcpy(mem->oam, input_ptr, 160);
}

static inline void write_high(memory *mem,
                              const uint16_t address,
                              const uint8_t input)
{
  if (address < 0xFE00)
    {
#ifdef CGB
      // Echo of above switchable RAM
      const uint16_t offset = get_internal_bank_offset(mem);
      mem->internal_8k_ram[address - 0xE000 + offset] = input;
#else
      mem->internal_8k_ram[address - 0xE000] = input;
#endif
    }
  else if (address < 0xFEA0)
    {
      mem->oam[address - 0xFE00] = input;
    }
  else if (address < 0xFF00)
    {
      mem->low_empty[address - 0xFEA0] = input;
    }
  else if (address < MEM_HIGH_EMPTY_START_ADDR)
    {
      switch(address)
      {
      case MEM_DIV_ADDR:
        mem->div_modified = true;
        mem->io_registers[address & 0x00FF] = 0;
        break;
      case MEM_LCDC_ADDR:
        // If LCD is disabled, LY needs to be cleared
        if ((mem->io_registers[address & 0x00FF] &
            MEM_LCDC_SCREEN_ENABLED_FLAG) &&
            !(input & MEM_LCDC_SCREEN_ENABLED_FLAG))
          {
            mem->io_registers[MEM_LY_ADDR & 0x00FF] = 0;
            isr_compare_ly_lyc(mem, 0, mmu_read_byte(mem, MEM_LYC_ADDR));

            mem->lcd_stopped = true;
          }

          mem->io_registers[address & 0x00FF] = input;
          break;
      case MEM_LCD_STAT:
        // Mode flags are not possible to overwrite
        mem->io_registers[address & 0x00FF] =
          (mem->io_registers[address & 0x00FF] & 0x87) |
          (input & 0x78);
        break;
      case MEM_LY_ADDR:
        mem->io_registers[address & 0x00FF] = 0;
        isr_compare_ly_lyc(mem, 0, mmu_read_byte(mem, MEM_LYC_ADDR));
        break;
      case MEM_LYC_ADDR:
        gb_log(VERBOSE, "LYC == (%d)", input);
        mem->io_registers[address & 0x00FF] = input;
        isr_compare_ly_lyc(mem, mmu_read_byte(mem, MEM_LY_ADDR), input);
        break;
      case MEM_DMA_ADDR:
        dma(mem, input);
        break;
      case 0xFF26:
        break;
      case MEM_SB_ADDR:
        if (mem->serial_cb) mem->serial_cb(input);
        // Intentional fall through
      default:
        mem->io_registers[address & 0x00FF] = input;
        break;
      }
    }
  else if (address < 0xFF80)
    {
#ifdef CGB
      switch (address)
      {
      case MEM_SVBK_ADDR:
        {
          uint8_t bank = input & MEM_VBK_BANK_MASK;
          if (!bank)
            bank = 1;

          mem->high_empty[MEM_SVBK_ADDR - MEM_HIGH_EMPTY_START_ADDR] = bank;
          break;
        }
      case MEM_VBK_ADDR:
        mem->high_empty[MEM_VBK_ADDR - MEM_HIGH_EMPTY_START_ADDR] = input & 0x01;
        break;
      case MEM_BCPD_BGPD_ADDR:
        color_palette_data(mem, MEM_BCPS_BGPI_ADDR, MEM_PALETTE_BG_INDEX, input);
        break;
      case MEM_OCPD_OBPD_ADDR:
        color_palette_data(mem, MEM_OCPS_OBPI_ADDR, MEM_PALETTE_SPRITE_INDEX, input);
        break;
      case MEM_HDMA1_ADDR:
      case MEM_HDMA2_ADDR:
      case MEM_HDMA3_ADDR:
      case MEM_HDMA4_ADDR:
        mem->high_empty[address - MEM_HIGH_EMPTY_START_ADDR] = input;
        break;
      case MEM_HDMA5_ADDR:
        {
          if (!(mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] & MEM_HDMA5_MODE_BIT))
            {
              gb_log(WARNING, "Wrote to HDMA5 while DMA active");
              return;
            }

          const uint16_t src = (mem->high_empty[MEM_HDMA1_ADDR - MEM_HIGH_EMPTY_START_ADDR] << 8) +
            (mem->high_empty[MEM_HDMA2_ADDR - MEM_HIGH_EMPTY_START_ADDR] & 0xF0);
          const uint16_t dst = ((mem->high_empty[MEM_HDMA3_ADDR - MEM_HIGH_EMPTY_START_ADDR] << 8) & 0x1F00) +
            (mem->high_empty[MEM_HDMA4_ADDR - MEM_HIGH_EMPTY_START_ADDR] & 0xF0);

          if (input & MEM_HDMA5_MODE_BIT)
            {
              mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] = input & MEM_HDMA5_LENGTH_MASK;
              mem->dma.h_blank.dst = dst;
              mem->dma.h_blank.src = src;
            }
          else
            {
              const uint16_t length = (uint16_t)((input & MEM_HDMA5_LENGTH_MASK) + 1) * MEM_HDMA_HBLANK_LENGTH;
              const uint8_t bank = get_video_ram_bank(mem);
              const uint8_t* input_addr = get_dma_input_addr(mem, src);

              memcpy(&mem->video_ram[bank][dst], input_addr, length);

              mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] = 0xFF;
            }
            break;
         }
        default:
#endif
          // Special register stops bootloader
          if (mem->bootloader_running && address == 0xFF50 && input == 0x01)
            mem->bootloader_running = false;
          else
            mem->high_empty[address - MEM_HIGH_EMPTY_START_ADDR] = input;
#ifdef CGB
          break;
      }
#endif
    }
  else if (address < 0xFFFF)
    {
      mem->working_ram[address - 0xFF80] = input;
    }
  else
    {
      mem->ie_register = input;
    }
}

void mmu_write_byte(memory *mem,
                    const uint16_t address,
                    const uint8_t input)
{
  switch(address & 0xF000)
    {
    case 0x0000:
    case 0x1000:
      break;
    case 0x2000:
      if ((mem->rom.type & MBC_TYPE_MASK) == MBC5)
        {
          mmu_select_rom_bank(mem, (mem->banks.rom.selected & 0xFF00) + input);
          break;
        }
      // Intentional fall through
    case 0x3000:
      {
        uint16_t val;
        switch(mem->rom.type & MBC_TYPE_MASK)
          {
          case MBC3:
            val = input & 0x7F;
            if (!val) ++val;
            break;
          case MBC5:
            val = mem->banks.rom.selected + ((input & 0x01) << 8);
            break;
          default:
            val = input & 0x1F;
            if (!val || val == 0x20 || val == 0x40 || val == 0x60) ++val;
            val |= (mem->banks.rom.selected & 0x60);
            break;
          }

        mmu_select_rom_bank(mem, val);
      }
      break;
    case 0x4000:
    case 0x5000:
      if (mem->banks.mode || (mem->rom.type & MBC_TYPE_MASK) == MBC5)
        {
          const uint8_t mask = (mem->rom.type & MBC_TYPE_MASK) == MBC5 ? 0x0F : 0x03;
          mem->banks.ram.selected = input & mask;
          gb_log(VERBOSE, "Selected RAM bank %d", mem->banks.ram.selected);
        }
      else
        {
          uint8_t val = (input & 0x03) << 5 | (mem->banks.rom.selected & 0x1F);

          switch(mem->rom.type & MBC_TYPE_MASK)
            {
            case MBC3:
              if (!val) ++val;
              break;
            default:
              if (!val || val == 0x20 || val == 0x40 || val == 0x60) ++val;
              break;
            }

          mmu_select_rom_bank(mem, val);
        }
      break;
    case 0x6000:
    case 0x7000:
      mem->banks.mode = input & 0x01;
      break;
    case 0x8000:
    case 0x9000:
      {
#ifdef CGB
        const uint8_t bank = get_video_ram_bank(mem);
        mem->video_ram[bank][address - 0x8000] = input;
#else
        mem->video_ram[address - 0x8000] = input;
#endif
        break;
      }
    case 0xA000:
    case 0xB000:
      if (!mem->banks.ram.enabled)
        {
          assert (!"Tried to access disabled RAM");
        }
      else
        {
          mem->banks.ram.written = true;
          mem->banks.ram.data[mem->banks.ram.selected][address - 0xA000] =
            input;
        }
      break;
    case 0xC000:
#ifndef CGB
    case 0xD000:
#endif
      // Fixed first 4k of internal RAM on CGB
      mem->internal_8k_ram[address - 0xC000] = input;
      break;
#ifdef CGB
    case 0xE000:
      // Echo of above
      mem->internal_8k_ram[address - 0xE000] = input;
      break;
    case 0xD000:
      {
        // Switchable second half of internal RAM
        const uint16_t offset = get_internal_bank_offset(mem);
        mem->internal_8k_ram[address - 0xC000 + offset] = input;
        break;
      }
#endif
    default:
      write_high(mem, address, input);
      break;
    }
}

static inline uint8_t read_high(memory *mem, const uint16_t address)
{
  if (address < 0xFE00)
    {
      // Echo of above switchable (on CGB) RAM
#ifdef CGB
      const uint16_t offset = get_internal_bank_offset(mem);
#endif
      return mem->internal_8k_ram[address - 0xE000
#ifdef CGB
          + offset
#endif
      ];
    }
  else if (address < 0xFEA0)
    {
      return mem->oam[address - 0xFE00];
    }
  else if (address < 0xFF00)
    {
      return mem->low_empty[address - 0xFEA0];
    }
  else if (address == 0xFF00)
    {
      const uint8_t key_base = 0xCF;
      uint8_t key_out = key_base | mem->io_registers[0];

      key_get_raw_output(mem->k, &mem->io_registers[0], &key_out);

      return key_out;
    }
  else if (address < MEM_HIGH_EMPTY_START_ADDR)
    {
      return mem->io_registers[address - 0xFF00];
    }
  else if (address < 0xFF80)
    {
      return mem->high_empty[address - MEM_HIGH_EMPTY_START_ADDR];
    }
  else if (address < 0xFFFF)
    {
      return mem->working_ram[address - 0xFF80];
    }
  else
    {
      return mem->ie_register;
    }
}

uint8_t mmu_read_byte(memory *mem, const uint16_t address)
{
  switch(address & 0xF000)
    {
    case 0x0000:
      if (mem->bootloader_running && address < 256)
        return mem->bootloader[address];
    case 0x1000:
    case 0x2000:
    case 0x3000:
      return mem->rom.data[address];
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
      return mem->rom.data[address + mem->banks.rom.offset];
    case 0x8000:
    case 0x9000:
      {
#ifdef CGB
        const uint8_t bank = get_video_ram_bank(mem);
        return mem->video_ram[bank][address - 0x8000];
#else
        return mem->video_ram[address - 0x8000];
#endif
      }
    case 0xA000:
    case 0xB000:
      if (!mem->banks.ram.enabled)
        {
          assert (!"Tried to access disabled RAM");
        }
      else
        {
          return mem->banks.ram.data[mem->banks.ram.selected][address - 0xA000];
        }
      break;
    case 0xC000:
#ifndef CGB
    case 0xD000:
#endif
      // Fixed first 4k of internal RAM
      return mem->internal_8k_ram[address - 0xC000];
#ifdef CGB
    case 0xE000:
      // Echo of above
      return mem->internal_8k_ram[address - 0xE000];
    case 0xD000:
      {
        // Switchable second half of internal RAM
        const uint16_t offset = get_internal_bank_offset(mem);
        return mem->internal_8k_ram[address - 0xC000 + offset];
      }
#endif
    default:
      return read_high(mem, address);
    }

  assert(!"Should not have any unhandled read addresses");

  return 0;
}

uint16_t mmu_read_word(memory *mem, const uint16_t address)
{
  return mmu_read_byte(mem, address) |
    (uint16_t)((mmu_read_byte(mem, address + 1)) << 8);
}

void mmu_write_word(memory *mem, const uint16_t address, const uint16_t word)
{
  mmu_write_byte(mem, address, (const uint8_t)word);
  mmu_write_byte(mem, address + 1, word >> 8);
}

#ifdef CGB
void mmu_hblank_dma(memory *mem)
{
  if (!(mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] & MEM_HDMA5_MODE_BIT))
    {
      const uint8_t bank = get_video_ram_bank(mem);
      const uint8_t* input_addr = get_dma_input_addr(mem, mem->dma.h_blank.src);

      memcpy(&mem->video_ram[bank][mem->dma.h_blank.dst], input_addr, MEM_HDMA_HBLANK_LENGTH);

      mem->dma.h_blank.dst += MEM_HDMA_HBLANK_LENGTH;
      mem->dma.h_blank.src += MEM_HDMA_HBLANK_LENGTH;

      mem->high_empty[MEM_HDMA5_ADDR - MEM_HIGH_EMPTY_START_ADDR] -= 1;
    }
}
#endif
