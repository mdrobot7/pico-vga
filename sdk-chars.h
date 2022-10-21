#ifndef SDK_CHARS_H
#define SDK_CHARS_H

#include "main.h"

/*
- This file contains constant bit arrays representing the layout of the IBM CP437 character list.

- It is based off of the Adafruit adaptation of CP437, image link below. Smaller character dims, easier to work with
- https://learn.adafruit.com/assets/103682

- Most letters are 5x7 pixels, with one pixel spacing on the right and bottom. Total dimensions: 6x8
- Some specific letters are 5x8, with one extra row of pixels, usually on the bottom.
  All of these characters are listed below.

  0x08, 0x0A, 0x0F, 0x15, 0x17, 0x2C, 

*/

const uint8_t cp437[256][8] = {
  { 0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000 }, //0x00, Null character
  { 0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b11011,
    0b10001,
    0b01110,
    0b00000 }, //0x01, frowny face
  { 0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10001,
    0b11011,
    0b01110,
    0b00000 }, //0x02, smiley face
  { 0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000 }, //0x03, heart
  { 0b00000,
    0b00100,
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000 }, //0x04, diamond
  { 0b01110,
    0b01010,
    0b11111,
    0b10101,
    0b11111,
    0b00100,
    0b01110,
    0b00000 }, //0x05, club
  { 0b00100,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b00100,
    0b01110,
    0b00000 }, //0x06, spade
  { 0b00000,
    0b00000,
    0b00100,
    0b01110,
    0b01110,
    0b00100,
    0b00000,
    0b00000 }, //0x07, dot
  { 0b11111,
    0b11111,
    0b11011,
    0b10001,
    0b10001,
    0b11011,
    0b11111,
    0b11111 }, //0x08, inverted dot
  { 0b00000,
    0b00000,
    0b00100,
    0b01010,
    0b01010,
    0b00100,
    0b00000,
    0b00000 }, //0x09, circle
  { 0b11111,
    0b11111,
    0b11011,
    0b10101,
    0b10101,
    0b11011,
    0b11111,
    0b11111 }, //0x0A, inverted circle
  { 0b00000,
    0b00111,
    0b00011,
    0b01101,
    0b10100,
    0b10100,
    0b01000,
    0b00000 }, //0x0B, male
  { 0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b00100,
    0b11111,
    0b00100,
    0b00000 }, //0x0C, female
  { 0b01111,
    0b01001,
    0b01111,
    0b01000,
    0b01000,
    0b01000,
    0b11000,
    0b00000 }, //0x0D, single eighth note
  { 0b01111,
    0b01001,
    0b01111,
    0b01001,
    0b01001,
    0b01011,
    0b11000,
    0b00000 }, //0x0E, double eighth note
  { 0b00100,
    0b10101,
    0b01110,
    0b11011,
    0b11011,
    0b01110,
    0b10101,
    0b00100 }, //0x0F, blast
  { 0b10000,
    0b11000,
    0b11110,
    0b11111,
    0b11110,
    0b11000,
    0b10000,
    0b00000 }, //0x10, right triangle arrow
  { 0b00001,
    0b00011,
    0b01111,
    0b11111,
    0b01111,
    0b00011,
    0b00001,
    0b00000 }, //0x11, left triangle arrow
  { 0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b10101,
    0b01110,
    0b00100,
    0b00000 }, //0x12, up and down arrow
  { 0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b00000,
    0b11011,
    0b00000 }, //0x13, warning
  { 0b01111,
    0b10101,
    0b10101,
    0b01101,
    0b00101,
    0b00101,
    0b00101,
    0b00000 }, //0x14, new paragraph
  { 0b00110,
    0b01001,
    0b01010,
    0b00101,
    0b00010,
    0b01001,
    0b01001,
    0b00110 }, //0x15, section mark
  { 0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b00000 }, //0x16, cursor
  { 0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b10101,
    0b01110,
    0b00100,
    0b11111 }, //0x17, up and down arrow with base
  { 0b00000,
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b00100,
    0b00000 }, //0x18, up arrow
  { 0b00000,
    0b00100,
    0b00100,
    0b00100,
    0b10101,
    0b01110,
    0b00100,
    0b00000 }, //0x19, down arrow
  { 0b00000,
    0b00100,
    0b00010,
    0b11111,
    0b00010,
    0b00100,
    0b00000,
    0b00000 }, //0x1A, right arrow
  { 0b00000,
    0b00100,
    0b01000,
    0b11111,
    0b01000,
    0b00100,
    0b00000,
    0b00000 }, //0x1B, left arrow
  { 0b00000,
    0b10000,
    0b10000,
    0b10000,
    0b11111,
    0b00000,
    0b00000,
    0b00000 }, //0x1C, right angle
  { 0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01010,
    0b00000,
    0b00000,
    0b00000 }, //0x1D, left and right arrow
  { 0b00000,
    0b00100,
    0b00100,
    0b01110,
    0b11111,
    0b11111,
    0b00000,
    0b00000 }, //0x1E, up triangle arrow
  { 0b00000,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00100,
    0b00000,
    0b00000 }, //0x1F, down triangle arrow
  { 0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000 }, //0x20, space
  { 0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00000,
    0b00100,
    0b00000 }, //0x21, exclamation point
  { 0b01010,
    0b01010,
    0b01010,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000 }, //0x22, quotation mark
  { 0b01010,
    0b01010,
    0b11111,
    0b01010,
    0b11111,
    0b01010,
    0b01010,
    0b00000 }, //0x23, hashtag, pound, octothorp
  { 0b00100,
    0b01111,
    0b10100,
    0b01110,
    0b00101,
    0b11110,
    0b00100,
    0b00000 }, //0x24, dollar sign
  { 0b11000,
    0b11001,
    0b00010,
    0b00100,
    0b01000,
    0b10011,
    0b00011,
    0b00000 }, //0x25, percent
  { 0b01000,
    0b10100,
    0b10100,
    0b01000,
    0b10101,
    0b10010,
    0b01101,
    0b00000 }, //0x26, ampersand
  { 0b00110,
    0b00110,
    0b00100,
    0b01000,
    0b00000,
    0b00000,
    0b00000,
    0b00000 }, //0x27, apostrophe
  { 0b00010,
    0b00100,
    0b01000,
    0b01000,
    0b01000,
    0b00100,
    0b00010,
    0b00000 }, //0x28, open parenthesis
  { 0b01000,
    0b00100,
    0b00010,
    0b00010,
    0b00010,
    0b00100,
    0b01000,
    0b00000 }, //0x29, close parenthesis
  { 0b00100,
    0b10101,
    0b01110,
    0b11111,
    0b01110,
    0b10101,
    0b00100,
    0b00000 }, //0x2A, asterisk
  { 0b00000,
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b00100,
    0b00000,
    0b00000 }, //0x2B, plus sign
  { 0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00110,
    0b00110,
    0b00100,
    0b01000 }, //0x2C, comma
  { 0b00000,
    0b00000,
    0b11111,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000 }, //0x2D, minus, dash
  { 0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00110,
    0b00110,
    0b00000 }, //0x2E, period
  { 0b00000,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b10000,
    0b00000,
    0b00000 }, //0x2F, forward slash
};

#endif