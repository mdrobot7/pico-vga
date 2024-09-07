#include "audio.h"

#include "../common.h"
#include "../pinout.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

#define BASE_SAMPLE_FREQ_HZ             11025
#define MAX_SAMPLE_FREQ_HZ              44100
#define SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ 2
#define PWM_BITS_PER_SAMP               8

#define BITS_PER_SAMP_32 24
#define BITS_PER_SAMP_16 8

#define SOUND_QUEUE_LEN 8

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

static audio_sound_t * sound_queue_l[SOUND_QUEUE_LEN];
static audio_sound_t * sound_queue_r[SOUND_QUEUE_LEN];
static uint32_t sound_i_l[SOUND_QUEUE_LEN];
static uint32_t sound_i_r[SOUND_QUEUE_LEN];

// lower = louder, max vol = 1
static uint8_t volume = 1;

/************************************
 * STATIC FUNCTIONS
 ************************************/

/**
 * @brief Finds an empty index in a sound queue
 *
 * @param sound_queue The sound queue to look in
 * @return uint32_t The first NULL index, or 0xFFFF if none found.
 */
static uint32_t find_empty_sound_queue_index(audio_sound_t * sound_queue[]) {
  for (int i = 0; i < SOUND_QUEUE_LEN; i++) {
    if (sound_queue[i] == NULL) return i;
  }
  return 0xFFFF;
}

/**
 * @brief Get the number of empty indices in a sound queue
 *
 * @param sound_queue The sound queue to look in
 * @return uint32_t The number of empty indices in the sound queue
 */
static uint32_t num_empty_sound_queue_indices(audio_sound_t * sound_queue[]) {
  uint32_t num = 0;
  for (int i = 0; i < SOUND_QUEUE_LEN; i++) {
    if (sound_queue[i] == NULL) num++;
  }
  return num;
}

/**
 * @brief Helper method to do the math on a particular audio sample (16 bit ONLY)
 *
 * @param s
 * @param data_i
 * @return uint8_t
 */
static inline uint32_t parse_audio_data_16(audio_sound_t * s, uint32_t data_i) {
  int32_t temp;
  temp = ((int16_t *) (s->data))[data_i >> s->sample_freq_mult];
  temp = temp >> BITS_PER_SAMP_16;
  temp += 1u << (PWM_BITS_PER_SAMP - 1); // get rid of the sign, now it's 0-255 instead of -128 - 127
  temp = temp >> s->vol;
  return temp;
}

/**
 * @brief Helper method to do the math on a particular audio sample (32 bit ONLY)
 *
 * @param s
 * @param data_i
 * @return uint8_t
 */
static inline uint32_t parse_audio_data_32(audio_sound_t * s, uint32_t data_i) {
  int32_t temp;
  temp = ((int32_t *) (s->data))[data_i >> s->sample_freq_mult];
  temp = temp >> BITS_PER_SAMP_32;
  temp += 1u << (PWM_BITS_PER_SAMP - 1); // get rid of the sign, now it's 0-255 instead of -128 - 127
  temp = temp >> s->vol;
  return temp;
}

/**
 * @brief Remove a song from the sound queue. Handles deallocating the audio_sound_t.data[] memory space,
 *        if applicable (NOT IMPLEMENTED YET).
 *
 * @param sound_queue
 * @param sound_i
 * @param index
 */
static inline void remove_song(audio_sound_t * sound_queue[], uint32_t sound_i[], uint32_t index) {
  sound_queue[index] = NULL;
  sound_i[index]     = 0;
}

/**
 * @brief Handle setting the PWM duty cycle for the audio pins
 *
 */
static void audio_irq_handler() {
  uint32_t l = 0;
  uint32_t r = 0;

  uint8_t num_l = 0;
  uint8_t num_r = 0;

  /*
  Takes all of the data for a particular channel and performs a weighted average. Also handles overflow.
  */
  for (int i = 0; i < SOUND_QUEUE_LEN; i++) {
    if (sound_queue_l[i] != NULL) {
      if (sound_queue_l[i]->bits_per_samp == BITS_PER_SAMP_16) {
        l += parse_audio_data_16(sound_queue_l[i], sound_i_l[i]);

        // bitshifts account for len being in bytes (not uint16_ts) and supersampling
        if (sound_i_l[i] >= (sound_queue_l[i]->len_data_bytes >> 1) << sound_queue_l[i]->sample_freq_mult) {
          if (sound_queue_l[i]->wrap)
            sound_i_l[i] = 0;
          else
            sound_queue_l[i] = NULL; // handle removing song from RAM/deallocating here
        }
      } else if (sound_queue_l[i]->bits_per_samp == BITS_PER_SAMP_32) {
        l += parse_audio_data_32(sound_queue_l[i], sound_i_l[i]);

        // bitshifts account for len being in bytes (not uint32_ts) and supersampling
        if (sound_i_l[i] >= (sound_queue_l[i]->len_data_bytes >> 2) << sound_queue_l[i]->sample_freq_mult) {
          if (sound_queue_l[i]->wrap)
            sound_i_l[i] = 0;
          else
            sound_queue_l[i] = NULL; // handle removing song from RAM/deallocating here
        }
      }
      sound_i_l[i]++;
      if (sound_queue_l[i]->num_channels == 2) sound_i_l[i]++;
      num_l++;
    }

    if (sound_queue_r[i] != NULL) {
      if (sound_queue_r[i]->bits_per_samp == BITS_PER_SAMP_16) {
        r += parse_audio_data_16(sound_queue_r[i], sound_i_r[i]);

        // bitshifts account for len being in bytes (not uint16_ts) and supersampling
        if (sound_i_r[i] >= (sound_queue_r[i]->len_data_bytes >> 1) << sound_queue_r[i]->sample_freq_mult) {
          if (sound_queue_r[i]->wrap)
            sound_i_r[i] = 0;
          else
            sound_queue_r[i] = NULL; // handle removing song from RAM/deallocating here
        }
      } else if (sound_queue_r[i]->bits_per_samp == BITS_PER_SAMP_32) {
        r += parse_audio_data_32(sound_queue_r[i], sound_i_r[i]);

        // bitshifts account for len being in bytes (not uint32_ts) and supersampling
        if (sound_i_r[i] >= (sound_queue_r[i]->len_data_bytes >> 2) << sound_queue_r[i]->sample_freq_mult) {
          if (sound_queue_r[i]->wrap)
            sound_i_r[i] = 0;
          else
            sound_queue_r[i] = NULL; // handle removing song from RAM/deallocating here
        }
      }
      sound_i_r[i]++;
      if (sound_queue_r[i]->num_channels == 2) sound_i_r[i]++;
      num_r++;
    }
  }

  if (num_l != 0) l /= num_l; // average the sound data for all of the currently playing sounds to mix them together
  if (num_r != 0) r /= num_r;

  l /= volume;
  r /= volume;

  pwm_set_chan_level(AUDIO_L_PWM_SLICE, AUDIO_L_PWM_CHAN, l);
  pwm_set_chan_level(AUDIO_R_PWM_SLICE, AUDIO_R_PWM_CHAN, r);

  pwm_clear_irq(AUDIO_L_PWM_SLICE);
  pwm_clear_irq(AUDIO_R_PWM_SLICE);
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

int audio_init(audio_config_t * config) {
  UNUSED(config);
  if (clock_get_hz(clk_sys) < 120000000) return 1;

  gpio_set_function(AUDIO_L_PIN, GPIO_FUNC_PWM);
  gpio_set_function(AUDIO_R_PIN, GPIO_FUNC_PWM);

  pwm_set_clkdiv(AUDIO_L_PWM_SLICE, (float) clock_get_hz(clk_sys) / (MAX_SAMPLE_FREQ_HZ * SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ * PWM_BITS_PER_SAMP));
  pwm_set_clkdiv(AUDIO_R_PWM_SLICE, (float) clock_get_hz(clk_sys) / (MAX_SAMPLE_FREQ_HZ * SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ * PWM_BITS_PER_SAMP));

  pwm_set_wrap(AUDIO_L_PWM_SLICE, 1u << PWM_BITS_PER_SAMP);
  pwm_set_wrap(AUDIO_R_PWM_SLICE, 1u << PWM_BITS_PER_SAMP);

  pwm_set_chan_level(AUDIO_L_PWM_SLICE, AUDIO_L_PWM_CHAN, 0);
  pwm_set_chan_level(AUDIO_R_PWM_SLICE, AUDIO_R_PWM_CHAN, 0);

  pwm_set_enabled(AUDIO_L_PWM_SLICE, true);
  pwm_set_enabled(AUDIO_L_PWM_SLICE, true);

  pwm_clear_irq(AUDIO_L_PWM_SLICE);
  pwm_clear_irq(AUDIO_R_PWM_SLICE);
  pwm_set_irq_enabled(AUDIO_L_PWM_SLICE, true);
  pwm_set_irq_enabled(AUDIO_R_PWM_SLICE, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, (irq_handler_t) audio_irq_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  return 0;
}

int audio_deinit(audio_config_t * config) {
  UNUSED(config);
  pwm_set_enabled(AUDIO_L_PWM_SLICE, false);
  pwm_set_enabled(AUDIO_R_PWM_SLICE, false);

  irq_set_enabled(PWM_IRQ_WRAP, false);
  return 0;
}

audio_sound_t audio_get_default_sound() {
  audio_sound_t sound = { 0 };
  return sound;
}

void audio_set_bits_per_sample(audio_sound_t * sound, uint8_t bits_per_samp) {
  if (bits_per_samp == 16)
    sound->bits_per_samp = BITS_PER_SAMP_16;
  else
    sound->bits_per_samp = BITS_PER_SAMP_32;
}

void audio_set_sample_rate(audio_sound_t * sound, uint32_t sample_rate) {
  if (sample_rate != 11025 && sample_rate != 22050 && sample_rate != 44100) return;
  sound->sample_freq_mult = sample_rate / BASE_SAMPLE_FREQ_HZ;
}

void audio_set_num_channels(audio_sound_t * sound, uint8_t num_channels) {
  if (num_channels > 2 || num_channels == 0) return;
  sound->num_channels = num_channels;
}

void audio_set_sound_vol(audio_sound_t * sound, uint8_t vol) {
  if (vol > 7) vol = 7;
  sound->vol = 8 - vol;
}

void audio_set_sound_wrap(audio_sound_t * sound, bool wrap) {
  sound->wrap = wrap;
}

void audio_set_cache_after_play(audio_sound_t * sound, bool cache) {
  sound->cache_after_play = cache;
}

void audio_set_data(audio_sound_t * sound, void * data, uint32_t len_data_bytes) {
  sound->data           = data;
  sound->len_data_bytes = len_data_bytes;
}

int audio_play(audio_sound_t * sound, audio_sound_channel_type_t channel_type) {
  if (channel_type == SOUND_CHANNEL_MONO_L) {
    if (num_empty_sound_queue_indices(sound_queue_l) < 1) return 1;
    sound_queue_l[find_empty_sound_queue_index(sound_queue_l)] = sound;
  } else if (channel_type == SOUND_CHANNEL_MONO_R) {
    if (num_empty_sound_queue_indices(sound_queue_r) < 1) return 1;
    sound_queue_r[find_empty_sound_queue_index(sound_queue_r)] = sound;
  } else if (channel_type == SOUND_CHANNEL_STEREO_L) {
    if (num_empty_sound_queue_indices(sound_queue_l) < 2) return 1;
    sound_queue_l[find_empty_sound_queue_index(sound_queue_l)] = sound;                                       // one sound queue index used per channel.
    uint32_t l2                                                = find_empty_sound_queue_index(sound_queue_l); // need to start one counter at 1 otherwise
    sound_queue_l[l2]                                          = sound;                                       // both sounds will be the same channel
    sound_i_l[l2]                                              = 1;
  } else if (channel_type == SOUND_CHANNEL_STEREO_R) {
    if (num_empty_sound_queue_indices(sound_queue_r) < 2) return 1;
    sound_queue_r[find_empty_sound_queue_index(sound_queue_r)] = sound;
    uint32_t r2                                                = find_empty_sound_queue_index(sound_queue_r);
    sound_queue_r[r2]                                          = sound;
    sound_i_r[r2]                                              = 1;
  } else if (channel_type == SOUND_CHANNEL_MONO_BOTH || channel_type == SOUND_CHANNEL_STEREO) {
    if (num_empty_sound_queue_indices(sound_queue_l) < 1 || num_empty_sound_queue_indices(sound_queue_r) < 1) return 1;
    sound_queue_l[find_empty_sound_queue_index(sound_queue_l)] = sound;
    uint32_t r                                                 = find_empty_sound_queue_index(sound_queue_r);
    sound_queue_r[r]                                           = sound;
    if (channel_type == SOUND_CHANNEL_STEREO) {
      sound_i_r[r] = 1;
    }
  }
  return 0;
}