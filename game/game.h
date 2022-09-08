#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include "pico/stdlib.h"

//Main Game loop -- DO NOT REMOVE!
void game();


//Other Game functions


//Sprites
// - Must be:
//    - Arrays of uint8_t (unsigned char), extern, and const (consts aren't loaded into RAM)
//    - Less than the frame dimensions -- [240][320]

extern const uint8_t sprite1[5][3]; //A 5x3 sprite -- 5 rows, 3 cols

#endif