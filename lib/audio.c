#include "lib-internal.h"

/*
How audio is going to work:

songs are going to be stored with a header and an array of Note_t's, each note holding
*just* a frequency.

when you want to play a song/sound, you run a function that sets up 4 dma channels:
- L data -- copies data from the array to the PWM frequency register
- L control -- resets the data channel so the sound loops (optional)
- R data
- R control

Then, the function sets the DMA TREQ to a transfer pacing timer, with the timer frequency set to
the BPM of the song / the smallest note (16th note, say, so div by 16).

The array of Note_t's holds the song as if it was all 16th notes (for example), with imaginary
slurs and ties to make notes longer. Then you can hand all of the transferring over to the DMA


Alt solution (saves a bunch of RAM, but requries the CPU for stuff):
Play a note, set a timer for the duration of the note that fires an interrupt when it expires. When
the interrupt fires, reconfigure PWM, increment your position in the Note list, then
reset the timer for the new note duration.

Alt solution 2:
Same as alt solution 1, but use PWM as your timer instead of the standard timer blocks.

*/

typedef struct __packed {
    uint16_t freq;
} Note_t;

typedef struct {
    uint8_t defaultDuration;
    uint8_t defaultOctave;
    uint8_t defaultTempo; //BPM
    Note_t * notes;
} Sound_t;

int initAudio() {

}

int deInitAudio() {
    return 0;
}