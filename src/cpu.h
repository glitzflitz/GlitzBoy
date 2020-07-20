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

	default:
		(gb->Error)(gb, INVALID_OPCODE, opcode);
	}

}