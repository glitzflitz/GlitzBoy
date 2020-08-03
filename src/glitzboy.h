#pragma once

#include <time.h>
#include "defs.h"
#include "mmu.h"
#include "cpu.h"
#include "gpu.h"
#include "timer.h"
#include "apu.h"

void gb_reset(Gameboy *gb)
{
	gb->halt = 0;
	gb->ime = 1;
	gb->lcd_mode = LCD_HBLANK;

	gb->selected_rom_bank = 1;
	gb->cart_ram_bank = 0;
	gb->enable_cart_ram = 0;
	gb->cart_mode_select = 0;

	gb->cpu_reg.AF = 0x01B0;
	gb->cpu_reg.BC = 0x0013;
	gb->cpu_reg.DE = 0x00D8;
	gb->cpu_reg.HL = 0x014D;
	gb->cpu_reg.SP = 0xFFFE;
	gb->cpu_reg.PC = 0x0100;

	gb->timer.lcd_count = 0;
	gb->timer.div_count = 0;
	gb->timer.tima_count = 0;
	gb->timer.serial_count = 0;

	gb->hw_reg.TIMA = 0x00;
	gb->hw_reg.TMA = 0x00;
	gb->hw_reg.TAC = 0xF8;
	gb->hw_reg.DIV = 0xAC;

	gb->hw_reg.IF = 0xE1;

	gb->hw_reg.LCDC = 0x91;
	gb->hw_reg.SCY = 0x00;
	gb->hw_reg.SCX = 0x00;
	gb->hw_reg.LYC = 0x00;

	gb->hw_reg.SC = 0x7E;
	gb->hw_reg.STAT = 0;
	gb->hw_reg.LY = 0;

	write_byte(gb, 0xFF47, 0xFC);
	write_byte(gb, 0xFF48, 0xFF);
	write_byte(gb, 0xFF49, 0x0F);
	gb->hw_reg.WY = 0x00;
	gb->hw_reg.WX = 0x00;
	gb->hw_reg.IE = 0x00;

	gb->direct.joypad = 0xFF;
	gb->hw_reg.P1 = 0xCF;
}

enum InitError gb_init(struct Gameboy *gb,
					   u8 (*read_rom)(Gameboy *, const uf32),
					   u8 (*read_ram)(Gameboy *, const uf32),
					   void (*write_ram)(Gameboy *, const uf32, const u8),
					   void (*Error)(Gameboy *, const enum Error, const u16),
					   void *misc_data)
{
	const u16 mbc_location = 0x0147;
	const u16 bank_count_location = 0x0148;
	const u16 ram_size_location = 0x0149;
	const u8 mbc_type[] =
		{
			0, 1, 1, 1, -1, 2, 2, -1, 0, 0, -1, 0, 0, 0, -1, 3,
			3, 3, 3, 3, -1, -1, -1, -1, -1, 5, 5, 5, 5, 5, 5, -1};
	const u8 ram_banks[] =
		{
			0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0};
	const u16 rom_banks[] =
		{
			2, 4, 8, 16, 32, 64, 128, 256, 512, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 72, 80, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	const u8 num_ram_banks[] = {0, 1, 1, 4, 16, 8};

	gb->read_rom = read_rom;
	gb->read_ram = read_ram;
	gb->write_ram = write_ram;
	gb->Error = Error;
	gb->direct.misc_data = misc_data;

	gb->serial_transmit = NULL;
	gb->serial_recv = NULL;

	{
		u8 x = 0;

		for (u16 i = 0x0134; i <= 0x014C; i++)
			x = x - gb->read_rom(gb, i) - 1;

		if (x != gb->read_rom(gb, ROM_HEADER_HASH_LOC))
			return INIT_INVALID_HASH;
	}

	{
		const u8 mbc_value = gb->read_rom(gb, mbc_location);

		if (mbc_value > sizeof(mbc_type) - 1 ||
			(gb->mbc = mbc_type[gb->read_rom(gb, mbc_location)]) == 255u)
			return INIT_CARTRIDGE_UNSUPPORTED;
	}

	gb->num_ram_banks = ram_banks[gb->read_rom(gb, mbc_location)];
	gb->num_rom_banks = rom_banks[gb->read_rom(gb, bank_count_location)];
	gb->num_ram_banks = num_ram_banks[gb->read_rom(gb, ram_size_location)];

	gb->display.gpu_draw_line = NULL;

	gb_reset(gb);

	return INIT_NO_ERROR;
}
