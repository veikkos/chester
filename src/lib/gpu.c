#include "gpu.h"
#include "interrupts.h"
#include "logger.h"

#include <stdbool.h>
#include <string.h>

int gpu_init(gpu *g, gpu_init_cb cb)
{
  g->app_data = NULL;
  g->locked_pixel_data = NULL;

  if (!cb(g))
    {
      return 0;
    }

  gpu_reset(g);

  return 1;
}

void gpu_reset(gpu *g)
{
  g->clock.t = 0;
}

#ifndef NDEBUG
void gpu_debug_print(gpu *g, level l)
{
  gb_log(l, "GPU info");
  gb_log(l, " - clock: %d", g->clock.t);
}
#endif

#ifdef EXPERIMENTAL_CGB
static inline uint8_t convert_color(uint8_t color)
{
  return (uint8_t)((uint32_t)(color) * 0xFF / 0x1F);
}

static inline const uint8_t *get_color(const unsigned int raw_color,
                                       const uint8_t *palette)
{
  static uint8_t colors[3];
  const uint16_t p_colors = palette[raw_color * 2] + (palette[raw_color * 2 + 1] << 8);

  colors[2] = convert_color(p_colors & 0x1F);
  colors[1] = convert_color((p_colors >> 5) & 0x1F);
  colors[0] = convert_color((p_colors >> 10) & 0x1F);

  return colors;
}

static inline uint8_t *get_palette_address(memory *mem, const uint8_t palette_index, const uint8_t bg_palette_num)
{
  return &mem->palette[palette_index][bg_palette_num * 8];
}
#endif

static inline const uint8_t *get_mono_color(const unsigned int raw_color,
                                            const unsigned int palette)
{
  static const uint8_t colors[] = { 255, 255, 255, 192, 192, 192, 96, 96, 96, 0, 0, 0 };
  return &colors[((palette >> (raw_color * 2)) & 0x03) * 3];
}

void write_texture(uint8_t line,
                   int16_t x,
                   uint16_t raw_tile_data,
                   uint8_t *texture,
                   uint8_t *row_data,
                   bool transparent,
                   bool priority,
                   bool x_flip,
                   uint8_t mono_palette
#ifdef EXPERIMENTAL_CGB
                   , const uint8_t *color_palette
#endif
    )
{
  int i;

  for(i = 0; i < 8; ++i)
    {
      const unsigned int bit_offset = x_flip ? i : (7 - i);
      int16_t x_position = x + bit_offset;

      if (x_position < 0)
        {
          x_position += 256;
        }
      else if (x_position >= 256)
        {
          x_position -= 256;
        }

      if (x_position < 160)
        {
          const unsigned int raw_color_1 = (raw_tile_data & (0x0001 << i)) >> i;
          const unsigned int raw_color_2 = (raw_tile_data & (0x0100 << i)) >> (i + 7);
          const unsigned int raw_color = raw_color_1 + raw_color_2;
          const uint8_t *color =
#ifdef EXPERIMENTAL_CGB
              color_palette ?
              get_color(raw_color, color_palette) :
#endif
              get_mono_color(raw_color, mono_palette);

          static const unsigned int per_line_offset = 256 * 4;
          const unsigned int line_offset = per_line_offset * line;
          const unsigned int output_pixel_offset =
            line_offset + (x_position * 4);

          if (transparent)
            {
              if (raw_color &&
                  (priority ||
                   (!row_data[x_position])))
                {
                  memcpy(texture + output_pixel_offset, color, 3);
                  texture[output_pixel_offset + 3] = 255;
                  if (row_data)
                    {
                      row_data[x_position] = (uint8_t)raw_color;
                    }
                }
            }
          else
            {
              memcpy(texture + output_pixel_offset, color, 3);
              texture[output_pixel_offset + 3] = 255;
              if (row_data)
                {
                  row_data[x_position] = (uint8_t)raw_color;
                }
            }
        }
    }
}

static inline void process_background_tiles(memory *mem,
                                            const uint8_t line,
                                            uint8_t line_data_offset,
                                            const uint16_t tile_map_addr,
                                            const uint16_t tile_data_addr,
                                            const int16_t offset,
                                            uint8_t *output,
                                            uint8_t *row_data)
{
  size_t tile_pos;
  uint16_t tile_data = 0;

  uint16_t input_line = line + line_data_offset;
  if (input_line >= 256)
    input_line -= 256;
  const uint8_t line_offset = input_line / 8;
  const uint16_t tile_base_addr = tile_data_addr + ((input_line % 8) * 2);
  uint16_t addr =
    tile_map_addr + (line_offset * 32);
#ifdef EXPERIMENTAL_CGB
  const uint16_t base_addr = addr;
  const uint8_t* color_palette = NULL;
  bool horizontal_flip = false;
  uint8_t tile_vram_bank_number = 0;
#endif

  const uint8_t mono_palette =
#ifdef EXPERIMENTAL_CGB
    mem->cgb_mode ? 0 :
#endif
    mmu_read_byte(mem, MEM_BGP_ADDR);

  for(tile_pos=0; tile_pos<32; ++tile_pos)
    {
      uint8_t id = mem->video_ram
#ifdef EXPERIMENTAL_CGB
        [MEM_CHARACTER_CODE_BANK_INDEX]
#endif
        [addr++ - 0x8000];

      // TODO: fix properly
      if (tile_data_addr == MEM_TILE_ADDR_1)
        id += 128;

#ifdef EXPERIMENTAL_CGB
      if (mem->cgb_mode)
        {
          const uint8_t bg_map = mem->video_ram[MEM_ATTRIBUTES_CODE_BANK_INDEX][base_addr - 0x8000 + tile_pos];
          const uint8_t palette_num = bg_map & PALETTE_NUM_MASK;
          tile_vram_bank_number = (bg_map >> 3) & 0x01;
          horizontal_flip = (bg_map >> 5) & 0x01;

          static bool vertical_flip_logged = false;

          if (!vertical_flip_logged)
            {
              const bool vertical_flip = (bg_map >> 6) & 0x01;

              if (vertical_flip)
                {
                  gb_log(WARNING, "BG tile vertical flip not supported");
                }

              vertical_flip_logged = true;
            }

          color_palette = color_palette = get_palette_address(mem, MEM_PALETTE_BG_INDEX, palette_num);
        }
#endif

      const uint16_t tile_base_addr_final = tile_base_addr + (id * 16) - 0x8000;
      tile_data = 0;

      for (size_t i = 0; i < 2; ++i)
        {
          tile_data += mem->video_ram
#ifdef EXPERIMENTAL_CGB
            [tile_vram_bank_number]
#endif
            [tile_base_addr_final + i] << (i * 8);
        }

      write_texture(line,
                    (uint16_t)(tile_pos * 8 + offset),
                    tile_data,
                    output,
                    row_data,
                    false,
                    true,
#ifdef EXPERIMENTAL_CGB
                    horizontal_flip,
#else
                    false,
#endif
                    mono_palette
#ifdef EXPERIMENTAL_CGB
                    , color_palette
#endif
      );
    }
}

static inline void process_sprite_attributes(memory *mem,
                                             const uint8_t line,
                                             uint16_t attribute_map_addr,
                                             const bool high,
                                             const uint16_t sprite_data_addr,
                                             uint8_t *output,
                                             uint8_t *row_data)
{
  const int number_of_sprites = 40;
  const unsigned int sprite_attributes_len = 4;
  int i;
  uint16_t tile_data;
  const uint8_t heigth = high ? 16 : 8;
  struct{
    struct{
      int16_t x, y;
    }pos;
    uint8_t pattern, flags;
  }sprite_attributes;
  // Read sprites back to front to get priority correct
  attribute_map_addr += number_of_sprites * sprite_attributes_len - 1;
#ifdef EXPERIMENTAL_CGB
  const uint8_t* color_palette = NULL;
#endif

  for(i=0; i<number_of_sprites; ++i)
    {
      sprite_attributes.flags = mmu_read_byte(mem, attribute_map_addr--);
      sprite_attributes.pattern = mmu_read_byte(mem, attribute_map_addr--);
      sprite_attributes.pos.x = mmu_read_byte(mem, attribute_map_addr--);
      sprite_attributes.pos.y = mmu_read_byte(mem, attribute_map_addr--);

      if (sprite_attributes.pos.x || sprite_attributes.pos.y)
        {
          sprite_attributes.pos.x -= 8;
          sprite_attributes.pos.y -= 16;

          if (line >= sprite_attributes.pos.y &&
              line < (sprite_attributes.pos.y + heigth))
            {
              const bool priority = !(sprite_attributes.flags & OBJ_PRIORITY_FLAG);
              const uint16_t mono_palette_address =
                sprite_attributes.flags & OBJ_PALETTE_FLAG ?
                MEM_OBP1_ADDR : MEM_OBP0_ADDR;
              uint8_t mono_palette = 0;
              const bool x_flip = sprite_attributes.flags & OBJ_X_FLIP_FLAG;
              const bool y_flip = sprite_attributes.flags & OBJ_Y_FLIP_FLAG;
              uint8_t sprite_line = (line - sprite_attributes.pos.y) % heigth;

#ifdef EXPERIMENTAL_CGB
              if (mem->cgb_mode)
                {
                  const uint8_t palette_num = sprite_attributes.flags & PALETTE_NUM_MASK;
                  color_palette = get_palette_address(mem, MEM_PALETTE_SPRITE_INDEX, palette_num);
                }
              else
                {
#endif
                  mono_palette = mmu_read_byte(mem, mono_palette_address);
#ifdef EXPERIMENTAL_CGB
                }
#endif

              if (y_flip)
                {
                  sprite_line = (heigth - 1) - sprite_line;
                }

              tile_data = mmu_read_word(mem,
                                        sprite_data_addr +
                                        (sprite_attributes.pattern * 16) +
                                        (sprite_line * 2));

              write_texture(line,
                            sprite_attributes.pos.x,
                            tile_data,
                            output,
                            row_data,
                            true,
                            priority,
                            x_flip,
                            mono_palette
#ifdef EXPERIMENTAL_CGB
                            , color_palette
#endif
              );
            }
        }
    }
}

static inline void scanline(gpu *g, memory *mem, const uint8_t line, gpu_lock_texture_cb cb)
{
  if (!g->locked_pixel_data)
    {
      cb(g);
    }

  if (g->locked_pixel_data)
    {
      const uint8_t lcdc = mmu_read_byte(mem, MEM_LCDC_ADDR);
      uint8_t row[160];
      memset(row, 0, sizeof(row));

      if (lcdc & MEM_LCDC_BG_WINDOW_ENABLED_FLAG)
        {
          const uint16_t tile_map_address =
            lcdc & MEM_LCDC_TILEMAP_SELECT_FLAG ?
            MEM_TILE_MAP_ADDR_2 :
            MEM_TILE_MAP_ADDR_1;

          const uint16_t tile_data_address =
            lcdc & MEM_LCDC_TILEMAP_DATA_FLAG ?
            MEM_TILE_ADDR_2 :
            MEM_TILE_ADDR_1;

          process_background_tiles(mem,
                                   line,
                                   mmu_read_byte(mem, MEM_SCY_ADDR),
                                   tile_map_address,
                                   tile_data_address,
                                   (int16_t)mmu_read_byte(mem, MEM_SCX_ADDR) *
                                   -1,
                                   g->locked_pixel_data,
                                   row);
        }

      if (lcdc & MEM_LCDC_BG_WINDOW_ENABLED_FLAG &&
          lcdc & MEM_LCDC_WINDOW_ENABLED_FLAG)
        {
          const uint8_t window_line = mmu_read_byte(mem, MEM_WY_ADDR);

          if (line >= window_line)
            {
              const uint16_t tile_map_address =
              lcdc & MEM_LCDC_WINDOW_TILEMAP_SELECT_FLAG ?
              MEM_TILE_MAP_ADDR_2 :
              MEM_TILE_MAP_ADDR_1;

              const uint16_t tile_data_address =
              lcdc & MEM_LCDC_TILEMAP_DATA_FLAG ?
              MEM_TILE_ADDR_2 :
              MEM_TILE_ADDR_1;

              process_background_tiles(mem,
                                       line,
                                       256 - mmu_read_byte(mem, MEM_WY_ADDR),
                                       tile_map_address,
                                       tile_data_address,
                                       (int16_t)mmu_read_byte(mem, MEM_WX_ADDR)
                                       - 7,
                                       g->locked_pixel_data,
                                       row);
            }
        }

      if (lcdc & MEM_LCDC_SPRITES_ENABLED_FLAG)
        {
          process_sprite_attributes(mem,
                                    line,
                                    MEM_SPRITE_ATTRIBUTE_TABLE,
                                    lcdc & MEM_LCDC_SPRITES_SIZE_FLAG,
                                    MEM_SPRITE_ADDR,
                                    g->locked_pixel_data,
                                    row);
        }
    }
}

static state get_mode(memory *mem)
{
  const uint8_t stat = mmu_read_byte(mem, MEM_LCD_STAT);
  return (state)(stat & LCD_STAT_MODE_MASK);
}

static void set_mode(memory *mem, const state mode)
{
  uint8_t stat = mmu_read_byte(mem, MEM_LCD_STAT);
  stat &= ~LCD_STAT_MODE_MASK;
  stat |= mode;
  mem->io_registers[MEM_LCD_STAT & 0x00FF] = stat;
}

int gpu_update(gpu *g, memory *mem, const uint8_t last_t, gpu_render_cb r_cb, gpu_lock_texture_cb l_cb)
{
  // Reset GPU state if LCD got disabled
  if (mem->lcd_stopped)
    {
      mem->lcd_stopped = false;

      g->clock.t = 0;

      set_mode(mem, HBLANK);
    }

  const uint8_t lcdc = mmu_read_byte(mem, MEM_LCDC_ADDR);
  if (!(lcdc & MEM_LCDC_SCREEN_ENABLED_FLAG))
    return 0;

  g->clock.t += last_t;

  uint8_t line = mmu_read_byte(mem, MEM_LY_ADDR);

  switch (get_mode(mem))
    {
    case READ_OAM:
      if (g->clock.t >= READ_OAM_CYCLES)
        {
          g->clock.t = 0;

          set_mode(mem, READ_VRAM);

          isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_OAM_ISR_ENABLED_FLAG);

          gb_log (VERBOSE, "GPU VRAM");
        }
      break;
    case READ_VRAM:
      if (g->clock.t >= READ_VRAM_CYCLES)
        {
          g->clock.t = 0;

          scanline(g, mem, line, l_cb);

          isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_HBLANK_ISR_ENABLED_FLAG);

          set_mode(mem, HBLANK);

          gb_log (VERBOSE, "GPU HBLANK (line %d)", line);
        }
      break;
    case HBLANK:
      if (g->clock.t >= HBLANK_CYCLES)
        {
          g->clock.t = 0;

          if (++line == 144)
            {
              isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_VBLANK_ISR_ENABLED_FLAG);

              set_mode(mem, VBLANK);

              isr_set_if_flag(mem, MEM_IF_VBLANK_FLAG);

              gb_log (VERBOSE, "GPU VBLANK");
            }
          else
            {
              isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_OAM_ISR_ENABLED_FLAG);

              set_mode(mem, READ_OAM);

              gb_log (VERBOSE, "GPU OAM");
            }

          mem->io_registers[MEM_LY_ADDR & 0x00FF] = line;
          isr_compare_ly_lyc(mem, line, mmu_read_byte(mem, MEM_LYC_ADDR));
        }
      break;
    case VBLANK:
      if (g->clock.t >= VBLANK_CYCLES)
        {
          g->clock.t = 0;

          if (++line == 154)
            {
              line = 0;

              set_mode(mem, READ_OAM);

              isr_set_lcdc_isr_if_enabled(mem, MEM_LCDC_OAM_ISR_ENABLED_FLAG);

              r_cb(g);

              gb_log (VERBOSE, "GPU RENDER - OAM");
            }

          mem->io_registers[MEM_LY_ADDR & 0x00FF] = line;
          isr_compare_ly_lyc(mem, line, mmu_read_byte(mem, MEM_LYC_ADDR));
        }
      break;
    }

  return 0;
}
