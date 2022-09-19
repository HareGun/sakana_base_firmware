
#include <stdint.h>

typedef struct {
	uint8_t riff_mark[4];
	uint32_t file_size;
	uint8_t	wave_str[4];
	uint8_t	fmt_str[4];
	uint32_t pcm_bit_num;
	uint16_t pcm_encode;
	uint16_t sound_channel;
	uint32_t pcm_sample_freq;
	uint32_t byte_freq;
	uint16_t block_alin;
	uint16_t sample_bits;
	uint8_t	data_str[4]; 
	uint32_t sound_size;
} WAV_Typedef;

void WavePlayer_init();
void WavePlayer_reqPlay(WAV_Typedef* wav);

void WavePlayer_waitPlayDone(void);
void WavePlayer_setVolumeScale(float scale);

