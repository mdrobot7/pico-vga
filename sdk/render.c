#include "sdk.h"
#include "font.h"

#include "pico/multicore.h"
#include "pico/malloc.h"

volatile uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];

volatile RenderQueueItem background = { //First element of the linked list, can be reset to any background
    .type = 'f',
    .color = 0,
    .obj = NULL,
    .next = NULL,
    .flags = RQI_UPDATE
};
volatile RenderQueueItem *lastItem = &background; //Last item in linked list, used to set *last in RenderQueueItem

uint8_t autoRender = 0;
static volatile uint8_t update = 0;

void updateDisplay() {
    update = 1;
    if(autoRender) background.flags |= 1u; //force-update in autoRender mode
}

//Checks for out-of-bounds coordinates and writes to the frame
static inline void writePixel(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= FRAME_WIDTH || y >= FRAME_HEIGHT) return;
    else frame[y][x] = color;
}

void render() {
    multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    RenderQueueItem *item;
    RenderQueueItem *previousItem;

    while(1) {
        if(autoRender) {
            item = (RenderQueueItem *) &background;
            //look for the first item that needs an update, render that item and everything after it
            while(!(item->flags & 1u)) {
                previousItem = item;
                item = item->next;
                if(item == NULL) item = (RenderQueueItem *) &background;
            }
            if(((item->flags >> 1) & 1u) || item->type == 'n') item = (RenderQueueItem *) &background; //if the update is to hide an item, rerender the whole thing
        }
        else { //manual rendering
            while(!update); //wait until it's told to update the display
            item = (RenderQueueItem *) &background;
        }

        while(item != NULL) {
            if(!((item->flags >> 1) & 1u)) {
            switch(item->type) {
                case 'p': //Pixel
                    writePixel(item->y1, item->x1, item->color);
                    break;
                case 'l': //Line
                    if(item->x1 == item->x2) {
                        for(uint16_t y = item->y1; y <= item->y2; y++) writePixel(y, item->x1, item->color);
                    }
                    else {
                        float m = ((float)(item->y2 - item->y1))/((float)(item->x2 - item->x1));
                        for(uint16_t x = item->x1; x <= item->x2; x++) {
                            writePixel((uint16_t)((m*(x - item->x1)) + item->y1), x, item->color);
                        }
                    }
                    break;
                case 'r': //Rectangle
                    for(uint16_t x = item->x1; x <= item->x2; x++) writePixel(item->y1, x, item->color); //top
                    for(uint16_t x = item->x1; x <= item->x2; x++) writePixel(item->y2, x, item->color); //bottom
                    for(uint16_t y = item->y1; y <= item->y2; y++) writePixel(y, item->x1, item->color); //left
                    for(uint16_t y = item->y1; y <= item->y2; y++) writePixel(y, item->x2, item->color); //right
                    break;
                case 't': //Triangle
                    break;
                case 'o': //Circle
                    //x1, y1 = center, x2 = radius
                    for(uint16_t y = item->y1 - item->x2; y <= item->y1 + item->x2; y++) {
                        uint16_t x = sqrt(pow(item->x2, 2.0) - pow(y - item->y1, 2.0));
                        writePixel(y, item->x1 + x, item->color); //handle the +/- from the sqrt (the 2 sides of the circle)
                        writePixel(y, item->x1 - x, item->color);
                    }
                    break;
                case 'R': //Filled Rectangle
                    for(uint16_t y = item->y1; y <= item->y2; y++) {
                        for(uint16_t x = item->x1; x <= item->x2; x++) {
                            writePixel(y, x, item->color);
                        }
                    }
                    break;
                case 'T': //Filled Triangle
                    break;
                case 'O': //Filled Circle
                    //x1, y1 = center, x2 = radius
                    for(uint16_t y = item->y1 - item->x2; y <= item->y1 + item->x2; y++) {
                        uint16_t x = sqrt(pow(item->x2, 2.0) - pow(y - item->y1, 2.0));
                        for(uint16_t i = item->x1 - x; i <= item->x1 + x; i++) {
                            writePixel(y, i, item->color);
                        }
                    }
                    break;
                case 'c': //Character/String
                    /*for(uint16_t i = 0; item->obj[i] != '\0'; i++) {
                        x = item->x1 + (i*CHAR_WIDTH);
                        for(uint8_t j; j < CHAR_WIDTH; j++) {
                            //font[][] is a bit array, so check if a particular bit is true, if so, set the pixel array
                            if(((*font)[item->obj[i]][currentLine - item->y1]) & ((1 << (CHAR_WIDTH - 1)) >> j)) {
                                frame[l][x + j] = item->color;
                            }
                        }
                    }*/
                    break;
                case 's': //Sprites
                    /*for(uint16_t i = item->x1; i < item->x2; i++) {
                        //Skip changing the pixel if it's set to the COLOR_NULL value
                        if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;
                        if(item->color == 0) {
                            //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                            frame[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                        else {
                            frame[l][i] = item->color;
                        }
                    }*/
                    break;
                case 'f': //Fill the screen
                    for(uint16_t y = 0; y < FRAME_HEIGHT; y++) {
                        for(uint16_t x = 0; x < FRAME_WIDTH; x++) {
                            writePixel(y, x, item->color);
                        }
                    }
                    /*if(item->obj == NULL) {
                        for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                            frame[l][i] = item->color;
                        }
                    }
                    else {
                        for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                            //Skip changing the pixel if it's set to the COLOR_NULL value
                            if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;
                            //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                            frame[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                    }*/
                    break;
                case 'n': //Deleted item (garbage collector)
                    previousItem->next = item->next;
                    free(item);
                    item = previousItem; //prevent the code from skipping items
                    break;
            }
            }
            
            item->flags &= ~(1u); //Clear the update bit
            previousItem = item;
            item = item->next;
        }

        update = 0;
    }
}