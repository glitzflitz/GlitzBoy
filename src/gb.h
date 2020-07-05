#pragma once

#include "defs.h"

struct Gameboy;

#define VBLANK_INTR 0x01
#define LCDC_INTR 0x02
#define TIMER_INTR 0x04
#define SERIAL_INTR 0x08
#define CONTROL_INTR 0x10
#define ANY_INTR 0x1F

#define WRAM_SIZE 0x2000
#define VRAM_SIZE 0x2000
#define HRAM_SIZE 0x0100
#define OAM_SIZE 0x00A0

#define ROM_0_ADDR 0x0000
#define ROM_N_ADDR 0x4000
#define VRAM_ADDR 0x8000
#define CART_RAM_ADDR 0xA000
#define WRAM_0_ADDR 0xC000
#define WRAM_1_ADDR 0xD000
#define ECHO_ADDR 0xE000
#define OAM_ADDR 0xFE00
#define UNUSED_ADDR 0xFEA0
#define IO_ADDR 0xFF00
#define HRAM_ADDR 0xFF80
#define INTR_EN_ADDR 0xFFFF

#define ROM_BANK_SIZE 0x4000
#define WRAM_BANK_SIZE 0x1000
#define CRAM_BANK_SIZE 0x2000
#define VRAM_BANK_SIZE 0x2000

#define DIV_CYCLES 256

#define SERIAL_CYCLES 4096

#define DMG_CLOCK_FREQ 4194304.0
#define SCREEN_REFRESH_CYCLES 70224.0
#define VERTICAL_SYNC (DMG_CLOCK_FREQ / SCREEN_REFRESH_CYCLES)

#define SERIAL_SC_TX_START 0x80
#define SERIAL_SC_CLOCK_SRC 0x01

#define STAT_LYC_INTR 0x40
#define STAT_MODE_2_INTR 0x20
#define STAT_MODE_1_INTR 0x10
#define STAT_MODE_0_INTR 0x08
#define STAT_LYC_COINC 0x04
#define STAT_MODE 0x03
#define STAT_USER_BITS 0xF8

#define LCDC_ENABLE 0x80
#define LCDC_WINDOW_MAP 0x40
#define LCDC_WINDOW_ENABLE 0x20
#define LCDC_TILE_SELECT 0x10
#define LCDC_BG_MAP 0x08
#define LCDC_OBJ_SIZE 0x04
#define LCDC_OBJ_ENABLE 0x02
#define LCDC_BG_ENABLE 0x01

#define LCD_LINE_CYCLES 456
#define LCD_MODE_0_CYCLES 0
#define LCD_MODE_2_CYCLES 204
#define LCD_MODE_3_CYCLES 284
#define LCD_VERT_LINES 154
#define LCD_WIDTH 160
#define LCD_HEIGHT 144

#define LCD_HBLANK 0
#define LCD_VBLANK 1
#define LCD_SEARCH_OAM 2
#define LCD_TRANSFER 3

#define VRAM_TILES_1 (0x8000 - VRAM_ADDR)
#define VRAM_TILES_2 (0x8800 - VRAM_ADDR)
#define VRAM_BMAP_1 (0x9800 - VRAM_ADDR)
#define VRAM_BMAP_2 (0x9C00 - VRAM_ADDR)
#define VRAM_TILES_3 (0x8000 - VRAM_ADDR + VRAM_BANK_SIZE)
#define VRAM_TILES_4 (0x8800 - VRAM_ADDR + VRAM_BANK_SIZE)

#define VBLANK_INTR_ADDR 0x0040
#define LCDC_INTR_ADDR 0x0048
#define TIMER_INTR_ADDR 0x0050
#define SERIAL_INTR_ADDR 0x0058
#define CONTROL_INTR_ADDR 0x0060

#define NUM_SPRITES 0x28
#define MAX_SPRITES_LINE 0x0A
#define OBJ_PRIORITY 0x80
#define OBJ_FLIP_Y 0x40
#define OBJ_FLIP_X 0x20
#define OBJ_PALETTE 0x10

#define ROM_HEADER_HASH_LOC 0x014D

#define LCD_COLOUR 0x03
#define LCD_PALETTE_OBJ 0x10
#define LCD_PALETTE_BG 0x20
#define LCD_PALETTE_ALL 0x30

typedef struct Timer
{
	uf16 lcd_count;
	uf16 div_count;
	uf16 tima_count;
	uf16 serial_count;
} Timer;

typedef struct Registers
{
	union
	{
		struct
		{
			union
			{
				struct
				{
					u8 padding : 4; /*Unused */
					u8 C : 1;		/* Carry flag. */
					u8 H : 1;		/* Half carry flag. */
					u8 N : 1;		/* Sign/Negative flag. */
					u8 Z : 1;		/* Zero flag. */
				} raw_bits;
				u8 f;
			};
			u8 a;
		};
		u16 AF;
	};

	union
	{
		struct
		{
			u8 C;
			u8 B;
		};
		u16 BC;
	};

	union
	{
		struct
		{
			u8 E;
			u8 D;
		};
		u16 DE;
	};

	union
	{
		struct
		{
			u8 L;
			u8 H;
		};
		u16 HL;
	};

	u16 SP;
	u16 PC;
} Registers;

typedef struct hw_registers
{
	u8 TIMA, TMA, DIV;
	union
	{
		struct
		{
			u8 rate : 2;
			u8 enable : 1;
			u8 padding : 5;
		};
		u8 TAC;
	};

	/* LCD */
	u8 LCDC;
	u8 STAT;
	u8 SCY;
	u8 SCX;
	u8 LY;
	u8 LYC;
	u8 DMA;
	u8 BGP;
	u8 OBP0;
	u8 OBP1;
	u8 WY;
	u8 WX;

	u8 P1;

	u8 SB;
	u8 SC;

	u8 IF;

	u8 IE;
} hw_registers;

typedef struct Display
{
	void (*gpu_draw_line)(struct Gameboy *,
						  const u8 pixels[static 160],
						  const uf8 line);

	u8 bg_palette[4];
	u8 sp_palette[8];

	u8 window_clear;
	u8 WY;

	u32 frame_skip_count : 1;
	u32 interlace_count : 1;
} Display;

enum Error
{
	UNKNOWN_ERROR,
	INVALID_OPCODE,
	INVALID_READ,
	INVALID_WRITE,

	INVALID_MAX
};

enum InitError
{
	INIT_NO_ERROR,
	INIT_CARTRIDGE_UNSUPPORTED,
	INIT_INVALID_HASH
};

enum SerialStatus
{
	SERIAL_SUCCESS = 0,
	SERIAL_NO_CONNECTION = 1
};

typedef struct Gameboy
{
	u8 (*read_rom)(struct Gameboy *, const uint_fast32_t address);

	u8 (*read_ram)(struct Gameboy *, const uint_fast32_t address);

	void (*write_ram)(struct Gameboy *, const uint_fast32_t address,
					  const u8 value);

	void (*Error)(struct Gameboy *, const enum Error, const u16 value);

	u8 (*serial_transmit)(struct Gameboy *, const u8 tx);
	enum SerialStatus (*serial_recv)(struct Gameboy *, u8 *rx);

	struct
	{
		unsigned char halt : 1;
		unsigned char ime : 1;
		unsigned char frame : 1;

		unsigned char lcd_mode : 2;
	};

	u8 mbc;
	u8 cartridge_ram;
	u16 num_rom_banks;
	u8 num_ram_banks;

	u8 selected_rom_bank;
	u8 cart_ram_bank;
	u8 enable_cart_ram;
	u8 cart_mode_select;
	union
	{
		struct
		{
			u8 sec;
			u8 min;
			u8 hour;
			u8 yday;
			u8 high;
		} raw_bits;
		u8 cart_rtc[5];
	};

	Registers cpu_reg;
	struct hw_registers hw_reg;
	Timer timer;
	Display display;

	u8 wram[WRAM_SIZE];
	u8 vram[VRAM_SIZE];
	u8 hram[HRAM_SIZE];
	u8 oam[OAM_SIZE];

	struct
	{
		u32 interlace : 1;
		u32 skipframe : 1;

		union
		{
			struct
			{
				u32 a : 1;
				u32 b : 1;
				u32 select : 1;
				u32 start : 1;
				u32 right : 1;
				u32 left : 1;
				u32 up : 1;
				u32 down : 1;
			} raw_bits;
			u8 joypad;
		};

		void *misc_data;
	} direct;
} Gameboy;
