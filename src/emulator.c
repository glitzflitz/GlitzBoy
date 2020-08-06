#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "glitzboy.h"

struct misc_data
{

  u8 *rom;

  u8 *cartridgeram;

  u16 _palette[3][4];
  u16 fb[LCD_HEIGHT][LCD_WIDTH];
};

u8 read_rom(Gameboy *gb, const uf32 address)
{
  const struct misc_data *const p = gb->direct.misc_data;
  return p->rom[address];
}

u8 read_ram(Gameboy *gb, const uf32 address)
{
  const struct misc_data *const p = gb->direct.misc_data;
  return p->cartridgeram[address];
}

void write_ram(Gameboy *gb, const uf32 address, const u8 value)
{
  const struct misc_data *const p = gb->direct.misc_data;
  p->cartridgeram[address] = value;
}

u8 *load_rom_into_ram(const char *file_name)
{
  FILE *romfile = fopen(file_name, "rb");
  size_t size;
  u8 *rom = NULL;

  if (romfile == NULL)
    return NULL;

  fseek(romfile, 0, SEEK_END);
  size = ftell(romfile);
  rewind(romfile);
  rom = malloc(size);

  if (fread(rom, sizeof(u8), size, romfile) != size)
  {
    free(rom);
    fclose(romfile);
    return NULL;
  }

  fclose(romfile);
  return rom;
}

void load_cartridge_ram(const char *save_filename, u8 **dest,
                        const size_t len)
{
  FILE *f;

  if (len == 0)
  {
    *dest = NULL;
    return;
  }

  if ((*dest = malloc(len)) == NULL)
  {
    printf("%d: %s\n", __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  f = fopen(save_filename, "rb");

  if (f == NULL)
  {
    memset(*dest, 0xFF, len);
    return;
  }

  fread(*dest, sizeof(u8), len, f);
  fclose(f);
}

void write_cartridge_ram(const char *save_file_name, u8 **dest,
                         const size_t len)
{
  FILE *f;

  if (len == 0 || *dest == NULL)
    return;

  if ((f = fopen(save_file_name, "wb")) == NULL)
  {
    puts("Unable to open save file.");
    printf("%d: %s\n", __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  fwrite(*dest, sizeof(u8), len, f);
  fclose(f);
}

void Error(Gameboy *gb, const enum Error gb_err, const u16 value)
{
  struct misc_data *misc_data = gb->direct.misc_data;

  switch (gb_err)
  {
  case INVALID_OPCODE:

    fprintf(stdout, "Invalid opcode %#04x at PC: %#06x, SP: %#06x\n", value,
            gb->cpu_reg.PC - 1, gb->cpu_reg.SP);
    break;

  case INVALID_WRITE:
  case INVALID_READ:
    return;

  default:
    printf("Unknown error");
    break;
  }

  fprintf(stderr, "Error. Press q to exit.");

  if (getchar() == 'q')
  {

    write_cartridge_ram("recovery.sav", &misc_data->cartridgeram,
                        get_save_size(gb));

    free(misc_data->rom);
    free(misc_data->cartridgeram);
    exit(EXIT_FAILURE);
  }

  return;
}
/*
 * TODO: Not all color codes are programmed in yet because I'm lazy.
 */
void auto_assign_palette(struct misc_data *misc_data, u8 game_checksum)
{
  size_t palette_bytes = 3 * 4 * sizeof(u16);

  switch (game_checksum)
  {
  case 0x01:
  case 0x10:
  case 0x29:
  case 0x52:
  case 0x5D:
  case 0x68:
  case 0x6D:
  case 0xF6:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x329F, 0x001F, 0x0000},
                               {0x7FFF, 0x3FE6, 0x0200, 0x0000},
                               {0x7FFF, 0x7EAC, 0x40C0, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x14:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x3FE6, 0x0200, 0x0000},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x15:
  case 0xDB:
  case 0x95:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                               {0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                               {0x7FFF, 0x7FE0, 0x7C00, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x19:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x7E60, 0x7C00, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x71:
  case 0xFF:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E60, 0x7C00, 0x0000},
                               {0x7FFF, 0x7E60, 0x7C00, 0x0000},
                               {0x7FFF, 0x7E60, 0x7C00, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x61:
  case 0x45:

  case 0xD8:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x329F, 0x001F, 0x0000},
                               {0x7FFF, 0x329F, 0x001F, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x8B:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x329F, 0x001F, 0x0000},
                               {0x7FFF, 0x3FE6, 0x0200, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x27:
  case 0x49:
  case 0x5C:
  case 0xB3:
  {
    const u16 palette[3][4] = {{0x7D8A, 0x6800, 0x3000, 0x0000},
                               {0x001F, 0x7FFF, 0x7FEF, 0x021F},
                               {0x527F, 0x7FE0, 0x0180, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x18:
  case 0x6A:
  case 0x4B:
  case 0x6B:
  {
    const u16 palette[3][4] = {{0x7F08, 0x7F40, 0x48E0, 0x2400},
                               {0x7FFF, 0x2EFF, 0x7C00, 0x001F},
                               {0x7FFF, 0x463B, 0x2951, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 0x70:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x03E0, 0x1A00, 0x0120},
                               {0x7FFF, 0x329F, 0x001F, 0x001F},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  default:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x5294, 0x294A, 0x0000},
                               {0x7FFF, 0x5294, 0x294A, 0x0000},
                               {0x7FFF, 0x5294, 0x294A, 0x0000}};
    printf("No palette found for 0x%02X.\n", game_checksum);
    memcpy(misc_data->_palette, palette, palette_bytes);
  }
  }
}
