#ifndef __PV_AUDIO_H
#define __PV_AUDIO_H

typedef struct {
  bool stereo; // False = mono audio, true = stereo
} AudioConfig_t;

// Default configuration for AudioConfig_t
#define AUDIO_CONFIG_DEFAULT { \
  .stereo = true,              \
}

#endif