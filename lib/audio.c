#include "lib-internal.h"
#include "hardware/pwm.h"

#define BASE_SAMPLE_FREQ_HZ 11025
#define MAX_SAMPLE_FREQ_HZ 44100
#define SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ 2
#define PWM_BITS_PER_SAMP 8

#define BITS_PER_SAMP_32 24
#define BITS_PER_SAMP_16 8

typedef struct {
    uint8_t bits_per_samp; // = 24 for 32bit samples, = 8 for 16bit samples (see defines above)
    uint8_t sample_freq_mult; //this*BASE_SAMPLE_FREQ_HZ = sample freqency
    uint8_t num_channels;
    uint8_t vol; //relative volume, for mixing. 0 (loudest) - 7 (quietest)
    bool wrap;
    bool cache_after_play; //keep the sound in RAM even after it has been played
    uint32_t len_data_bytes;
    int32_t * data; //audio data is SIGNED!
} sound_t;

#define SOUND_QUEUE_LEN 8
static sound_t * sound_queue_l[SOUND_QUEUE_LEN];
static sound_t * sound_queue_r[SOUND_QUEUE_LEN];
static uint32_t sound_i_l[SOUND_QUEUE_LEN];
static uint32_t sound_i_r[SOUND_QUEUE_LEN];

//lower = louder, max vol = 1
static uint8_t volume = 1;

/**
 * @brief Helper method to do the math on a particular audio sample (16 bit ONLY)
 * 
 * @param s 
 * @param data_i 
 * @return uint8_t 
 */
static inline uint32_t parse_audio_data_16(sound_t * s, uint32_t data_i) {
    int32_t temp;
    temp = ((int16_t *)(s->data))[data_i >> s->sample_freq_mult];
    temp = temp >> BITS_PER_SAMP_16;
    temp += 1u << (PWM_BITS_PER_SAMP - 1); //get rid of the sign, now it's 0-255 instead of -128 - 127
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
static inline uint32_t parse_audio_data_32(sound_t * s, uint32_t data_i) {
    int32_t temp;
    temp = ((int32_t *)(s->data))[data_i >> s->sample_freq_mult];
    temp = temp >> BITS_PER_SAMP_32;
    temp += 1u << (PWM_BITS_PER_SAMP - 1); //get rid of the sign, now it's 0-255 instead of -128 - 127
    temp = temp >> s->vol;
    return temp;
}

/**
 * @brief Handle setting the PWM duty cycle for the audio pins
 * 
 * @return irq_handler_t 
 */
irq_handler_t audio_irq_handler() {
    uint32_t l = 0;
    uint32_t r = 0;

    uint8_t num_l = 0;
    uint8_t num_r = 0;

    /*
    Takes all of the data for a particular channel and performs a weighted average. Also handles overflow.
    */
    for(int i = 0; i < SOUND_QUEUE_LEN; i++) {
        if(sound_queue_l[i] != NULL) {
            if(sound_queue_l[i]->bits_per_samp == BITS_PER_SAMP_16) {
                l += parse_audio_data_16(sound_queue_l[i], sound_i_l[i]);

                //bitshifts account for len being in bytes (not uint16_ts) and supersampling
                if(sound_i_l[i] >= (sound_queue_l[i]->len_data_bytes >> 1) << sound_queue_l[i]->sample_freq_mult) {
                    if(sound_queue_l[i]->wrap) sound_i_l[i] = 0;
                    else sound_queue_l[i] = NULL; //handle removing song from RAM/deallocating here
                }
            }
            else if(sound_queue_l[i]->bits_per_samp == BITS_PER_SAMP_32) {
                l += parse_audio_data_32(sound_queue_l[i], sound_i_l[i]);

                //bitshifts account for len being in bytes (not uint32_ts) and supersampling
                if(sound_i_l[i] >= (sound_queue_l[i]->len_data_bytes >> 2) << sound_queue_l[i]->sample_freq_mult) {
                    if(sound_queue_l[i]->wrap) sound_i_l[i] = 0;
                    else sound_queue_l[i] = NULL; //handle removing song from RAM/deallocating here
                }
            }
            sound_i_l[i]++;
            if(sound_queue_l[i]->num_channels == 2) sound_i_l[i]++;
            num_l++;
        }

        if(sound_queue_r[i] != NULL) {
            if(sound_queue_r[i]->bits_per_samp == BITS_PER_SAMP_16) {
                r += parse_audio_data_16(sound_queue_r[i], sound_i_r[i]);

                //bitshifts account for len being in bytes (not uint16_ts) and supersampling
                if(sound_i_r[i] >= (sound_queue_r[i]->len_data_bytes >> 1) << sound_queue_r[i]->sample_freq_mult) {
                    if(sound_queue_r[i]->wrap) sound_i_r[i] = 0;
                    else sound_queue_r[i] = NULL; //handle removing song from RAM/deallocating here
                }
            }
            else if(sound_queue_r[i]->bits_per_samp == BITS_PER_SAMP_32) {
                r += parse_audio_data_32(sound_queue_r[i], sound_i_r[i]);

                //bitshifts account for len being in bytes (not uint32_ts) and supersampling
                if(sound_i_r[i] >= (sound_queue_r[i]->len_data_bytes >> 2) << sound_queue_r[i]->sample_freq_mult) {
                    if(sound_queue_r[i]->wrap) sound_i_r[i] = 0;
                    else sound_queue_r[i] = NULL; //handle removing song from RAM/deallocating here
                }
            }
            sound_i_r[i]++;
            if(sound_queue_r[i]->num_channels == 2) sound_i_r[i]++;
            num_r++;
        }
    }

    if(num_l != 0) l /= num_l; //average the sound data for all of the currently playing sounds to mix them together
    if(num_r != 0) r /= num_r;

    l /= volume;
    r /= volume;

    pwm_set_chan_level(AUDIO_L_PWM_SLICE, AUDIO_L_PWM_CHAN, l);
    pwm_set_chan_level(AUDIO_R_PWM_SLICE, AUDIO_R_PWM_CHAN, r);

    pwm_clear_irq(AUDIO_L_PWM_SLICE);
    pwm_clear_irq(AUDIO_R_PWM_SLICE);
}

int initAudio() {
    if(clock_get_hz(clk_sys) < 120000000) return 1;

    gpio_set_function(AUDIO_L_PIN, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_R_PIN, GPIO_FUNC_PWM);

    pwm_set_clkdiv(AUDIO_L_PWM_SLICE, (float)clock_get_hz(clk_sys)/(MAX_SAMPLE_FREQ_HZ * SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ * PWM_BITS_PER_SAMP));
    pwm_set_clkdiv(AUDIO_R_PWM_SLICE, (float)clock_get_hz(clk_sys)/(MAX_SAMPLE_FREQ_HZ * SUPER_SAMPLE_AT_MAX_SAMPLE_FREQ * PWM_BITS_PER_SAMP));

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
    irq_set_exclusive_handler(PWM_IRQ_WRAP, audio_irq_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

int deInitAudio() {
    pwm_set_enabled(AUDIO_L_PWM_SLICE, false);
    pwm_set_enabled(AUDIO_R_PWM_SLICE, false);

    irq_set_enabled(PWM_IRQ_WRAP, false);
}

/**
 * @brief Get a blank sound_t struct.
 * 
 * @return sound_t 
 */
sound_t audio_get_default_sound() {
    sound_t sound;
    return sound;
}

/**
 * @brief Set the number of bits per sample for a particular sound
 * 
 * @param sound 
 * @param bits_per_samp Either 16 or 32 bits/sample
 */
void audio_set_bits_per_sample(sound_t * sound, uint8_t bits_per_samp) {
    if(bits_per_samp == 16) sound->bits_per_samp = BITS_PER_SAMP_16;
    else sound->bits_per_samp = BITS_PER_SAMP_32;
}

/**
 * @brief Set the sample rate of a particular sound.
 * 
 * @param sound
 * @param sample_rate Supported sample rates: 11025 Hz, 22050 Hz, 44100 Hz.
 */
void audio_set_sample_rate(sound_t * sound, uint32_t sample_rate) {
    if(sample_rate != 11025 && sample_rate != 22050 && sample_rate != 44100) return;
    sound->sample_freq_mult = sample_rate/BASE_SAMPLE_FREQ_HZ;
}

/**
 * @brief Set the number of audio channels present in sound data
 * 
 * @param sound 
 * @param num_channels Either 1 or 2 channels
 */
void audio_set_num_channels(sound_t * sound, uint8_t num_channels) {
    if(num_channels > 2 || num_channels == 0) return;
    sound->num_channels = num_channels;
}

/**
 * @brief Set the relative volume "mixing" of sounds
 * 
 * @param sound 
 * @param vol Relative volume -- 0 is quietest, 7 is loudest
 */
void audio_set_sound_vol(sound_t * sound, uint8_t vol) {
    if(vol > 7) vol = 7;
    sound->vol = 8 - vol;
}

/**
 * @brief Set whether or not the sound should repeat infinitely after it ends
 * 
 * @param sound 
 * @param wrap 
 */
void audio_set_sound_wrap(sound_t * sound, bool wrap) {
    sound->wrap = wrap;
}

/**
 * @brief Set whether the sound should remain in RAM after it is played
 * 
 * @param sound 
 * @param cache If true, the processor will attempt to keep the sound data in RAM after the sound is played.
 *              If false, the processor will deallocate the memory after playing the song and use it for
 *              future sound data.
 * @return true if there is enough memory to cache the data, false otherwise
 */
bool audio_set_cache_after_play(sound_t * sound, bool cache) {
    sound->cache_after_play = cache;
}

/**
 * @brief Set the data for a sound
 * 
 * @param sound 
 * @param data A pointer to the start of the data buffer. It must be SIGNED int16_ts or int32_ts
 *             (signed int or signed short)
 * @param len_data_bytes The length of the data in **bytes**
 */
void audio_set_data(sound_t * sound, void * data, uint32_t len_data_bytes) {
    sound->data = data;
    sound->len_data_bytes = len_data_bytes;
}

typedef enum {
    SOUND_CHANNEL_MONO_L,
    SOUND_CHANNEL_MONO_R,
    SOUND_CHANNEL_MONO_BOTH,
    SOUND_CHANNEL_STEREO,
} sound_channel_type_t;

/**
 * @brief Play the sound using a specific channel configuration
 * 
 * @param sound 
 * @param channel_type See sound_channel_type_t enum. Stereo audio data (2 channels) can ONLY be played
 *                     stereo, using both channels.
 * @return int Returns a nonzero value if the function fails.
 */
int play_sound(sound_t * sound, sound_channel_type_t channel_type) {
    int l = -1, r = -1;
    for(int i = 0; i < SOUND_QUEUE_LEN; i++) {
        if(sound_queue_l[i] == NULL) {
            l = i;
        }
        if(sound_queue_r[i] == NULL) {
            r = i;
        }
    }
    if(channel_type != SOUND_CHANNEL_MONO_R && l == -1) return 1; //no space in sound queue, fail
    if(channel_type != SOUND_CHANNEL_MONO_L && r == -1) return 1;

    if(channel_type == SOUND_CHANNEL_MONO_L) {
        sound_queue_l[l] = sound;
    }
    else if(channel_type == SOUND_CHANNEL_MONO_R) {
        sound_queue_r[r] = sound;
    }
    else if(channel_type == SOUND_CHANNEL_MONO_BOTH || channel_type == SOUND_CHANNEL_STEREO) {
        sound_queue_l[l] = sound;
        sound_queue_r[r] = sound;
    }
    return 0;
}