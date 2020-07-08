#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defs.h"

#define AUDIO_SAMPLE_RATE 48000.0

#define DMG_CLOCK_FREQ 4194304.0
#define SCREEN_REFRESH_CYCLES 70224.0

#define AUDIO_SAMPLES ((u32)(AUDIO_SAMPLE_RATE / VERTICAL_SYNC))

#define AUDIO_NSAMPLES ((u32)(AUDIO_SAMPLE_RATE / VERTICAL_SYNC) * 2)

#define AUDIO_MEM_SIZE (0xFF3F - 0xFF10 + 1)
#define AUDIO_ADDR_COMPENSATION 0xFF10

#define MAX(a, b) ({ a > b ? a : b; })
#define MIN(a, b) ({ a <= b ? a : b; })

static u8 apumemory[AUDIO_MEM_SIZE];

typedef struct LengthCounter
{
	u32 load : 6;
	u32 enabled : 1;
	f32 counter;
	f32 inc;
} LengthCounter;

typedef struct VolumeEnvelope
{
	u32 step : 3;
	u32 up : 1;
	f32 counter;
	f32 inc;
} VolumeEnvelope;

typedef struct FrequencySweep
{
	uf16 freq;
	u32 rate : 3;
	u32 up : 1;
	u32 shift : 3;
	f32 counter;
	f32 inc;
} FrequencySweep;

static struct Channel
{
	u32 enabled : 1;
	u32 powered : 1;
	u32 on_left : 1;
	u32 on_right : 1;
	u32 muted : 1;

	u32 volume : 4;
	u32 volume_init : 4;

	u16 freq;
	f32 freq_counter;
	f32 freq_inc;

	int value;

	LengthCounter len;
	VolumeEnvelope venv;
	FrequencySweep sweep;

	u8 duty;
	u8 duty_cntr;

	u16 lfsr;
	bool wmode;
	int divisor_code;

	u8 vol_code;

	f32 capacitor;
} chans[4];

static f32 left, right;

static f32 hipass(struct Channel *c, f32 sample)
{
	f32 out = sample - c->capacitor;
	c->capacitor = sample - out * 0.996f;
	return out;
}

static void enable_channel(const uf8 i, const bool enable)
{
	chans[i].enabled = enable;

	u8 value = (apumemory[0xFF26 - AUDIO_ADDR_COMPENSATION] & 0x80) |
			   (chans[3].enabled << 3) | (chans[2].enabled << 2) |
			   (chans[1].enabled << 1) | (chans[0].enabled << 0);

	apumemory[0xFF26 - AUDIO_ADDR_COMPENSATION] = value;
}

static void update_env(struct Channel *c)
{
	c->venv.counter += c->venv.inc;

	while (c->venv.counter > 1.0f)
	{
		if (c->venv.step)
		{
			c->volume += c->venv.up ? 1 : -1;
			if (c->volume == 0 || c->volume == 15)
			{
				c->venv.inc = 0;
			}
			c->volume = MAX(0, MIN(15, c->volume));
		}
		c->venv.counter -= 1.0f;
	}
}

static void update_len(struct Channel *c)
{
	if (c->len.enabled)
	{
		c->len.counter += c->len.inc;
		if (c->len.counter > 1.0f)
		{
			enable_channel(c - chans, 0);
			c->len.counter = 0.0f;
		}
	}
}

static bool update_freq(struct Channel *c, f32 *pos)
{
	f32 inc = c->freq_inc - *pos;
	c->freq_counter += inc;

	if (c->freq_counter > 1.0f)
	{
		*pos = c->freq_inc - (c->freq_counter - 1.0f);
		c->freq_counter = 0.0f;
		return true;
	}
	else
	{
		*pos = c->freq_inc;
		return false;
	}
}

static void update_sweep(struct Channel *c)
{
	c->sweep.counter += c->sweep.inc;

	while (c->sweep.counter > 1.0f)
	{
		if (c->sweep.shift)
		{
			u16 inc = (c->sweep.freq >> c->sweep.shift);
			if (!c->sweep.up)
				inc *= -1;

			c->freq += inc;
			if (c->freq > 2047)
			{
				c->enabled = 0;
			}
			else
			{
				c->freq_inc = (4194304 / ((2048 - c->freq) << 5)) / AUDIO_SAMPLE_RATE;
				c->freq_inc *= 8.0f;
			}
		}
		else if (c->sweep.rate)
		{
			c->enabled = 0;
		}
		c->sweep.counter -= 1.0f;
	}
}

static void update_square(f32 *restrict samples, const bool ch2)
{
	struct Channel *c = chans + ch2;
	if (!c->powered)
		return;

	c->freq_inc = (4194304.0f / ((2048 - c->freq) << 5)) / AUDIO_SAMPLE_RATE;
	c->freq_inc *= 8.0f;

	for (uf16 i = 0; i < AUDIO_NSAMPLES; i += 2)
	{
		update_len(c);

		if (c->enabled)
		{
			update_env(c);
			if (!ch2)
				update_sweep(c);

			f32 pos = 0.0f;
			f32 prev_pos = 0.0f;
			f32 sample = 0.0f;

			while (update_freq(c, &pos))
			{
				c->duty_cntr = (c->duty_cntr + 1) & 7;
				sample += ((pos - prev_pos) / c->freq_inc) *
						  (f32)c->value;
				c->value = (c->duty & (1 << c->duty_cntr)) ? 1 : -1;
				prev_pos = pos;
			}
			sample += ((pos - prev_pos) / c->freq_inc) *
					  (f32)c->value;
			sample = hipass(c, sample * (c->volume / 15.0f));

			if (!c->muted)
			{
				samples[i + 0] +=
					sample * 0.25f * c->on_left * left;
				samples[i + 1] +=
					sample * 0.25f * c->on_right * right;
			}
		}
	}
}

static u8 wave_sample(const u32 pos, const u32 volume)
{
	u8 sample =
		apumemory[(0xFF30 + pos / 2) - AUDIO_ADDR_COMPENSATION];
	if (pos & 1)
	{
		sample &= 0xF;
	}
	else
	{
		sample >>= 4;
	}
	return volume ? (sample >> (volume - 1)) : 0;
}

static void update_wave(f32 *restrict samples)
{
	struct Channel *c = chans + 2;
	if (!c->powered)
		return;

	uf16 freq = 4194304.0f / ((2048 - c->freq) << 5);
	c->freq_inc = freq / AUDIO_SAMPLE_RATE;

	c->freq_inc *= 16.0f;

	for (uf16 i = 0; i < AUDIO_NSAMPLES; i += 2)
	{
		update_len(c);

		if (c->enabled)
		{
			f32 pos = 0.0f;
			f32 prev_pos = 0.0f;
			f32 sample = 0.0f;

			c->vol_code = wave_sample(c->value, c->volume);

			while (update_freq(c, &pos))
			{
				c->value = (c->value + 1) & 31;
				sample += ((pos - prev_pos) / c->freq_inc) *
						  (f32)c->vol_code;
				c->vol_code = wave_sample(c->value, c->volume);
				prev_pos = pos;
			}
			sample += ((pos - prev_pos) / c->freq_inc) *
					  (f32)c->vol_code;

			if (c->volume > 0)
			{
				f32 diff = (f32[]){7.5f, 3.75f,
								   1.5f}[c->volume - 1];
				sample = hipass(c, (sample - diff) / 7.5f);

				if (!c->muted)
				{
					samples[i + 0] += sample * 0.25f *
									  c->on_left * left;
					samples[i + 1] += sample * 0.25f *
									  c->on_right * right;
				}
			}
		}
	}
}

static void update_noise(f32 *restrict samples)
{
	struct Channel *c = chans + 3;
	if (!c->powered)
		return;

	uf16 freq = 4194304 / ((uf8[]){
							   8, 16, 32, 48, 64, 80, 96, 112}[c->divisor_code]
						   << c->freq);
	c->freq_inc = freq / AUDIO_SAMPLE_RATE;

	if (c->freq >= 14)
		c->enabled = 0;

	for (uf16 i = 0; i < AUDIO_NSAMPLES; i += 2)
	{
		update_len(c);

		if (c->enabled)
		{
			update_env(c);

			f32 pos = 0.0f;
			f32 prev_pos = 0.0f;
			f32 sample = 0.0f;

			while (update_freq(c, &pos))
			{
				c->lfsr = (c->lfsr << 1) |
						  (c->value == 1);

				if (c->wmode)
				{
					c->value = !(((c->lfsr >> 14) & 1) ^
								 ((c->lfsr >> 13) & 1))
								   ? 1
								   : -1;
				}
				else
				{
					c->value = !(((c->lfsr >> 6) & 1) ^
								 ((c->lfsr >> 5) & 1))
								   ? 1
								   : -1;
				}
				sample += ((pos - prev_pos) / c->freq_inc) *
						  c->value;
				prev_pos = pos;
			}
			sample += ((pos - prev_pos) / c->freq_inc) * c->value;
			sample = hipass(c, sample * (c->volume / 15.0f));

			if (!c->muted)
			{
				samples[i + 0] +=
					sample * 0.25f * c->on_left * left;
				samples[i + 1] +=
					sample * 0.25f * c->on_right * right;
			}
		}
	}
}

void audio_callback(void *misc_data, u8 *restrict stream, int len)
{
	f32 *samples = (f32 *)stream;

	(void)misc_data;

	memset(stream, 0, len);

	update_square(samples, 0);
	update_square(samples, 1);
	update_wave(samples);
	update_noise(samples);
}

static void trigger_channel(uf8 i)
{
	struct Channel *c = chans + i;

	enable_channel(i, 1);
	c->volume = c->volume_init;

	{
		u8 value =
			apumemory[(0xFF12 + (i * 5)) - AUDIO_ADDR_COMPENSATION];

		c->venv.step = value & 0x07;
		c->venv.up = value & 0x08 ? 1 : 0;
		c->venv.inc = c->venv.step ? (64.0f / (f32)c->venv.step) /
										 AUDIO_SAMPLE_RATE
								   : 8.0f / AUDIO_SAMPLE_RATE;
		c->venv.counter = 0.0f;
	}

	if (i == 0)
	{
		u8 value = apumemory[0xFF10 - AUDIO_ADDR_COMPENSATION];

		c->sweep.freq = c->freq;
		c->sweep.rate = (value >> 4) & 0x07;
		c->sweep.up = !(value & 0x08);
		c->sweep.shift = (value & 0x07);
		c->sweep.inc = c->sweep.rate ? (128.0f / (f32)(c->sweep.rate)) /
										   AUDIO_SAMPLE_RATE
									 : 0;
		c->sweep.counter = nexttowardf(1.0f, 1.1f);
	}

	int len_max = 64;

	if (i == 2)
	{
		len_max = 256;
		c->value = 0;
	}
	else if (i == 3)
	{
		c->lfsr = 0xFFFF;
		c->value = -1;
	}

	c->len.inc =
		(256.0f / (f32)(len_max - c->len.load)) / AUDIO_SAMPLE_RATE;
	c->len.counter = 0.0f;
}

u8 audio_read(const u16 address)
{
	static u8 ortab[] = {0x80, 0x3f, 0x00, 0xff, 0xbf, 0xff,
						 0x3f, 0x00, 0xff, 0xbf, 0x7f, 0xff,
						 0x9f, 0xff, 0xbf, 0xff, 0xff, 0x00,
						 0x00, 0xbf, 0x00, 0x00, 0x70};

	if (address > 0xFF26)
		return apumemory[address - AUDIO_ADDR_COMPENSATION];

	return apumemory[address - AUDIO_ADDR_COMPENSATION] | ortab[address - 0xFF10];
}

void audio_write(const u16 address, const u8 value)
{
	uf8 i = (address - 0xFF10) / 5;
	apumemory[address - AUDIO_ADDR_COMPENSATION] = value;

	switch (address)
	{
	case 0xFF12:
	case 0xFF17:
	case 0xFF21:
	{
		chans[i].volume_init = value >> 4;
		chans[i].powered = (value >> 3) != 0;

		if (chans[i].powered && chans[i].enabled)
		{
			if ((chans[i].venv.step == 0 && chans[i].venv.inc != 0))
			{
				if (value & 0x08)
				{
					chans[i].volume++;
				}
				else
				{
					chans[i].volume += 2;
				}
			}
			else
			{
				chans[i].volume = 16 - chans[i].volume;
			}

			chans[i].volume &= 0x0F;
			chans[i].venv.step = value & 0x07;
		}
	}
	break;

	case 0xFF1C:
		chans[i].volume = chans[i].volume_init = (value >> 5) & 0x03;
		break;

	case 0xFF11:
	case 0xFF16:
	case 0xFF20:
	{
		const u8 duty_lookup[] = {0x10, 0x30, 0x3C, 0xCF};
		chans[i].len.load = value & 0x3f;
		chans[i].duty = duty_lookup[value >> 6];
		break;
	}

	case 0xFF1B:
		chans[i].len.load = value;
		break;

	case 0xFF13:
	case 0xFF18:
	case 0xFF1D:
		chans[i].freq &= 0xFF00;
		chans[i].freq |= value;
		break;

	case 0xFF1A:
		chans[i].powered = (value & 0x80) != 0;
		enable_channel(i, value & 0x80);
		break;

	case 0xFF14:
	case 0xFF19:
	case 0xFF1E:
		chans[i].freq &= 0x00FF;
		chans[i].freq |= ((value & 0x07) << 8);
		// Fall through

	case 0xFF23:
		chans[i].len.enabled = value & 0x40 ? 1 : 0;
		if (value & 0x80)
			trigger_channel(i);

		break;

	case 0xFF22:
		chans[3].freq = value >> 4;
		chans[3].wmode = !(value & 0x08);
		chans[3].divisor_code = value & 0x07;
		break;

	case 0xFF24:
		left = ((value >> 4) & 0x07) / 7.0f;
		right = (value & 0x07) / 7.0f;
		break;

	case 0xFF25:
		for (uf8 i = 0; i < 4; ++i)
		{
			chans[i].on_left = (value >> (4 + i)) & 1;
			chans[i].on_right = (value >> i) & 1;
		}
		break;
	}
}

void audio_init(void)
{
	memset(chans, 0, sizeof(chans));
	chans[0].value = chans[1].value = -1;

	{
		const u8 regs_init[] = {0x80, 0xBF, 0xF3, 0xFF, 0x3F,
								0xFF, 0x3F, 0x00, 0xFF, 0x3F,
								0x7F, 0xFF, 0x9F, 0xFF, 0x3F,
								0xFF, 0xFF, 0x00, 0x00, 0x3F,
								0x77, 0xF3, 0xF1};

		for (uf8 i = 0; i < sizeof(regs_init); ++i)
			audio_write(0xFF10 + i, regs_init[i]);
	}

	{
		const u8 wave_init[] = {0xac, 0xdd, 0xda, 0x48,
								0x36, 0x02, 0xcf, 0x16,
								0x2c, 0x04, 0xe5, 0x2c,
								0xac, 0xdd, 0xda, 0x48};

		for (uf8 i = 0; i < sizeof(wave_init); ++i)
			audio_write(0xFF30 + i, wave_init[i]);
	}
}
