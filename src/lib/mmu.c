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
  memset(mem->banks.ram.data, 0, sizeof mem->banks.ram.data);

  mem->tima_modified = false;
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
  mmu_write_byte(mem, 0xFF47, 0xFC);
  mmu_write_byte(mem, 0xFF48, 0xFF);
  mmu_write_byte(mem, 0xFF49, 0xFF);
}

#ifdef ENABLE_DEBUG
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

static inline void mmu_select_rom_bank(memory *mem, const uint8_t bank)
{
  mem->banks.rom.selected = bank;
  mem->banks.rom.selected &= mem->banks.rom.blocks - 1;
  mem->banks.rom.offset = 0x4000 * (mem->banks.rom.selected - 1);

  gb_log(VERBOSE, "Selected ROM bank %d", mem->banks.rom.selected);
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
    case 0x3000:
      {
        uint8_t val;
        switch(mem->rom.type & MBC_TYPE_MASK)
          {
          case MBC3:
            val = input & 0x7F;
            if (!val) ++val;
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
      if (mem->banks.mode)
        {
          mem->banks.ram.selected = input & 0x03;
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
      mem->video_ram[address - 0x8000] = input;
      break;
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
    case 0xD000:
      mem->internal_8k_ram[address - 0xC000] = input;
      break;
    default:
      if (address < 0xFE00)
        {
          mem->internal_8k_ram[address - 0xE000] = input;
        }
      else if (address < 0xFEA0)
        {
          mem->oam[address - 0xFE00] = input;
        }
      else if (address < 0xFF00)
        {
          mem->low_empty[address - 0xFEA0] = input;
        }
      else if (address < 0xFF4C)
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
              {
                const uint16_t input_addr = (uint16_t)input << 8;
                uint8_t *input_ptr = NULL;

                switch(input_addr & 0xF000)
                  {
                  case 0x0000:
                  case 0x1000:
                  case 0x2000:
                  case 0x3000:
                    input_ptr = &mem->rom.data[input_addr];
                    break;
                  case 0x4000:
                  case 0x5000:
                  case 0x6000:
                  case 0x7000:
                    input_ptr = &mem->rom.data[input_addr + mem->banks.rom.offset];
                    break;
                  case 0x8000:
                  case 0x9000:
                    input_ptr = &mem->video_ram[input_addr - 0x8000];
                    break;
                  case 0xA000:
                  case 0xB000:
                    input_ptr = &mem->banks.ram.data[mem->banks.ram.selected][input_addr - 0xA000];
                    break;
                  case 0xC000:
                  case 0xD000:
                    input_ptr = &mem->internal_8k_ram[input_addr - 0xC000];
                    break;
                  default:
                    break;
                  }

                assert(input_ptr);
                memcpy(mem->oam, input_ptr, 160);
                break;
              }
            case MEM_TIMA_ADDR:
              mem->tima_modified = true;
              // Intentional fall through
            default:
              mem->io_registers[address & 0x00FF] = input;
              break;
            }
        }
      else if (address < 0xFF80)
        {
          // Special register stops bootloader
          if (mem->bootloader_running && address == 0xFF50 && input == 0x01)
              mem->bootloader_running = false;
          else
            mem->high_empty[address - 0xFF4C] = input;
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
      return mem->video_ram[address - 0x8000];
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
    case 0xD000:
      return mem->internal_8k_ram[address - 0xC000];
    default:
      if (address < 0xFE00)
        {
          return mem->internal_8k_ram[address - 0xE000];
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
      else if (address < 0xFF4C)
        {
          return mem->io_registers[address - 0xFF00];
        }
      else if (address < 0xFF80)
        {
          return mem->high_empty[address - 0xFF4C];
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
  mmu_write_byte(mem, address, word);
  mmu_write_byte(mem, address + 1, word >> 8);
}
