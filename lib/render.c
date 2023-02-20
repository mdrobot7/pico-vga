#include "lib-internal.h"

#include "pico/multicore.h"
#include "pico/malloc.h"

//Pointer to the frame buffer (a 2d array, frameHeight*frameWidth)
volatile uint8_t * frame = NULL;

//Pointer to an array of pointers, each pointing to the start of a line in the frame buffer OR an interpolated line in line (size = full height of the BASE resolution).
uint8_t ** frameReadAddr = NULL;

//Pointer to an array of zeros the size of a line.
uint8_t * BLANK = NULL;

//Pointer to the interpolated lines (a 2d array, displayConfig->numInterpolatedLines*frameWidth)
uint8_t * line = NULL;

volatile RenderQueueItem_t background = { //First element of the linked list, can be reset to any background
    .type = RQI_T_FILL,
    .color = 0,
    .obj = NULL,
    .next = NULL,
    .flags = RQI_UPDATE
};
volatile RenderQueueItem_t *lastItem = &background; //Last item in linked list, used to set *last in RenderQueueItem

static volatile uint8_t update = 0;

void updateDisplay() {
    update = 1;
    if(displayConfig->autoRender) background.flags |= RQI_UPDATE; //force-update in autoRender mode
}

/*
        Render Utilities
================================
*/
//Checks for out-of-bounds coordinates and writes to the frame
static inline void writePixel(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= frameWidth || y >= frameWidth) return;
    //else frame[y][x] = color;
}

static inline uint8_t getFontBit(uint8_t c, uint8_t x, uint8_t y) {
    return *(font + CHAR_HEIGHT*c + y) >> (CHAR_WIDTH - x + 1) & 1u;
}

/*
        Render Functions
================================
*/
static inline void renderLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t scale) {
    if(x1 == x2) {
        for(uint16_t y = y1; y <= y2; y++) writePixel(y, x1, color);
    }
    else if(y1 == y2) {
        for(uint16_t x = x1; x <= x2; x++) writePixel(y1, x, color);
    }
    else {
        float m = ((float)(y2 - y1))/((float)(x2 - x1));
        for(uint16_t x = x1; x <= x2; x++) {
            writePixel((uint16_t)((m*(x - x1)) + y1), x, color);
        }
    }
}

static inline void renderLineAA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t scale) {
    return;
}

static inline void renderCircle(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void renderCircleAA(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void fillBetweenPoints(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
    for(uint16_t x = x1; x <= x2; x++) {
        writePixel(y, x, color);
    }
}

static inline void renderRect(RenderQueueItem_t *item, uint8_t aa) {
    if(aa) {
        renderLineAA(item->x1, item->y1, item->x2, item->y1, item->color, item->scale); //top
        renderLineAA(item->x1, item->y2, item->x2, item->y2, item->color, item->scale); //bottom
        renderLineAA(item->x1, item->y1, item->x1, item->y2, item->color, item->scale); //left
        renderLineAA(item->x2, item->y1, item->x2, item->y2, item->color, item->scale); //right
    }
    else {
        renderLine(item->x1, item->y1, item->x2, item->y1, item->color, item->scale); //top
        renderLine(item->x1, item->y2, item->x2, item->y2, item->color, item->scale); //bottom
        renderLine(item->x1, item->y1, item->x1, item->y2, item->color, item->scale); //left
        renderLine(item->x2, item->y1, item->x2, item->y2, item->color, item->scale); //right
    }
}

static inline void renderTriangle(RenderQueueItem_t *item, uint8_t aa) {
    return;
}

static inline void renderFilledRect(RenderQueueItem_t *item, uint8_t aa) {
    return;
}


/*
        Renderer
========================
*/
void render() {
    multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    RenderQueueItem_t *item;
    RenderQueueItem_t *previousItem;

    while(1) {
        if(displayConfig->autoRender) {
            item = (RenderQueueItem_t *) &background;
            //look for the first item that needs an update, render that item and everything after it
            while(!(item->flags & RQI_UPDATE)) {
                previousItem = item;
                item = item->next;
                if(item == NULL) item = (RenderQueueItem_t *) &background;
            }
            if(item->flags & RQI_HIDDEN || item->type == RQI_T_REMOVED) item = (RenderQueueItem_t *) &background; //if the update is to hide or remove an item, rerender the whole thing
        }
        else { //manual rendering
            while(!update); //wait until it's told to update the display
            item = (RenderQueueItem_t *) &background;
        }

        while(item != NULL) {
            if(!(item->flags & RQI_HIDDEN)) {
            switch(item->type) {
                case 'p': //Pixel
                    writePixel(item->y1, item->x1, item->color);
                    break;
                case 'l': //Line
                    
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
                    //x1, y1 = top left corner, x2 = right side (for word wrap), y2 = background color
                    uint16_t x = item->x1;
                    uint16_t y = item->y1;
                    for(uint16_t c = 0; item->obj[c] != '\0'; c++) {
                        for(uint8_t i = 0; i < CHAR_HEIGHT; i++) {
                            for(uint8_t j = 0; j < CHAR_WIDTH; j++) {
                                writePixel(y + i, x + j, getFontBit(item->obj[c], i, j) ? item->color : item->y2);
                            }
                        }
                        x += CHAR_WIDTH + 1;
                        if(item->flags & RQI_WORDWRAP && x + CHAR_WIDTH >= item->x2) {
                            x = item->x1;
                            y += CHAR_HEIGHT;
                        }
                    }
                    break;
                case 's': //Sprites
                    for(uint16_t i = 0; i < item->y2; i++) {
                        for(uint16_t j = 0; j < item->x2; j++) {
                            if(*(item->obj + i*item->x2 + j) != item->color) {
                                writePixel(item->y1 + i, item->x1 + j, *(item->obj + i*item->x2 + j));
                            }
                        }
                    }
                    break;
                case 'f': //Fill the screen
                    for(uint16_t y = 0; y < frameHeight; y++) {
                        for(uint16_t x = 0; x < frameWidth; x++) {
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
            
            item->flags &= ~RQI_UPDATE; //Clear the update bit
            previousItem = item;
            item = item->next;
        }

        update = 0;
    }
}