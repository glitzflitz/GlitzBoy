#pragma once

#include "gb.h"

void draw_line(Gameboy *gb)
{
	u8 framebuffer[160] = {0};

	if (gb->display.gpu_draw_line == NULL)
		return;

	if (gb->direct.skipframe && !gb->display.frame_skip_count)
		return;

	if (gb->direct.interlace)
	{
		if ((gb->display.interlace_count == 0 && (gb->hw_reg.LY & 1) == 0) || (gb->display.interlace_count == 1 && (gb->hw_reg.LY & 1) == 1))
		{
			if (gb->hw_reg.LCDC & LCDC_WINDOW_ENABLE && gb->hw_reg.LY >= gb->display.WY && gb->hw_reg.WX <= 166)
				gb->display.window_clear++;

			return;
		}
	}

	if (gb->hw_reg.LCDC & LCDC_BG_ENABLE)
	{
		const u8 bg_y = gb->hw_reg.LY + gb->hw_reg.SCY;

		const u16 bg_map =
			((gb->hw_reg.LCDC & LCDC_BG_MAP) ? VRAM_BMAP_2 : VRAM_BMAP_1) + (bg_y >> 3) * 0x20;

		u8 disp_x = LCD_WIDTH - 1;

		u8 bg_x = disp_x + gb->hw_reg.SCX;

		u8 idx = gb->vram[bg_map + (bg_x >> 3)];

		const u8 py = (bg_y & 0x07);

		u8 px = 7 - (bg_x & 0x07);

		u16 tile;

		if (gb->hw_reg.LCDC & LCDC_TILE_SELECT)
			tile = VRAM_TILES_1 + idx * 0x10;
		else
			tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

		tile += 2 * py;

		u8 t1 = gb->vram[tile] >> px;
		u8 t2 = gb->vram[tile + 1] >> px;

		for (; disp_x != 0xFF; disp_x--)
		{
			if (px == 8)
			{

				px = 0;
				bg_x = disp_x + gb->hw_reg.SCX;
				idx = gb->vram[bg_map + (bg_x >> 3)];

				if (gb->hw_reg.LCDC & LCDC_TILE_SELECT)
					tile = VRAM_TILES_1 + idx * 0x10;
				else
					tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

				tile += 2 * py;
				t1 = gb->vram[tile];
				t2 = gb->vram[tile + 1];
			}

			u8 C = (t1 & 0x1) | ((t2 & 0x1) << 1);
			framebuffer[disp_x] = gb->display.bg_palette[C];
			framebuffer[disp_x] |= LCD_PALETTE_BG;
			t1 = t1 >> 1;
			t2 = t2 >> 1;
			px++;
		}
	}

	if (gb->hw_reg.LCDC & LCDC_WINDOW_ENABLE && gb->hw_reg.LY >= gb->display.WY && gb->hw_reg.WX <= 166)
	{

		u16 win_line = (gb->hw_reg.LCDC & LCDC_WINDOW_MAP) ? VRAM_BMAP_2 : VRAM_BMAP_1;
		win_line += (gb->display.window_clear >> 3) * 0x20;

		u8 disp_x = LCD_WIDTH - 1;
		u8 win_x = disp_x - gb->hw_reg.WX + 7;

		u8 py = gb->display.window_clear & 0x07;
		u8 px = 7 - (win_x & 0x07);
		u8 idx = gb->vram[win_line + (win_x >> 3)];

		u16 tile;

		if (gb->hw_reg.LCDC & LCDC_TILE_SELECT)
			tile = VRAM_TILES_1 + idx * 0x10;
		else
			tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

		tile += 2 * py;

		u8 t1 = gb->vram[tile] >> px;
		u8 t2 = gb->vram[tile + 1] >> px;

		u8 end = (gb->hw_reg.WX < 7 ? 0 : gb->hw_reg.WX - 7) - 1;

		for (; disp_x != end; disp_x--)
		{
			if (px == 8)
			{

				px = 0;
				win_x = disp_x - gb->hw_reg.WX + 7;
				idx = gb->vram[win_line + (win_x >> 3)];

				if (gb->hw_reg.LCDC & LCDC_TILE_SELECT)
					tile = VRAM_TILES_1 + idx * 0x10;
				else
					tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

				tile += 2 * py;
				t1 = gb->vram[tile];
				t2 = gb->vram[tile + 1];
			}

			u8 C = (t1 & 0x1) | ((t2 & 0x1) << 1);
			framebuffer[disp_x] = gb->display.bg_palette[C];
			framebuffer[disp_x] |= LCD_PALETTE_BG;
			t1 = t1 >> 1;
			t2 = t2 >> 1;
			px++;
		}

		gb->display.window_clear++;
	}

	if (gb->hw_reg.LCDC & LCDC_OBJ_ENABLE)
	{
		u8 count = 0;

		for (u8 s = NUM_SPRITES - 1;
			 s != 0xFF;
			 s--)
		{

			u8 OY = gb->oam[4 * s + 0];

			u8 OX = gb->oam[4 * s + 1];

			u8 OT = gb->oam[4 * s + 2] & (gb->hw_reg.LCDC & LCDC_OBJ_SIZE ? 0xFE : 0xFF);

			u8 OF = gb->oam[4 * s + 3];

			if (gb->hw_reg.LY +
						(gb->hw_reg.LCDC & LCDC_OBJ_SIZE ? 0 : 8) >=
					OY ||
				gb->hw_reg.LY + 16 < OY)
				continue;

			count++;

			if (OX == 0 || OX >= 168)
				continue;

			u8 py = gb->hw_reg.LY - OY + 16;

			if (OF & OBJ_FLIP_Y)
				py = (gb->hw_reg.LCDC & LCDC_OBJ_SIZE ? 15 : 7) - py;

			u8 t1 = gb->vram[VRAM_TILES_1 + OT * 0x10 + 2 * py];
			u8 t2 = gb->vram[VRAM_TILES_1 + OT * 0x10 + 2 * py + 1];

			u8 dir, start, end, shift;

			if (OF & OBJ_FLIP_X)
			{
				dir = 1;
				start = (OX < 8 ? 0 : OX - 8);
				end = MIN(OX, LCD_WIDTH);
				shift = 8 - OX + start;
			}
			else
			{
				dir = -1;
				start = MIN(OX, LCD_WIDTH) - 1;
				end = (OX < 8 ? 0 : OX - 8) - 1;
				shift = OX - (start + 1);
			}

			t1 >>= shift;
			t2 >>= shift;

			for (u8 disp_x = start; disp_x != end; disp_x += dir)
			{
				u8 C = (t1 & 0x1) | ((t2 & 0x1) << 1);
#if 0

				if(c
						
						&& !((OF & OBJ_PRIORITY)
						     && ((pixels[disp_x] & 0x3)
							 && fx[disp_x] == 0xFE)))
#else
				if (C && !(OF & OBJ_PRIORITY && framebuffer[disp_x] & 0x3))
#endif
				{

					framebuffer[disp_x] = (OF & OBJ_PALETTE)
											  ? gb->display.sp_palette[C + 4]
											  : gb->display.sp_palette[C];

					framebuffer[disp_x] |= (OF & OBJ_PALETTE);

					framebuffer[disp_x] &= ~LCD_PALETTE_BG;
				}

				t1 = t1 >> 1;
				t2 = t2 >> 1;
			}
		}
	}

	gb->display.gpu_draw_line(gb, framebuffer, gb->hw_reg.LY);
}

u8 color_code(Gameboy *gb)
{
#define ROM_TITLE_START_ADDR 0x0134
#define ROM_TITLE_END_ADDR 0x0143

	u8 x = 0;

	for (u16 i = ROM_TITLE_START_ADDR; i <= ROM_TITLE_END_ADDR; i++)
		x += gb->read_rom(gb, i);

	return x;
}

void init_gpu(Gameboy *gb,
			  void (*gpu_draw_line)(Gameboy *,
									const u8 framebuffer[static 160],
									const uf8 line))
{
	gb->display.gpu_draw_line = gpu_draw_line;

	gb->direct.interlace = 0;
	gb->display.interlace_count = 0;
	gb->direct.skipframe = 0;
	gb->display.frame_skip_count = 0;

	gb->display.window_clear = 0;
	gb->display.WY = 0;

	return;
}