#include "lib-internal.h"
#include "hardware/pwm.h"

#define MAX_SAMPLE_FREQ_HZ 44100
#define MAX_SUPER_SAMPLE_RATE 3

//TODO: scale _set_level for the different top values, figure out how the conversion script works

typedef struct {
    uint32_t bitrate;
    uint16_t sample_freq;
    uint16_t super_sample_rate;
    uint32_t len;
    uint32_t num_channels;
    uint8_t vol; //relative volume, for mixing. 0 (quietest) - 7 (loudest)
    bool wrap;
    uint16_t * data;
    uint32_t data_i;
} sound_t;

#define SOUND_QUEUE_LEN 8
static sound_t * sound_queue_l[SOUND_QUEUE_LEN];
static sound_t * sound_queue_r[SOUND_QUEUE_LEN];

static uint8_t volume = 1;

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
            l += sound_queue_l[i]->data[sound_queue_l[i]->data_i/sound_queue_l[i]->super_sample_rate] << sound_queue_l[i]->vol;
            sound_queue_l[i]->data_i++;
            if(sound_queue_l[i]->data_i >= sound_queue_l[i]->len * sound_queue_l[i]->super_sample_rate) {
                if(sound_queue_l[i]->wrap) sound_queue_l[i]->data_i = 0;
                else sound_queue_l[i] = NULL;
            }
            num_l++;
        }

        if(sound_queue_r[i] != NULL) {
            r += sound_queue_r[i]->data[sound_queue_r[i]->data_i/sound_queue_r[i]->super_sample_rate] << sound_queue_r[i]->vol;
            sound_queue_r[i]->data_i++;
            if(sound_queue_r[i]->data_i >= sound_queue_r[i]->len * sound_queue_r[i]->super_sample_rate) {
                if(sound_queue_r[i]->wrap) sound_queue_r[i]->data_i = 0;
                else sound_queue_r[i] = NULL;
            }
            num_r++;
        }
    }

    if(num_l != 0) l /= num_l; //average the sound data for all of the currently playing sounds to mix them together
    if(num_r != 0) r /= num_r;

    l /= volume;
    r /= volume;

    pwm_set_gpio_level(AUDIO_L_PIN, l);
    pwm_set_gpio_level(AUDIO_R_PIN, r);
}

int initAudio() {
    if(clock_get_hz(clk_sys) < 120000000) return 1;

    uint8_t slice_l = pwm_gpio_to_slice_num(AUDIO_L_PIN);
    bool chan_l = pwm_gpio_to_channel(AUDIO_L_PIN);
    uint8_t slice_r = pwm_gpio_to_slice_num(AUDIO_R_PIN);
    bool chan_r = pwm_gpio_to_channel(AUDIO_R_PIN);

    gpio_set_function(AUDIO_L_PIN, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_R_PIN, GPIO_FUNC_PWM);

    pwm_set_wrap(slice_l, clock_get_hz(clk_sys)/(MAX_SUPER_SAMPLE_RATE * MAX_SAMPLE_FREQ_HZ));
    pwm_set_wrap(slice_r, clock_get_hz(clk_sys)/(MAX_SUPER_SAMPLE_RATE * MAX_SAMPLE_FREQ_HZ));

    pwm_set_chan_level(slice_l, chan_l, 0);
    pwm_set_chan_level(slice_r, chan_r, 0);

    pwm_set_enabled(slice_l, true);
    pwm_set_enabled(slice_r, true);

    pwm_set_irq_enabled(slice_l, true);
    pwm_set_irq_enabled(slice_r, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, audio_irq_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

int deInitAudio() {
    uint8_t slice_l = pwm_gpio_to_slice_num(AUDIO_L_PIN);
    uint8_t slice_r = pwm_gpio_to_slice_num(AUDIO_R_PIN);

    pwm_set_enabled(slice_l, false);
    pwm_set_enabled(slice_r, false);

    irq_set_enabled(PWM_IRQ_WRAP, false);
}