#ifndef GAME_H
#define GAME_H

#include "pico/stdlib.h"
#include "../lib/pico-vga.h"

//Main Game loop -- DO NOT REMOVE!
void game();


//Other Game functions


//Sprites
// - Must be:
//    - Arrays of uint8_t (unsigned char), extern, and const (consts aren't loaded into RAM)
//    - Less than the frame dimensions -- [FRAME_HEIGHT][FRAME_WIDTH]

extern const uint8_t sprite1[5][3]; //A 5x3 sprite -- 5 rows, 3 cols

#endif