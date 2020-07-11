#pragma once

#include "defs.h"
#include "gb.h"

void tick(Gameboy *gb)
{

	if ((gb->cart_rtc[4] & 0x40) == 0)
	{
		if (++gb->raw_bits.sec == 60)
		{
			gb->raw_bits.sec = 0;

			if (++gb->raw_bits.min == 60)
			{
				gb->raw_bits.min = 0;

				if (++gb->raw_bits.hour == 24)
				{
					gb->raw_bits.hour = 0;

					if (++gb->raw_bits.yday == 0)
					{
						if (gb->raw_bits.high & 1)
						{
							gb->raw_bits.high |= 0x80;
						}

						gb->raw_bits.high ^= 1;
					}
				}
			}
		}
	}
}

void set_rtc(Gameboy *gb, const struct tm *const time)
{
	gb->cart_rtc[0] = time->tm_sec;
	gb->cart_rtc[1] = time->tm_min;
	gb->cart_rtc[2] = time->tm_hour;
	gb->cart_rtc[3] = time->tm_yday & 0xFF;
	gb->cart_rtc[4] = time->tm_yday >> 8;
}
