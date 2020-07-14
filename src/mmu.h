#pragma once

#include "gb.h"
#include "apu.h"

u8 read_byte(Gameboy *gb, const uf16 address)
{
	switch (address >> 12)
	{
	case 0x0:

	case 0x1:
	case 0x2:
	case 0x3:
		return gb->read_rom(gb, address);

	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		if (gb->mbc == 1 && gb->cart_mode_select)
			return gb->read_rom(gb,
								address + ((gb->selected_rom_bank & 0x1F) - 1) * ROM_BANK_SIZE);
		else
			return gb->read_rom(gb, address + (gb->selected_rom_bank - 1) * ROM_BANK_SIZE);

	case 0x8:
	case 0x9:
		return gb->vram[address - VRAM_ADDR];

	case 0xA:
	case 0xB:
		if (gb->cartridge_ram && gb->enable_cart_ram)
		{
			if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
				return gb->cart_rtc[gb->cart_ram_bank - 0x08];
			else if ((gb->cart_mode_select || gb->mbc != 1) &&
					 gb->cart_ram_bank < gb->num_ram_banks)
			{
				return gb->read_ram(gb, address - CART_RAM_ADDR +
											(gb->cart_ram_bank * CRAM_BANK_SIZE));
			}
			else
				return gb->read_ram(gb, address - CART_RAM_ADDR);
		}

		return 0;

	case 0xC:
		return gb->wram[address - WRAM_0_ADDR];

	case 0xD:
		return gb->wram[address - WRAM_1_ADDR + WRAM_BANK_SIZE];

	case 0xE:
		return gb->wram[address - ECHO_ADDR];

	case 0xF:
		if (address < OAM_ADDR)
			return gb->wram[address - ECHO_ADDR];

		if (address < UNUSED_ADDR)
			return gb->oam[address - OAM_ADDR];

		if (address < IO_ADDR)
			return 0xFF;

		if (HRAM_ADDR <= address && address < INTR_EN_ADDR)
			return gb->hram[address - HRAM_ADDR];

		if ((address >= 0xFF10) && (address <= 0xFF3F))
		{
			return audio_read(address);
		}

		switch (address & 0xFF)
		{

		case 0x00:
			return 0xC0 | gb->hw_reg.P1;

		case 0x01:
			return gb->hw_reg.SB;

		case 0x02:
			return gb->hw_reg.SC;

		case 0x04:
			return gb->hw_reg.DIV;

		case 0x05:
			return gb->hw_reg.TIMA;

		case 0x06:
			return gb->hw_reg.TMA;

		case 0x07:
			return gb->hw_reg.TAC;

		case 0x0F:
			return gb->hw_reg.IF;

		case 0x40:
			return gb->hw_reg.LCDC;

		case 0x41:
			return (gb->hw_reg.STAT & STAT_USER_BITS) |
				   (gb->hw_reg.LCDC & LCDC_ENABLE ? gb->lcd_mode : LCD_VBLANK);

		case 0x42:
			return gb->hw_reg.SCY;

		case 0x43:
			return gb->hw_reg.SCX;

		case 0x44:
			return gb->hw_reg.LY;

		case 0x45:
			return gb->hw_reg.LYC;

		case 0x46:
			return gb->hw_reg.DMA;

		case 0x47:
			return gb->hw_reg.BGP;

		case 0x48:
			return gb->hw_reg.OBP0;

		case 0x49:
			return gb->hw_reg.OBP1;

		case 0x4A:
			return gb->hw_reg.WY;

		case 0x4B:
			return gb->hw_reg.WX;

		case 0xFF:
			return gb->hw_reg.IE;

		default:
			return 0xFF;
		}
	}

	(gb->Error)(gb, INVALID_READ, address);
	return 0xFF;
}

void write_byte(Gameboy *gb, const uf16 address, const u8 value)
{
	switch (address >> 12)
	{
	case 0x0:
	case 0x1:
		if (gb->mbc == 2 && address & 0x10)
			return;
		else if (gb->mbc > 0 && gb->cartridge_ram)
			gb->enable_cart_ram = ((value & 0x0F) == 0x0A);

		return;

	case 0x2:
		if (gb->mbc == 5)
		{
			gb->selected_rom_bank = (gb->selected_rom_bank & 0x100) | value;
			gb->selected_rom_bank =
				gb->selected_rom_bank % gb->num_rom_banks;
			return;
		}

	case 0x3:
		if (gb->mbc == 1)
		{

			gb->selected_rom_bank = (value & 0x1F) | (gb->selected_rom_bank & 0x60);

			if ((gb->selected_rom_bank & 0x1F) == 0x00)
				gb->selected_rom_bank++;
		}
		else if (gb->mbc == 2 && address & 0x10)
		{
			gb->selected_rom_bank = value & 0x0F;

			if (!gb->selected_rom_bank)
				gb->selected_rom_bank++;
		}
		else if (gb->mbc == 3)
		{
			gb->selected_rom_bank = value & 0x7F;

			if (!gb->selected_rom_bank)
				gb->selected_rom_bank++;
		}
		else if (gb->mbc == 5)
			gb->selected_rom_bank = (value & 0x01) << 8 | (gb->selected_rom_bank & 0xFF);

		gb->selected_rom_bank = gb->selected_rom_bank % gb->num_rom_banks;
		return;
		// Fall through

	case 0x4:
	case 0x5:
		if (gb->mbc == 1)
		{
			gb->cart_ram_bank = (value & 3);
			gb->selected_rom_bank = ((value & 3) << 5) | (gb->selected_rom_bank & 0x1F);
			gb->selected_rom_bank = gb->selected_rom_bank % gb->num_rom_banks;
		}
		else if (gb->mbc == 3)
			gb->cart_ram_bank = value;
		else if (gb->mbc == 5)
			gb->cart_ram_bank = (value & 0x0F);

		return;

	case 0x6:
	case 0x7:
		gb->cart_mode_select = (value & 1);
		return;

	case 0x8:
	case 0x9:
		gb->vram[address - VRAM_ADDR] = value;
		return;

	case 0xA:
	case 0xB:
		if (gb->cartridge_ram && gb->enable_cart_ram)
		{
			if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
				gb->cart_rtc[gb->cart_ram_bank - 0x08] = value;
			else if (gb->cart_mode_select &&
					 gb->cart_ram_bank < gb->num_ram_banks)
			{
				gb->write_ram(gb,
							  address - CART_RAM_ADDR + (gb->cart_ram_bank * CRAM_BANK_SIZE), value);
			}
			else if (gb->num_ram_banks)
				gb->write_ram(gb, address - CART_RAM_ADDR, value);
		}

		return;

	case 0xC:
		gb->wram[address - WRAM_0_ADDR] = value;
		return;

	case 0xD:
		gb->wram[address - WRAM_1_ADDR + WRAM_BANK_SIZE] = value;
		return;

	case 0xE:
		gb->wram[address - ECHO_ADDR] = value;
		return;

	case 0xF:
		if (address < OAM_ADDR)
		{
			gb->wram[address - ECHO_ADDR] = value;
			return;
		}

		if (address < UNUSED_ADDR)
		{
			gb->oam[address - OAM_ADDR] = value;
			return;
		}

		if (address < IO_ADDR)
			return;

		if (HRAM_ADDR <= address && address < INTR_EN_ADDR)
		{
			gb->hram[address - HRAM_ADDR] = value;
			return;
		}

		if ((address >= 0xFF10) && (address <= 0xFF3F))
		{
			audio_write(address, value);
			return;
		}

		switch (address & 0xFF)
		{

		case 0x00:

			gb->hw_reg.P1 = value;

			if ((gb->hw_reg.P1 & 0b010000) == 0)
				gb->hw_reg.P1 |= (gb->direct.joypad >> 4);

			else
				gb->hw_reg.P1 |= (gb->direct.joypad & 0x0F);

			return;

		case 0x01:
			gb->hw_reg.SB = value;
			return;

		case 0x02:
			gb->hw_reg.SC = value;
			return;

		case 0x04:
			gb->hw_reg.DIV = 0x00;
			return;

		case 0x05:
			gb->hw_reg.TIMA = value;
			return;

		case 0x06:
			gb->hw_reg.TMA = value;
			return;

		case 0x07:
			gb->hw_reg.TAC = value;
			return;

		case 0x0F:
			gb->hw_reg.IF = (value | 0b11100000);
			return;

		case 0x40:
			gb->hw_reg.LCDC = value;

			if ((gb->hw_reg.LCDC & LCDC_ENABLE) == 0)
			{

				if (gb->lcd_mode != LCD_VBLANK)
				{
					gb->hw_reg.LCDC |= LCDC_ENABLE;
					return;
				}

				gb->hw_reg.STAT = (gb->hw_reg.STAT & ~0x03) | LCD_VBLANK;
				gb->hw_reg.LY = 0;
				gb->timer.lcd_count = 0;
			}

			return;

		case 0x41:
			gb->hw_reg.STAT = (value & 0b01111000);
			return;

		case 0x42:
			gb->hw_reg.SCY = value;
			return;

		case 0x43:
			gb->hw_reg.SCX = value;
			return;

		case 0x45:
			gb->hw_reg.LYC = value;
			return;

		case 0x46:
			gb->hw_reg.DMA = (value % 0xF1);

			for (u8 i = 0; i < OAM_SIZE; i++)
				gb->oam[i] = read_byte(gb, (gb->hw_reg.DMA << 8) + i);

			return;

		case 0x47:
			gb->hw_reg.BGP = value;
			gb->display.bg_palette[0] = (gb->hw_reg.BGP & 0x03);
			gb->display.bg_palette[1] = (gb->hw_reg.BGP >> 2) & 0x03;
			gb->display.bg_palette[2] = (gb->hw_reg.BGP >> 4) & 0x03;
			gb->display.bg_palette[3] = (gb->hw_reg.BGP >> 6) & 0x03;
			return;

		case 0x48:
			gb->hw_reg.OBP0 = value;
			gb->display.sp_palette[0] = (gb->hw_reg.OBP0 & 0x03);
			gb->display.sp_palette[1] = (gb->hw_reg.OBP0 >> 2) & 0x03;
			gb->display.sp_palette[2] = (gb->hw_reg.OBP0 >> 4) & 0x03;
			gb->display.sp_palette[3] = (gb->hw_reg.OBP0 >> 6) & 0x03;
			return;

		case 0x49:
			gb->hw_reg.OBP1 = value;
			gb->display.sp_palette[4] = (gb->hw_reg.OBP1 & 0x03);
			gb->display.sp_palette[5] = (gb->hw_reg.OBP1 >> 2) & 0x03;
			gb->display.sp_palette[6] = (gb->hw_reg.OBP1 >> 4) & 0x03;
			gb->display.sp_palette[7] = (gb->hw_reg.OBP1 >> 6) & 0x03;
			return;

		case 0x4A:
			gb->hw_reg.WY = value;
			return;

		case 0x4B:
			gb->hw_reg.WX = value;
			return;

		case 0xFF:
			gb->hw_reg.IE = value;
			return;
		}
	}

	(gb->Error)(gb, INVALID_WRITE, address);
}

uf32 get_save_size(Gameboy *gb)
{
	const uf16 ram_size_location = 0x0149;
	const uf32 ram_sizes[] =
		{
			0x00, 0x800, 0x2000, 0x8000, 0x20000};
	u8 ram_size = gb->read_rom(gb, ram_size_location);
	return ram_sizes[ram_size];
}

const char *rom_title(Gameboy *gb, char title_str[static 16])
{
	uf16 title_loc = 0x134;
	const uf16 title_end = 0x143;
	const char *title_start = title_str;

	for (; title_loc <= title_end; title_loc++)
	{
		const char title_char = gb->read_rom(gb, title_loc);

		if (title_char >= ' ' && title_char <= '_')
		{
			*title_str = title_char;
			title_str++;
		}
		else
			break;
	}

	*title_str = '\0';
	return title_start;
}
