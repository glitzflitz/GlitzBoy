#pragma once

#include "defs.h"
#include "gb.h"
#include "gpu.h"

u8 execute_instr(Gameboy *gb)
{
	u8 inst_cycles;
	u8 instr = read_byte(gb, gb->cpu_reg.PC++);
	u8 r = (instr & 0x7);
	u8 B = (instr >> 3) & 0x7;
	u8 D = (instr >> 3) & 0x1;
	u8 reg;
	u8 write = 1;

	inst_cycles = 8;

	switch (instr & 0xC7)
	{
	case 0x06:
	case 0x86:
	case 0xC6:
		inst_cycles += 8;
		break;
	case 0x46:
		inst_cycles += 4;
		break;
	}

	switch (r)
	{
	case 0:
		reg = gb->cpu_reg.B;
		break;

	case 1:
		reg = gb->cpu_reg.C;
		break;

	case 2:
		reg = gb->cpu_reg.D;
		break;

	case 3:
		reg = gb->cpu_reg.E;
		break;

	case 4:
		reg = gb->cpu_reg.H;
		break;

	case 5:
		reg = gb->cpu_reg.L;
		break;

	case 6:
		reg = read_byte(gb, gb->cpu_reg.HL);
		break;

	default:
		reg = gb->cpu_reg.a;
		break;
	}

	switch (instr >> 6)
	{
	case 0x0:
		instr = (instr >> 4) & 0x3;

		switch (instr)
		{
		case 0x0:
		case 0x1:
			if (D)
			{
				u8 temp = reg;
				reg = (reg >> 1);
				reg |= instr ? (gb->cpu_reg.raw_bits.C << 7) : (temp << 7);
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
				gb->cpu_reg.raw_bits.C = (temp & 0x01);
			}
			else
			{
				u8 temp = reg;
				reg = (reg << 1);
				reg |= instr ? gb->cpu_reg.raw_bits.C : (temp >> 7);
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
				gb->cpu_reg.raw_bits.C = (temp >> 7);
			}

			break;

		case 0x2:
			if (D)
			{
				gb->cpu_reg.raw_bits.C = reg & 0x01;
				reg = (reg >> 1) | (reg & 0x80);
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
			}
			else
			{
				gb->cpu_reg.raw_bits.C = (reg >> 7);
				reg = reg << 1;
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
			}

			break;

		case 0x3:
			if (D)
			{
				gb->cpu_reg.raw_bits.C = reg & 0x01;
				reg = reg >> 1;
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
			}
			else
			{
				u8 temp = (reg >> 4) & 0x0F;
				temp |= (reg << 4) & 0xF0;
				reg = temp;
				gb->cpu_reg.raw_bits.Z = (reg == 0x00);
				gb->cpu_reg.raw_bits.N = 0;
				gb->cpu_reg.raw_bits.H = 0;
				gb->cpu_reg.raw_bits.C = 0;
			}

			break;
		}

		break;

	case 0x1:
		gb->cpu_reg.raw_bits.Z = !((reg >> B) & 0x1);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 1;
		write = 0;
		break;

	case 0x2:
		reg &= (0xFE << B) | (0xFF >> (8 - B));
		break;

	case 0x3:
		reg |= (0x1 << B);
		break;
	}

	if (write)
	{
		switch (r)
		{
		case 0:
			gb->cpu_reg.B = reg;
			break;

		case 1:
			gb->cpu_reg.C = reg;
			break;

		case 2:
			gb->cpu_reg.D = reg;
			break;

		case 3:
			gb->cpu_reg.E = reg;
			break;

		case 4:
			gb->cpu_reg.H = reg;
			break;

		case 5:
			gb->cpu_reg.L = reg;
			break;

		case 6:
			write_byte(gb, gb->cpu_reg.HL, reg);
			break;

		case 7:
			gb->cpu_reg.a = reg;
			break;
		}
	}
	return inst_cycles;
}

void cpu_step(Gameboy *gb)
{
	u8 opcode, inst_cycles;
	static const u8 op_cycles[0x100] =
		{

			4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4,
			4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4,
			8, 12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,
			8, 12, 8, 8, 12, 12, 12, 4, 8, 8, 8, 8, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
			8, 12, 12, 16, 12, 16, 8, 16, 8, 16, 12, 8, 12, 24, 8, 16,
			8, 12, 12, 0, 12, 16, 8, 16, 8, 16, 12, 0, 12, 0, 8, 16,
			12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16,
			12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16

		};

	if ((gb->ime || gb->halt) &&
		(gb->hw_reg.IF & gb->hw_reg.IE & ANY_INTR))
	{
		gb->halt = 0;

		if (gb->ime)
		{

			gb->ime = 0;

			write_byte(gb, --gb->cpu_reg.SP, gb->cpu_reg.PC >> 8);
			write_byte(gb, --gb->cpu_reg.SP, gb->cpu_reg.PC & 0xFF);

			if (gb->hw_reg.IF & gb->hw_reg.IE & VBLANK_INTR)
			{
				gb->cpu_reg.PC = VBLANK_INTR_ADDR;
				gb->hw_reg.IF ^= VBLANK_INTR;
			}
			else if (gb->hw_reg.IF & gb->hw_reg.IE & LCDC_INTR)
			{
				gb->cpu_reg.PC = LCDC_INTR_ADDR;
				gb->hw_reg.IF ^= LCDC_INTR;
			}
			else if (gb->hw_reg.IF & gb->hw_reg.IE & TIMER_INTR)
			{
				gb->cpu_reg.PC = TIMER_INTR_ADDR;
				gb->hw_reg.IF ^= TIMER_INTR;
			}
			else if (gb->hw_reg.IF & gb->hw_reg.IE & SERIAL_INTR)
			{
				gb->cpu_reg.PC = SERIAL_INTR_ADDR;
				gb->hw_reg.IF ^= SERIAL_INTR;
			}
			else if (gb->hw_reg.IF & gb->hw_reg.IE & CONTROL_INTR)
			{
				gb->cpu_reg.PC = CONTROL_INTR_ADDR;
				gb->hw_reg.IF ^= CONTROL_INTR;
			}
		}
	}

	opcode = (gb->halt ? 0x00 : read_byte(gb, gb->cpu_reg.PC++));
	inst_cycles = op_cycles[opcode];

	switch (opcode)
	{
	case 0x00:
		break;

	case 0x01:
		gb->cpu_reg.C = read_byte(gb, gb->cpu_reg.PC++);
		gb->cpu_reg.B = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x02:
		write_byte(gb, gb->cpu_reg.BC, gb->cpu_reg.a);
		break;

	case 0x03:
		gb->cpu_reg.BC++;
		break;

	case 0x04:
		gb->cpu_reg.B++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.B == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.B & 0x0F) == 0x00);
		break;

	case 0x05:
		gb->cpu_reg.B--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.B == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.B & 0x0F) == 0x0F);
		break;

	case 0x06:
		gb->cpu_reg.B = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x07:
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | (gb->cpu_reg.a >> 7);
		gb->cpu_reg.raw_bits.Z = 0;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = (gb->cpu_reg.a & 0x01);
		break;

	case 0x08:
	{
		u16 temp = read_byte(gb, gb->cpu_reg.PC++);
		temp |= read_byte(gb, gb->cpu_reg.PC++) << 8;
		write_byte(gb, temp++, gb->cpu_reg.SP & 0xFF);
		write_byte(gb, temp, gb->cpu_reg.SP >> 8);
		break;
	}

	case 0x09:
	{
		uf32 temp = gb->cpu_reg.HL + gb->cpu_reg.BC;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(temp ^ gb->cpu_reg.HL ^ gb->cpu_reg.BC) & 0x1000 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.HL = (temp & 0x0000FFFF);
		break;
	}

	case 0x0A:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.BC);
		break;

	case 0x0B:
		gb->cpu_reg.BC--;
		break;

	case 0x0C:
		gb->cpu_reg.C++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.C == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.C & 0x0F) == 0x00);
		break;

	case 0x0D:
		gb->cpu_reg.C--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.C == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.C & 0x0F) == 0x0F);
		break;

	case 0x0E:
		gb->cpu_reg.C = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x0F:
		gb->cpu_reg.raw_bits.C = gb->cpu_reg.a & 0x01;
		gb->cpu_reg.a = (gb->cpu_reg.a >> 1) | (gb->cpu_reg.a << 7);
		gb->cpu_reg.raw_bits.Z = 0;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		break;

	case 0x10:

		break;

	case 0x11:
		gb->cpu_reg.E = read_byte(gb, gb->cpu_reg.PC++);
		gb->cpu_reg.D = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x12:
		write_byte(gb, gb->cpu_reg.DE, gb->cpu_reg.a);
		break;

	case 0x13:
		gb->cpu_reg.DE++;
		break;

	case 0x14:
		gb->cpu_reg.D++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.D == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.D & 0x0F) == 0x00);
		break;

	case 0x15:
		gb->cpu_reg.D--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.D == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.D & 0x0F) == 0x0F);
		break;

	case 0x16:
		gb->cpu_reg.D = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x17:
	{
		u8 temp = gb->cpu_reg.a;
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = 0;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = (temp >> 7) & 0x01;
		break;
	}

	case 0x18:
	{
		int8_t temp = (int8_t)read_byte(gb, gb->cpu_reg.PC++);
		gb->cpu_reg.PC += temp;
		break;
	}

	case 0x19:
	{
		uf32 temp = gb->cpu_reg.HL + gb->cpu_reg.DE;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(temp ^ gb->cpu_reg.HL ^ gb->cpu_reg.DE) & 0x1000 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.HL = (temp & 0x0000FFFF);
		break;
	}

	case 0x1A:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.DE);
		break;

	case 0x1B:
		gb->cpu_reg.DE--;
		break;

	case 0x1C:
		gb->cpu_reg.E++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.E == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.E & 0x0F) == 0x00);
		break;

	case 0x1D:
		gb->cpu_reg.E--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.E == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.E & 0x0F) == 0x0F);
		break;

	case 0x1E:
		gb->cpu_reg.E = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x1F:
	{
		u8 temp = gb->cpu_reg.a;
		gb->cpu_reg.a = gb->cpu_reg.a >> 1 | (gb->cpu_reg.raw_bits.C << 7);
		gb->cpu_reg.raw_bits.Z = 0;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = temp & 0x1;
		break;
	}

	case 0x20:
		if (!gb->cpu_reg.raw_bits.Z)
		{
			int8_t temp = (int8_t)read_byte(gb, gb->cpu_reg.PC++);
			gb->cpu_reg.PC += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.PC++;

		break;

	case 0x21:
		gb->cpu_reg.L = read_byte(gb, gb->cpu_reg.PC++);
		gb->cpu_reg.H = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x22:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.a);
		gb->cpu_reg.HL++;
		break;

	case 0x23:
		gb->cpu_reg.HL++;
		break;

	case 0x24:
		gb->cpu_reg.H++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.H == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.H & 0x0F) == 0x00);
		break;

	case 0x25:
		gb->cpu_reg.H--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.H == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.H & 0x0F) == 0x0F);
		break;

	case 0x26:
		gb->cpu_reg.H = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x27:
	{
		u16 a = gb->cpu_reg.a;

		if (gb->cpu_reg.raw_bits.N)
		{
			if (gb->cpu_reg.raw_bits.H)
				a = (a - 0x06) & 0xFF;

			if (gb->cpu_reg.raw_bits.C)
				a -= 0x60;
		}
		else
		{
			if (gb->cpu_reg.raw_bits.H || (a & 0x0F) > 9)
				a += 0x06;

			if (gb->cpu_reg.raw_bits.C || a > 0x9F)
				a += 0x60;
		}

		if ((a & 0x100) == 0x100)
			gb->cpu_reg.raw_bits.C = 1;

		gb->cpu_reg.a = a;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.a == 0);
		gb->cpu_reg.raw_bits.H = 0;

		break;
	}

	case 0x28:
		if (gb->cpu_reg.raw_bits.Z)
		{
			int8_t temp = (int8_t)read_byte(gb, gb->cpu_reg.PC++);
			gb->cpu_reg.PC += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.PC++;

		break;

	case 0x29:
	{
		uf32 temp = gb->cpu_reg.HL + gb->cpu_reg.HL;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = (temp & 0x1000) ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.HL = (temp & 0x0000FFFF);
		break;
	}

	case 0x2A:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.HL++);
		break;

	case 0x2B:
		gb->cpu_reg.HL--;
		break;

	case 0x2C:
		gb->cpu_reg.L++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.L == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.L & 0x0F) == 0x00);
		break;

	case 0x2D:
		gb->cpu_reg.L--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.L == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.L & 0x0F) == 0x0F);
		break;

	case 0x2E:
		gb->cpu_reg.L = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x2F:
		gb->cpu_reg.a = ~gb->cpu_reg.a;
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = 1;
		break;

	case 0x30:
		if (!gb->cpu_reg.raw_bits.C)
		{
			int8_t temp = (int8_t)read_byte(gb, gb->cpu_reg.PC++);
			gb->cpu_reg.PC += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.PC++;

		break;

	case 0x31:
		gb->cpu_reg.SP = read_byte(gb, gb->cpu_reg.PC++);
		gb->cpu_reg.SP |= read_byte(gb, gb->cpu_reg.PC++) << 8;
		break;

	case 0x32:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.a);
		gb->cpu_reg.HL--;
		break;

	case 0x33:
		gb->cpu_reg.SP++;
		break;

	case 0x34:
	{
		u8 temp = read_byte(gb, gb->cpu_reg.HL) + 1;
		gb->cpu_reg.raw_bits.Z = (temp == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((temp & 0x0F) == 0x00);
		write_byte(gb, gb->cpu_reg.HL, temp);
		break;
	}

	case 0x35:
	{
		u8 temp = read_byte(gb, gb->cpu_reg.HL) - 1;
		gb->cpu_reg.raw_bits.Z = (temp == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((temp & 0x0F) == 0x0F);
		write_byte(gb, gb->cpu_reg.HL, temp);
		break;
	}

	case 0x36:
		write_byte(gb, gb->cpu_reg.HL, read_byte(gb, gb->cpu_reg.PC++));
		break;

	case 0x37:
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = 1;
		break;

	case 0x38:
		if (gb->cpu_reg.raw_bits.C)
		{
			int8_t temp = (int8_t)read_byte(gb, gb->cpu_reg.PC++);
			gb->cpu_reg.PC += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.PC++;

		break;

	case 0x39:
	{
		uf32 temp = gb->cpu_reg.HL + gb->cpu_reg.SP;
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			((gb->cpu_reg.HL & 0xFFF) + (gb->cpu_reg.SP & 0xFFF)) & 0x1000 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = temp & 0x10000 ? 1 : 0;
		gb->cpu_reg.HL = (u16)temp;
		break;
	}

	case 0x3A:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.HL--);
		break;

	case 0x3B:
		gb->cpu_reg.SP--;
		break;

	case 0x3C:
		gb->cpu_reg.a++;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.a == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.a & 0x0F) == 0x00);
		break;

	case 0x3D:
		gb->cpu_reg.a--;
		gb->cpu_reg.raw_bits.Z = (gb->cpu_reg.a == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = ((gb->cpu_reg.a & 0x0F) == 0x0F);
		break;

	case 0x3E:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.PC++);
		break;

	case 0x3F:
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = ~gb->cpu_reg.raw_bits.C;
		break;

	case 0x40:
		break;

	case 0x41:
		gb->cpu_reg.B = gb->cpu_reg.C;
		break;

	case 0x42:
		gb->cpu_reg.B = gb->cpu_reg.D;
		break;

	case 0x43:
		gb->cpu_reg.B = gb->cpu_reg.E;
		break;

	case 0x44:
		gb->cpu_reg.B = gb->cpu_reg.H;
		break;

	case 0x45:
		gb->cpu_reg.B = gb->cpu_reg.L;
		break;

	case 0x46:
		gb->cpu_reg.B = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x47:
		gb->cpu_reg.B = gb->cpu_reg.a;
		break;

	case 0x48:
		gb->cpu_reg.C = gb->cpu_reg.B;
		break;

	case 0x49:
		break;

	case 0x4A:
		gb->cpu_reg.C = gb->cpu_reg.D;
		break;

	case 0x4B:
		gb->cpu_reg.C = gb->cpu_reg.E;
		break;

	case 0x4C:
		gb->cpu_reg.C = gb->cpu_reg.H;
		break;

	case 0x4D:
		gb->cpu_reg.C = gb->cpu_reg.L;
		break;

	case 0x4E:
		gb->cpu_reg.C = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x4F:
		gb->cpu_reg.C = gb->cpu_reg.a;
		break;

	case 0x50:
		gb->cpu_reg.D = gb->cpu_reg.B;
		break;

	case 0x51:
		gb->cpu_reg.D = gb->cpu_reg.C;
		break;

	case 0x52:
		break;

	case 0x53:
		gb->cpu_reg.D = gb->cpu_reg.E;
		break;

	case 0x54:
		gb->cpu_reg.D = gb->cpu_reg.H;
		break;

	case 0x55:
		gb->cpu_reg.D = gb->cpu_reg.L;
		break;

	case 0x56:
		gb->cpu_reg.D = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x57:
		gb->cpu_reg.D = gb->cpu_reg.a;
		break;

	case 0x58:
		gb->cpu_reg.E = gb->cpu_reg.B;
		break;

	case 0x59:
		gb->cpu_reg.E = gb->cpu_reg.C;
		break;

	case 0x5A:
		gb->cpu_reg.E = gb->cpu_reg.D;
		break;

	case 0x5B:
		break;

	case 0x5C:
		gb->cpu_reg.E = gb->cpu_reg.H;
		break;

	case 0x5D:
		gb->cpu_reg.E = gb->cpu_reg.L;
		break;

	case 0x5E:
		gb->cpu_reg.E = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x5F:
		gb->cpu_reg.E = gb->cpu_reg.a;
		break;

	case 0x60:
		gb->cpu_reg.H = gb->cpu_reg.B;
		break;

	case 0x61:
		gb->cpu_reg.H = gb->cpu_reg.C;
		break;

	case 0x62:
		gb->cpu_reg.H = gb->cpu_reg.D;
		break;

	case 0x63:
		gb->cpu_reg.H = gb->cpu_reg.E;
		break;

	case 0x64:
		break;

	case 0x65:
		gb->cpu_reg.H = gb->cpu_reg.L;
		break;

	case 0x66:
		gb->cpu_reg.H = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x67:
		gb->cpu_reg.H = gb->cpu_reg.a;
		break;

	case 0x68:
		gb->cpu_reg.L = gb->cpu_reg.B;
		break;

	case 0x69:
		gb->cpu_reg.L = gb->cpu_reg.C;
		break;

	case 0x6A:
		gb->cpu_reg.L = gb->cpu_reg.D;
		break;

	case 0x6B:
		gb->cpu_reg.L = gb->cpu_reg.E;
		break;

	case 0x6C:
		gb->cpu_reg.L = gb->cpu_reg.H;
		break;

	case 0x6D:
		break;

	case 0x6E:
		gb->cpu_reg.L = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x6F:
		gb->cpu_reg.L = gb->cpu_reg.a;
		break;

	case 0x70:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.B);
		break;

	case 0x71:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.C);
		break;

	case 0x72:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.D);
		break;

	case 0x73:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.E);
		break;

	case 0x74:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.H);
		break;

	case 0x75:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.L);
		break;

	case 0x76:

		gb->halt = 1;
		break;

	case 0x77:
		write_byte(gb, gb->cpu_reg.HL, gb->cpu_reg.a);
		break;

	case 0x78:
		gb->cpu_reg.a = gb->cpu_reg.B;
		break;

	case 0x79:
		gb->cpu_reg.a = gb->cpu_reg.C;
		break;

	case 0x7A:
		gb->cpu_reg.a = gb->cpu_reg.D;
		break;

	case 0x7B:
		gb->cpu_reg.a = gb->cpu_reg.E;
		break;

	case 0x7C:
		gb->cpu_reg.a = gb->cpu_reg.H;
		break;

	case 0x7D:
		gb->cpu_reg.a = gb->cpu_reg.L;
		break;

	case 0x7E:
		gb->cpu_reg.a = read_byte(gb, gb->cpu_reg.HL);
		break;

	case 0x7F:
		break;

	case 0x80:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.B;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.B ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x81:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.C ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x82:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.D;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.D ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x83:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.E;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.E ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x84:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.H;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.H ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x85:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.L;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.L ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x86:
	{
		u8 HL = read_byte(gb, gb->cpu_reg.HL);
		u16 temp = gb->cpu_reg.a + HL;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ HL ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x87:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.a;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H = temp & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x88:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.B + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.B ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x89:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.C + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.C ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8A:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.D + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.D ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8B:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.E + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.E ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8C:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.H + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.H ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8D:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.L + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.L ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8E:
	{
		u8 reg = read_byte(gb, gb->cpu_reg.HL);
		u16 temp = gb->cpu_reg.a + reg + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ reg ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x8F:
	{
		u16 temp = gb->cpu_reg.a + gb->cpu_reg.a + gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 0;

		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.a ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x90:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.B;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.B ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x91:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.C ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x92:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.D;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.D ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x93:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.E;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.E ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x94:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.H;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.H ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x95:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.L;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.L ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x96:
	{
		u8 reg = read_byte(gb, gb->cpu_reg.HL);
		u16 temp = gb->cpu_reg.a - reg;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ reg ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x97:
		gb->cpu_reg.a = 0;
		gb->cpu_reg.raw_bits.Z = 1;
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H = 0;
		gb->cpu_reg.raw_bits.C = 0;
		break;

	case 0x98:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.B - gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.B ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x99:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.C - gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.C ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0x9A:
	{
		u16 temp = gb->cpu_reg.a - gb->cpu_reg.D - gb->cpu_reg.raw_bits.C;
		gb->cpu_reg.raw_bits.Z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.raw_bits.N = 1;
		gb->cpu_reg.raw_bits.H =
			(gb->cpu_reg.a ^ gb->cpu_reg.D ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.raw_bits.C = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	default:
		(gb->Error)(gb, INVALID_OPCODE, opcode);
	}

}