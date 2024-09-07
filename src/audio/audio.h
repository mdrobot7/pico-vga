#ifndef __PV_AUDIO_H
#define __PV_AUDIO_H

#include "pico/stdlib.h"

/************************************
 * MACROS AND TYPEDEFS
 ************************************/

typedef struct {
  bool stereo; // False = mono audio, true = stereo
} audio_config_t;

// Default configuration for audio_config_t
#define AUDIO_CONFIG_DEFAULT { \
  .stereo = true,              \
}

typedef struct {
  uint8_t bits_per_samp;    // = 24 for 32bit samples, = 8 for 16bit samples (see defines above)
  uint8_t sample_freq_mult; // this*BASE_SAMPLE_FREQ_HZ = sample freqency
  uint8_t num_channels;
  uint8_t vol; // relative volume, for mixing. 0 (loudest) - 7 (quietest)
  bool wrap;
  bool cache_after_play; // keep the sound in RAM even after it has been played
  uint32_t len_data_bytes;
  int32_t * data; // audio data is SIGNED!
} audio_sound_t;

typedef enum {
  SOUND_CHANNEL_MONO_L,    // play the one channel through the left output only
  SOUND_CHANNEL_MONO_R,    // play the one channel through the right output only
  SOUND_CHANNEL_MONO_BOTH, // play the one channel through both the left and right outputs
  SOUND_CHANNEL_STEREO_L,  // play both channels through the left channel output ONLY
  SOUND_CHANNEL_STEREO_R,  // play both channels through the right channel output ONLY
  SOUND_CHANNEL_STEREO,
} audio_sound_channel_type_t;

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

/**
 * @brief Initialize the audio module
 *
 * @param config Audio config struct
 * @return int 0 on success, nonzero otherwise
 */
int audio_init(audio_config_t * config);

/**
 * @brief Deinitialize the audio module
 *
 * @param config Audio config struct
 * @return int 0 on success, nonzero otherwise
 */
int audio_deinit(audio_config_t * config);

/**
 * @brief Get a blank audio_sound_t struct.
 *
 * @return audio_sound_t
 */
audio_sound_t audio_get_default_sound();

/**
 * @brief Set the number of bits per sample for a particular sound
 *
 * @param sound
 * @param bits_per_samp Either 16 or 32 bits/sample
 */
void audio_set_bits_per_sample(audio_sound_t * sound, uint8_t bits_per_samp);

/**
 * @brief Set the sample rate of a particular sound.
 *
 * @param sound
 * @param sample_rate Supported sample rates: 11025 Hz, 22050 Hz, 44100 Hz.
 */

void audio_set_sample_rate(audio_sound_t * sound, uint32_t sample_rate);

/**
 * @brief Set the number of audio channels present in sound data
 *
 * @param sound
 * @param num_channels Either 1 or 2 channels
 */
void audio_set_num_channels(audio_sound_t * sound, uint8_t num_channels);

/**
 * @brief Set the relative volume "mixing" of sounds
 *
 * @param sound
 * @param vol Relative volume -- 0 is quietest, 7 is loudest
 */
void audio_set_sound_vol(audio_sound_t * sound, uint8_t vol);

/**
 * @brief Set whether or not the sound should repeat infinitely after it ends
 *
 * @param sound
 * @param wrap
 */
void audio_set_sound_wrap(audio_sound_t * sound, bool wrap);

/**
 * @brief Set whether the sound should remain in RAM after it is played
 *
 * @param sound
 * @param cache If true, the processor will attempt to keep the sound data in RAM after the sound is played.
 *              If false, the processor will deallocate the memory after playing the song and use it for
 *              future sound data.
 * @return true if there is enough memory to cache the data, false otherwise
 */
void audio_set_cache_after_play(audio_sound_t * sound, bool cache);

/**
 * @brief Set the data for a sound
 *
 * @param sound
 * @param data A pointer to the start of the data buffer. It must be SIGNED int16_ts or int32_ts
 *             (signed int or signed short)
 * @param len_data_bytes The length of the data in **bytes**
 */
void audio_set_data(audio_sound_t * sound, void * data, uint32_t len_data_bytes);

/**
 * @brief Play the sound using a specific channel configuration
 *
 * @param sound
 * @param channel_type What output channel(s) to use to play the sound. See audio_sound_channel_type_t enum.
 * @return int Returns a nonzero value if the function fails.
 */
int audio_play(audio_sound_t * sound, audio_sound_channel_type_t channel_type);

#endif