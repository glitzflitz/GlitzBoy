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

void assign_palette(struct misc_data *misc_data, u8 selection)
{
#define NUMBER_OF_PALETTES 12
  size_t palette_bytes = 3 * 4 * sizeof(u16);

  switch (selection)
  {

  case 0:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x2BE0, 0x7D00, 0x0000},
                               {0x7FFF, 0x2BE0, 0x7D00, 0x0000},
                               {0x7FFF, 0x2BE0, 0x7D00, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 1:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                               {0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                               {0x7FFF, 0x7FE0, 0x7C00, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 2:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                               {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                               {0x7FFF, 0x7EAC, 0x40C0, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 3:
  {
    const u16 palette[3][4] = {{0x0000, 0x0210, 0x7F60, 0x7FFF},
                               {0x0000, 0x0210, 0x7F60, 0x7FFF},
                               {0x0000, 0x0210, 0x7F60, 0x7FFF}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  default:
  case 4:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x5294, 0x294A, 0x0000},
                               {0x7FFF, 0x5294, 0x294A, 0x0000},
                               {0x7FFF, 0x5294, 0x294A, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 5:
  {
    const u16 palette[3][4] = {{0x7FF4, 0x7E52, 0x4A5F, 0x0000},
                               {0x7FF4, 0x7E52, 0x4A5F, 0x0000},
                               {0x7FF4, 0x7E52, 0x4A5F, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 6:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                               {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                               {0x7F98, 0x6670, 0x41A5, 0x2CC1}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 7:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x3FE6, 0x0198, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 8:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                               {0x7FFF, 0x463B, 0x2951, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 9:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x3FE6, 0x0200, 0x0000},
                               {0x7FFF, 0x329F, 0x001F, 0x0000},
                               {0x7FFF, 0x7E10, 0x48E7, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 10:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                               {0x7FFF, 0x3FE6, 0x0200, 0x0000},
                               {0x7FFF, 0x329F, 0x001F, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }

  case 11:
  {
    const u16 palette[3][4] = {{0x7FFF, 0x329F, 0x001F, 0x0000},
                               {0x7FFF, 0x3FE6, 0x0200, 0x0000},
                               {0x7FFF, 0x7FE0, 0x3D20, 0x0000}};
    memcpy(misc_data->_palette, palette, palette_bytes);
    break;
  }
  }

  return;
}

void fb_draw_line(Gameboy *gb, const u8 pixels[160], const uint_least8_t line)
{
  struct misc_data *misc_data = gb->direct.misc_data;

  for (u32 x = 0; x < LCD_WIDTH; x++)
  {
    misc_data->fb[line][x] =
        misc_data->_palette[(pixels[x] & LCD_PALETTE_ALL) >> 4][pixels[x] & 3];
  }
}

int main(int argc, char **argv)
{
  Gameboy gb;
  struct misc_data misc_data = {.rom = NULL, .cartridgeram = NULL};
  const double target_speed_ms = 1000.0 / VERTICAL_SYNC;
  double speed_compensation = 0.0;
  u32 running = 1;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_Event event;
  SDL_GameController *controller = NULL;
  uf32 new_ticks, old_ticks;
  enum InitError gb_ret;
  u32 fast_mode = 1;
  u32 fast_mode_timer = 1;

  int save_timer = 60;

  char *rom_file_name = NULL;
  char *save_file_name = NULL;
  int ret = EXIT_SUCCESS;

  switch (argc)
  {
  case 2:

    rom_file_name = argv[1];
    break;

  case 3:

    rom_file_name = argv[1];
    save_file_name = argv[2];
    break;

  default:
    printf("Usage: %s ROM [SAVE]\n", argv[0]);
    puts("SAVE is set by default if not provided.");
    ret = EXIT_FAILURE;
    goto out;
  }

  if ((misc_data.rom = load_rom_into_ram(rom_file_name)) == NULL)
  {
    printf("%d: %s\n", __LINE__, strerror(errno));
    ret = EXIT_FAILURE;
    goto out;
  }

  if (save_file_name == NULL)
  {
    char *str_replace;
    const char extension[] = ".sav";

    save_file_name = malloc(strlen(rom_file_name) + strlen(extension) + 1);

    if (save_file_name == NULL)
    {
      printf("%d: %s\n", __LINE__, strerror(errno));
      ret = EXIT_FAILURE;
      goto out;
    }

    strcpy(save_file_name, rom_file_name);

    if ((str_replace = strrchr(save_file_name, '.')) == NULL ||
        str_replace == save_file_name)
      str_replace = save_file_name + strlen(save_file_name);

    for (u32 i = 0; i <= strlen(extension); i++)
      *(str_replace++) = extension[i];
  }

  gb_ret = gb_init(&gb, &read_rom, &read_ram, &write_ram, &Error, &misc_data);

  switch (gb_ret)
  {
  case INIT_NO_ERROR:
    break;

  case INIT_CARTRIDGE_UNSUPPORTED:
    puts("Unsupported cartridge.");
    ret = EXIT_FAILURE;
    goto out;

  case INIT_INVALID_HASH:
    puts("Invalid ROM: Checksum failure.");
    ret = EXIT_FAILURE;
    goto out;

  default:
    printf("Unknown error: %d\n", gb_ret);
    ret = EXIT_FAILURE;
    goto out;
  }

  load_cartridge_ram(save_file_name, &misc_data.cartridgeram,
                     get_save_size(&gb));

  {
    time_t rawtime;
    time(&rawtime);
#ifdef _POSIX_C_SOURCE
    struct tm timeinfo;
    localtime_r(&rawtime, &timeinfo);
#else
    struct tm *timeinfo;
    timeinfo = localtime(&rawtime);
#endif

#ifdef _POSIX_C_SOURCE
    set_rtc(&gb, &timeinfo);
#else
    set_rtc(&gb, timeinfo);
#endif
  }

  // Standard SDL boilerplate
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0)
  {
    printf("Could not initialise SDL: %s\n", SDL_GetError());
    ret = EXIT_FAILURE;
    goto out;
  }

  SDL_AudioDeviceID dev;

  {
    SDL_AudioSpec want, have;

    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_F32SYS, want.channels = 2;
    want.samples = AUDIO_SAMPLES;
    want.callback = audio_callback;
    want.userdata = NULL;

    printf("Audio driver: %s\n", SDL_GetAudioDeviceName(0, 0));

    if ((dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0)) == 0)
    {
      printf("Could not open audio device: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    audio_init();
    SDL_PauseAudioDevice(dev, 0);
  }

  init_gpu(&gb, &fb_draw_line);

  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

  if (SDL_GameControllerAddMappingsFromFile("src/controllerdb.dat") < 0)
  {
    printf("Unable to open controllerdb.dat: %s\n", SDL_GetError());
  }

  for (int i = 0; i < SDL_NumJoysticks(); i++)
  {
    if (!SDL_IsGameController(i))
      continue;

    controller = SDL_GameControllerOpen(i);

    if (controller)
    {
      printf("Game Controller %s connected.\n",
             SDL_GameControllerName(controller));
      break;
    }
    else
    {
      printf("Could not open game controller %i: %s\n", i, SDL_GetError());
    }
  }

  {

    char title_str[28] = "GlitzBoy: ";
    printf("ROM: %s\n", rom_title(&gb, title_str + 10));
    printf("MBC: %d\n", gb.mbc);

    window =
        SDL_CreateWindow(title_str, SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, LCD_WIDTH * 2, LCD_HEIGHT * 2,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

    if (window == NULL)
    {
      printf("Could not create window: %s\n", SDL_GetError());
      ret = EXIT_FAILURE;
      goto out;
    }
  }

  SDL_SetWindowMinimumSize(window, LCD_WIDTH, LCD_HEIGHT);

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  if (renderer == NULL)
  {
    printf("Could not create renderer: %s\n", SDL_GetError());
    ret = EXIT_FAILURE;
    goto out;
  }

  if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) < 0)
  {
    printf("Renderer could not draw color: %s\n", SDL_GetError());
    ret = EXIT_FAILURE;
    goto out;
  }

  if (SDL_RenderClear(renderer) < 0)
  {
    printf("Renderer could not clear: %s\n", SDL_GetError());
    ret = EXIT_FAILURE;
    goto out;
  }

  SDL_RenderPresent(renderer);

  SDL_RenderSetLogicalSize(renderer, LCD_WIDTH, LCD_HEIGHT);
  SDL_RenderSetIntegerScale(renderer, 1);

  texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB555,
                        SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);

  if (texture == NULL)
  {
    printf("Texture could not be created: %s\n", SDL_GetError());
    ret = EXIT_FAILURE;
    goto out;
  }

  auto_assign_palette(&misc_data, color_code(&gb));

  while (running)
  {
    int delay;
    static u32 rtc_timer = 0;
    static u32 selected_palette = 3;

    old_ticks = SDL_GetTicks();

    while (SDL_PollEvent(&event))
    {
      static int fullscreen = 0;

      switch (event.type)
      {
      case SDL_QUIT:
        running = 0;
        break;

      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERBUTTONUP:
        switch (event.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_A:
          gb.direct.raw_bits.a = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_B:
          gb.direct.raw_bits.b = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_BACK:
          gb.direct.raw_bits.select = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_START:
          gb.direct.raw_bits.start = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          gb.direct.raw_bits.up = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          gb.direct.raw_bits.right = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          gb.direct.raw_bits.down = !event.cbutton.state;
          break;

        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          gb.direct.raw_bits.left = !event.cbutton.state;
          break;
        }

        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_RETURN:
          gb.direct.raw_bits.start = 0;
          break;

        case SDLK_BACKSPACE:
          gb.direct.raw_bits.select = 0;
          break;

        case SDLK_z:
          gb.direct.raw_bits.a = 0;
          break;

        case SDLK_x:
          gb.direct.raw_bits.b = 0;
          break;

        case SDLK_UP:
          gb.direct.raw_bits.up = 0;
          break;

        case SDLK_RIGHT:
          gb.direct.raw_bits.right = 0;
          break;

        case SDLK_DOWN:
          gb.direct.raw_bits.down = 0;
          break;

        case SDLK_LEFT:
          gb.direct.raw_bits.left = 0;
          break;

        case SDLK_SPACE:
          fast_mode = 2;
          break;

        case SDLK_1:
          fast_mode = 1;
          break;

        case SDLK_2:
          fast_mode = 2;
          break;

        case SDLK_3:
          fast_mode = 3;
          break;

        case SDLK_4:
          fast_mode = 4;
          break;

        case SDLK_r:
          gb_reset(&gb);
          break;

        case SDLK_i:
          gb.direct.interlace = ~gb.direct.interlace;
          break;

        case SDLK_o:
          gb.direct.skipframe = ~gb.direct.skipframe;
          break;

        case SDLK_p:
          if (event.key.keysym.mod == KMOD_LSHIFT)
          {
            auto_assign_palette(&misc_data, color_code(&gb));
            break;
          }

          if (++selected_palette == NUMBER_OF_PALETTES)
            selected_palette = 0;

          assign_palette(&misc_data, selected_palette);
          break;
        }

        break;

      case SDL_KEYUP:
        switch (event.key.keysym.sym)
        {
        case SDLK_RETURN:
          gb.direct.raw_bits.start = 1;
          break;

        case SDLK_BACKSPACE:
          gb.direct.raw_bits.select = 1;
          break;

        case SDLK_z:
          gb.direct.raw_bits.a = 1;
          break;

        case SDLK_x:
          gb.direct.raw_bits.b = 1;
          break;

        case SDLK_UP:
          gb.direct.raw_bits.up = 1;
          break;

        case SDLK_RIGHT:
          gb.direct.raw_bits.right = 1;
          break;

        case SDLK_DOWN:
          gb.direct.raw_bits.down = 1;
          break;

        case SDLK_LEFT:
          gb.direct.raw_bits.left = 1;
          break;

        case SDLK_SPACE:
          fast_mode = 1;
          break;

        case SDLK_f:
          if (fullscreen)
          {
            SDL_SetWindowFullscreen(window, 0);
            fullscreen = 0;
            SDL_ShowCursor(SDL_ENABLE);
          }
          else
          {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            fullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
            SDL_ShowCursor(SDL_DISABLE);
          }
          break;

        case SDLK_F11:
        {
          if (fullscreen)
          {
            SDL_SetWindowFullscreen(window, 0);
            fullscreen = 0;
            SDL_ShowCursor(SDL_ENABLE);
          }
          else
          {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            fullscreen = SDL_WINDOW_FULLSCREEN;
            SDL_ShowCursor(SDL_DISABLE);
          }
        }
        break;
        }

        break;
      }
    }

    run_cpu(&gb);

    rtc_timer += target_speed_ms / fast_mode;

    if (rtc_timer >= 1000)
    {
      rtc_timer -= 1000;
      tick(&gb);
    }

    if (fast_mode_timer > 1)
    {
      fast_mode_timer--;

      continue;
    }

    fast_mode_timer = fast_mode;

    SDL_UpdateTexture(texture, NULL, &misc_data.fb, LCD_WIDTH * sizeof(u16));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    new_ticks = SDL_GetTicks();

    speed_compensation += target_speed_ms - (new_ticks - old_ticks);

    delay = (int)(speed_compensation);

    speed_compensation -= delay;

    /* Absolutely do not touch this timer code.
      It WILL mess things up
    */

    if (delay > 0)
    {
      uf32 delay_ticks = SDL_GetTicks();
      uf32 after_delay_ticks;

      rtc_timer += delay;

      if (rtc_timer >= 1000)
      {
        rtc_timer -= 1000;
        tick(&gb);

        --save_timer;

        if (!save_timer)
        {
          load_cartridge_ram(save_file_name, &misc_data.cartridgeram,
                             get_save_size(&gb));
          save_timer = 60;
        }
      }

      SDL_Delay(delay);

      after_delay_ticks = SDL_GetTicks();
      speed_compensation +=
          (double)delay - (int)(after_delay_ticks - delay_ticks);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_DestroyTexture(texture);
  SDL_GameControllerClose(controller);
  SDL_Quit();

  write_cartridge_ram(save_file_name, &misc_data.cartridgeram,
                      get_save_size(&gb));

out:
  free(misc_data.rom);
  free(misc_data.cartridgeram);

  if (argc == 2)
    free(save_file_name);

  if (argc == 1)
    free(rom_file_name);

  return ret;
}
