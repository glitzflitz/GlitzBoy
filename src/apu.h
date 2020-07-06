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
