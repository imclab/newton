#include <gba_base.h>

#include <gba_dma.h>
#include <gba_sound.h>
#include <gba_timers.h>
#include <gba_systemcalls.h>

#include <stdio.h>

#define SAMPLERATE (18157.16)

/* setup some constants */
#define CYCLES_PR_FRAME 280896
#define SAMPLES_PR_FRAME  ((int)((1 << 24) / ((float)SAMPLERATE)))
#define SOUND_BUFFER_SIZE ((int)((float)CYCLES_PR_FRAME / SAMPLES_PR_FRAME))

static s8 sound_buffers[2][SOUND_BUFFER_SIZE] EWRAM_DATA;
static u8 sound_buffer_index = 0;

static const void *adpcm_data = NULL;
static int adpcm_frame = 0;

void adpcm_gba_init(const void *data)
{
	adpcm_data = data;
	adpcm_gba_set_frame(0);
	
	static const u32 zero = 0;
	CpuFastSet(&zero, &sound_buffers[0][0], DMA_SRC_FIXED | ((SOUND_BUFFER_SIZE / 4) * 2));
	REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
	REG_SOUNDCNT_X = (1 << 7);
	
	/* setup timer */
	REG_TM0CNT_L = (1 << 16) - SAMPLES_PR_FRAME;
	REG_TM0CNT_H = TIMER_START;
}

void adpcm_gba_set_frame(int frame)
{
	adpcm_frame = frame;
	adpcm_init((u8*)adpcm_data + frame * (SOUND_BUFFER_SIZE / 2));
}

int adpcm_gba_get_frame()
{
	return adpcm_frame;
}

void adpcm_gba_close()
{
	REG_TM0CNT_H = 0;
	REG_SOUNDCNT_X = 0;
}

void adpcm_gba_vblank()
{
	if (sound_buffer_index == 0)
	{
		REG_DMA1CNT = 0;
		REG_DMA1SAD = (u32) &(sound_buffers[0][0]);
		REG_DMA1DAD = (u32) &REG_FIFO_A;
		REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA32 | DMA_SPECIAL | DMA_ENABLE;
	}
	sound_buffer_index ^= 1;
	adpcm_frame++;
}

void adpcm_gba_frame()
{
	adpcm_render(sound_buffers[sound_buffer_index], SOUND_BUFFER_SIZE);
}