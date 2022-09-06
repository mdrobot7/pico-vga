#include "game.h"

//Sprites
//Must be defined as const, since const variables (on ARM) are NOT loaded into memory,
//they are read directly from flash. This means you can actually put a meaningful amount
//of sprites onto the pico.

const uint8_t sprite1[5][3] = { {0, 0, 0},
                                {0, 0, 0},
                                {0, 0, 0},
                                {0, 0, 0},
                                {0, 0, 0}};