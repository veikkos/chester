#ifndef MMU_H
#define MMU_H

#include "keys.h"
#include "logger.h"

#include <stdint.h>

#define MBC_BATTERY_BIT 0x80

typedef void (*serial_cb)(uint8_t);

typedef enum {
  NONE = 0x00,
  MBC1 = 0x01,
  MBC1_BATTERY = MBC1 + MBC_BATTERY_BIT,
  MBC3 = 0x02,
  MBC3_BATTERY = MBC3 + MBC_BATTERY_BIT,
  MBC5 = 0x03,
  MBC5_BATTERY = MBC5 + MBC_BATTERY_BIT,
  NOT_SUPPORTED = 0xFF
} mbc;

#define MBC_TYPE_MASK (uint8_t)0x7F
#define MBC_BATTERY_MASK (uint8_t)MBC_BATTERY_BIT

struct memory_s {
  uint8_t ie_register;
  uint8_t working_ram[127];
  uint8_t internal_ram[128];
  uint8_t high_empty[52];
  uint8_t io_registers[76];
  uint8_t low_empty[96];
#ifdef CGB
  uint8_t internal_8k_ram[8192 + 6 * 4096];
#else
  uint8_t internal_8k_ram[8192];
#endif
  uint8_t oam[160];
#ifdef CGB
  uint8_t video_ram[2][8192];
  uint8_t palette[2][64];
#else
  uint8_t video_ram[8192];
#endif
  struct {
    uint8_t *data;
    mbc type;
  }rom;
  uint8_t *bootloader;
  bool bootloader_running;
  keys *k;

  bool div_modified;
  bool lcd_stopped;

  struct {
    bool mode;
    struct {
      uint16_t selected;
      uint32_t offset;
      uint16_t blocks;
    }rom;
    struct {
      bool enabled, written;
      uint8_t selected;
      uint8_t data[16][8192];
    }ram;
  }banks;

#ifdef CGB
  bool cgb_mode;

  struct {
    struct {
      uint16_t src;
      uint16_t dst;
    }h_blank;
  }dma;
#endif

  serial_cb serial_cb;
};

typedef struct memory_s memory;

#define MEM_TILE_MAP_ADDR_1 0x9800
#define MEM_TILE_MAP_ADDR_2 0x9C00
#define MEM_TILE_ADDR_1 0x8800
#define MEM_TILE_ADDR_2 0x8000

#define MEM_SPRITE_ATTRIBUTE_TABLE 0xFE00
#define MEM_SPRITE_ADDR 0x8000

#define MEM_VBLANK_ISR_ADDR 0x0040
#define MEM_LCD_ISR_ADDR 0x0048
#define MEM_TIMER_ISR_ADDR 0x0050
#define MEM_PIN_ISR_ADDR 0x0060
#define MEM_SB_ADDR 0xFF01
#define MEM_SC_ADDR 0xFF02
#define MEM_DIV_ADDR 0xFF04
#define MEM_TIMA_ADDR 0xFF05
#define MEM_TMA_ADDR 0xFF06
#define MEM_TAC_ADDR 0xFF07
#define MEM_IF_ADDR 0xFF0F
#define MEM_LCDC_ADDR 0xFF40
#define MEM_LCD_STAT 0xFF41
#define MEM_SCY_ADDR 0xFF42
#define MEM_SCX_ADDR 0xFF43
#define MEM_LY_ADDR 0xFF44
#define MEM_LYC_ADDR 0xFF45
#define MEM_DMA_ADDR 0xFF46
#define MEM_BGP_ADDR 0xFF47
#define MEM_OBP0_ADDR 0xFF48
#define MEM_OBP1_ADDR 0xFF49
#define MEM_WY_ADDR 0xFF4A
#define MEM_WX_ADDR 0xFF4B
#define MEM_HIGH_EMPTY_START_ADDR 0xFF4C
#define MEM_KEY1_ADDR 0xFF4D
#define MEM_VBK_ADDR 0xFF4F
#define MEM_HDMA1_ADDR 0xFF51
#define MEM_HDMA2_ADDR 0xFF52
#define MEM_HDMA3_ADDR 0xFF53
#define MEM_HDMA4_ADDR 0xFF54
#define MEM_HDMA5_ADDR 0xFF55
#define MEM_BCPS_BGPI_ADDR 0xFF68
#define MEM_BCPD_BGPD_ADDR 0xFF69
#define MEM_OCPS_OBPI_ADDR 0xFF6A
#define MEM_OCPD_OBPD_ADDR 0xFF6B
#define MEM_SVBK_ADDR 0xFF70
#define MEM_IE_ADDR 0xFFFF

#define MEM_IF_PIN_FLAG 0x10
#define MEM_IF_SERIAL_IO_FLAG 0x08
#define MEM_IF_TIMER_OVF_FLAG 0x04
#define MEM_IF_LCDC_FLAG 0x02
#define MEM_IF_VBLANK_FLAG 0x01

#define MEM_LCDC_SCREEN_ENABLED_FLAG 0x80
#define MEM_LCDC_WINDOW_TILEMAP_SELECT_FLAG 0x40
#define MEM_LCDC_WINDOW_ENABLED_FLAG 0x20
#define MEM_LCDC_TILEMAP_DATA_FLAG 0x10
#define MEM_LCDC_TILEMAP_SELECT_FLAG 0x08
#define MEM_LCDC_SPRITES_SIZE_FLAG 0x04
#define MEM_LCDC_SPRITES_ENABLED_FLAG 0x02
#define MEM_LCDC_BG_WINDOW_ENABLED_FLAG 0x01

#define MEM_LCDC_LYC_LY_ISR_ENABLED_FLAG 0x40
#define MEM_LCDC_OAM_ISR_ENABLED_FLAG 0x20
#define MEM_LCDC_VBLANK_ISR_ENABLED_FLAG 0x10
#define MEM_LCDC_HBLANK_ISR_ENABLED_FLAG 0x08
#define MEM_LCDC_LYC_LY_COINCIDENCE_FLAG 0x04

#define MEM_PALETTE_INDEX_INCREMENT_FLAG 0x80

#define MEM_PALETTE_BG_INDEX 0
#define MEM_PALETTE_SPRITE_INDEX 1

#define MEM_TAC_START 0x04

#define MEM_CHARACTER_CODE_BANK_INDEX 0
#define MEM_ATTRIBUTES_CODE_BANK_INDEX 1

#define MEM_KEY1_PREPARE_SPEED_SWITCH_BIT 0x01
#define MEM_KEY1_MODE_BIT 0x80

#define MEM_VBK_BANK_MASK 0x07

#define MEM_HDMA5_MODE_BIT 0x80
#define MEM_HDMA5_LENGTH_MASK 0x7F

#define MEM_HDMA_HBLANK_LENGTH 0x10

void mmu_reset(memory *mem);

#ifndef NDEBUG
void mmu_debug_print(memory *mem, level l);
#else
#define mmu_debug_print(r, l) ;
#endif

void mmu_set_keys(memory *mem, keys *k);

void mmu_set_rom(memory *mem, uint8_t *rom, mbc type, uint32_t rom_size);

void mmu_set_bootloader(memory *mem, uint8_t *bootloader);

uint8_t mmu_read_byte(memory *mem, const uint16_t address);

uint16_t mmu_read_word(memory *mem, const uint16_t address);

void mmu_write_byte(memory *mem, const uint16_t address, const uint8_t input);

void mmu_write_word(memory *mem, const uint16_t address, const uint16_t input);

#ifdef CGB
void mmu_hblank_dma(memory *mem);
#endif

#endif // MMU_H
