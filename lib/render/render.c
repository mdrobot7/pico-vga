#include "render.h"

static volatile uint8_t update = 0;
volatile uint8_t interpolatedLine = 0;
volatile uint8_t startInterpolation = false;
volatile uint8_t interpolationIncomplete = 0;

void updateDisplay() {
    update = 1;
    if(displayConfig->autoRender) ((RenderQueueItem_t *)renderQueueStart)->flags |= RQI_UPDATE; //force-update in autoRender mode
}

/*
        Render Utilities
================================
*/
//Checks for out-of-bounds coordinates, does pointer math, and writes to the frame
static inline void writePixel(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= frameWidth || y >= frameHeight) return;
    //calculating the row/y off of frameReadAddr handles interpolated lines.
    //This grabs the y'th element of frameReadAddr, adds x to it, dereferences, and writes the color to it
    else *((uint8_t *)(*((uint8_t **)frameReadAddrStart + y)) + x) = color;
}

static inline uint8_t getFontBit(uint8_t c, uint8_t x, uint8_t y) {
    return *(font + CHAR_HEIGHT*c + y) >> (CHAR_WIDTH - x + 1) & 1u;
}

/*
        Render Functions
================================
*/
static inline void bresenhamLow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    //Bresenham's line drawing algorithm, thanks Wikipedia! (https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;

    int32_t yi = 1;
    if(dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int32_t D = (2*dy) - dx;
    int32_t y = y1;

    for(int32_t x = x1; x < x2; x++) {
        writePixel(y, x, color);
        if(D > 0) {
            y = y + yi;
            D = D + 2*(dy - dx);
        }
        else {
            D = D + 2*dy;
        }
    }
}

static inline void bresenhamHigh(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    
    int32_t xi = 1;
    if(dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int32_t D = (2*dx) - dy;
    int32_t x = x1;

    for(int32_t y = y1; y < y2; y++) {
        writePixel(y, x, color);
        if(D > 0) {
            x = x + xi;
            D = D + 2*(dx - dy);
        }
        else {
            D = D + 2*dx;
        }
    }
}

static inline void renderFastVertLine(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color) {
    for(; y1 < y2; y1++) {
        writePixel(y1, x, color);
    }
}

static inline void renderFastHorizLine(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
    for(; x1 < x2; x1++) {
        writePixel(y, x1, color);
    }
}

static inline void renderLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    if(x1 == x2) {
        renderFastVertLine(x1, y1, y2, color);
    }
    else if (y1 == y2) {
        renderFastHorizLine(x1, x2, y1, color);
    }
    else {
        if(y2 - y1 < x2 - x1) {
            if(x1 > x2) {
                //if(thickness != 0); //bresenhamLowThick(x2, y2, x1, y1, color, thickness) <- implement later
                bresenhamLow(x2, y2, x1, y1, color); //Coordinate pairs reversed
            }
            else {
                bresenhamLow(x1, y1, x2, y2, color);
            }
        }
        else {
            if(y1 > y2) {
                bresenhamHigh(x2, y2, x1, y1, color); //Coordinate pairs reversed
            }
            else {
                bresenhamHigh(x1, y1, x2, y2, color);
            }
        }
    }
}

static inline void renderLineAA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    return;
}

static inline void renderTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t thickness) {
    renderLine(x1, y1, x2, y2, color, thickness);
    renderLine(x2, y2, x3, y3, color, thickness);
    renderLine(x1, y1, x3, y3, color, thickness);
}

static inline void renderFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    //Bresenham triangle algorithm, thanks to http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
    if(y1 == y2 && y2 == y3) { //weird case where all points are on the same line
        renderFastHorizLine(min(x1, x2, x3), max(x1, x2, x3), y1, color);
    }

    // Sort coordinates by Y order (y3 >= y2 >= y1)
    if (y1 > y2) {
        uint16_t ty = y1; //swap y1, y2
        y1 = y2;
        y2 = ty;

        uint16_t tx = x1;
        x1 = x2;
        x2 = tx;
    }
    if (y2 > y3) {
        uint16_t ty = y2; //swap y2, y3
        y2 = y3;
        y3 = ty;

        uint16_t tx = x2;
        x2 = x3;
        x3 = tx;
    }
    if (y1 > y2) {
        uint16_t ty = y1; //swap y1, y2
        y1 = y2;
        y2 = ty;

        uint16_t tx = x1;
        x1 = x2;
        x2 = tx;
    }

    if(y1 == y2) {} //flat-topped triangle
    else if(y2 == y3) {} //flat-bottomed triangle
    else { //all other triangles
        uint16_t ySplit = y2; //the split between the two flat-sided triangles
    }
}

static inline void renderCircle(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void renderCircleAA(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void renderPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
    //Split up for performance
    if(displayConfig->antiAliasing) {
        for(int i = 0; i < numPoints - 2; i += 2) {
            renderLineAA(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
        }
    }
    else {
        for(int i = 0; i < numPoints - 2; i += 2) {
            renderLine(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
        }
    }
}

//If you have any shape that is convex on the top or bottom, the stack overflow polygon fill algorithm isn't
//gonna work (like an upside down U shape or something). It doesn't handle multiple shading sections at all.

static void renderFillPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
    uint16_t minX = 0xFFFF, minY = 0xFFFF, maxX = 0, maxY = 0;
    for(int i = 0; i < numPoints; i += 2) {
        if(points[i] < minX) minX = points[i];
        if(points[i] > maxX) maxX = points[i];
        if(points[i + 1] < minY) minY = points[i + 1];
        if(points[i + 1] < minX) minX = points[i + 1];
    }

    uint16_t x0Buf[maxY - minY];
    uint16_t x1Buf[maxY - minY];

    //TODO

    for(int i = 0; i < maxY - minY; i++) {
        renderFastHorizLine(x0Buf[i], x1Buf[i], minY + i, color);
    }
}


/*
        Renderer
========================
*/
void render() {
    multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    RenderQueueItem_t *item;

    while(1) {
        if(displayConfig->autoRender) {
            item = (RenderQueueItem_t *) renderQueueStart;
            //look for the first item that needs an update, render that item and everything after it
            while(!(item->flags & RQI_UPDATE)) {
                if(item >= lastItem) item = (RenderQueueItem_t *) renderQueueStart;
            }
            //if the update is to hide or remove an item, rerender the whole thing
            if(item->flags & RQI_HIDDEN || item->uid == RQI_UID_REMOVED) item = (RenderQueueItem_t *) renderQueueStart;
        }
        else { //manual rendering
            while(!update); //wait until it's told to update the display
            item = (RenderQueueItem_t *) renderQueueStart;
        }

        while(item <= lastItem) {
            if(!(item->flags & RQI_HIDDEN) && item->uid == RQI_UID_REMOVED) {
            switch(item->type) {
                case RQI_T_PIXEL:
                    writePixel(item->y, item->x, item->thetaZ);
                    break;
                case RQI_T_LINE:
                    break;
                case RQI_T_RECTANGLE:
                    break;
                case RQI_T_TRIANGLE:
                    break;
                case RQI_T_CIRCLE:
                    //x1, y1 = center, z = radius in x direction
                    for(uint16_t y = item->y - item->z; y <= item->y + item->z; y++) {
                        uint16_t x = sqrt(pow(item->z, 2.0) - pow(y - item->y, 2.0));
                        writePixel(y, item->x + x, item->thetaZ); //handle the +/- from the sqrt (the 2 sides of the circle)
                        writePixel(y, item->x - x, item->thetaZ);
                    }
                    break;
                case RQI_T_FILLED_RECTANGLE:
                    break;
                case RQI_T_FILLED_TRIANGLE:
                    break;
                case RQI_T_FILLED_CIRCLE:
                    //x1, y1 = center, z = radius in x direction
                    for(uint16_t y = item->y - item->z; y <= item->y + item->z; y++) {
                        uint16_t x = sqrt(pow(item->z, 2.0) - pow(y - item->y, 2.0));
                        for(uint16_t i = item->x - x; i <= item->x + x; i++) {
                            writePixel(y, i, item->thetaZ);
                        }
                    }
                    break;
                case RQI_T_STRING:
                    /*//x1, y = top left corner, x2 = right side (for word wrap), thetaZ = color, scaleZ = background
                    uint16_t x1 = ((Points_t *)lastItem->pointsOrTriangles)->points[0]; //Fetch points from the memory space right after item
                    uint16_t y = ((Points_t *)lastItem->pointsOrTriangles)->points[1];
                    uint16_t x2 = ((Points_t *)lastItem->pointsOrTriangles)->points[2];
                    char * str = &(((Points_t *)lastItem->pointsOrTriangles)->points[3]);

                    for(uint16_t c = 0; str[c] != '\0'; c++) {
                        for(uint8_t i = 0; i < CHAR_HEIGHT; i++) {
                            for(uint8_t j = 0; j < CHAR_WIDTH; j++) {
                                writePixel(y + i, x1 + j, getFontBit(str[c], i, j) ? item->thetaZ : item->scaleZ);
                            }
                        }
                        x += CHAR_WIDTH + 1;
                        if(item->flags & RQI_WORDWRAP && x + CHAR_WIDTH >= x2) {
                            x = item->x;
                            y += CHAR_HEIGHT;
                        }
                    }*/
                    break;
                case RQI_T_SPRITE:
                    /*for(uint16_t i = 0; i < item->y2; i++) {
                        for(uint16_t j = 0; j < item->x2; j++) {
                            if(*(item->obj + i*item->x2 + j) != item->color) {
                                writePixel(item->y1 + i, item->x1 + j, *(item->obj + i*item->x2 + j));
                            }
                        }
                    }*/
                    break;
                case RQI_T_FILL:
                    for(uint16_t y = 0; y < frameHeight; y++) {
                        for(uint16_t x = 0; x < frameWidth; x++) {
                            writePixel(y, x, item->thetaZ);
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
            }
            }
            
            item->flags &= ~RQI_UPDATE; //Clear the update bit
            item++;
        }

        update = 0;
    }
}


/*
        Garbage Collector
=================================
*/
static volatile bool garbageCollectorActive = false;
static int8_t garbageCollectorDMA = 0;
struct repeating_timer garbageCollectorTimer;

static void garbageCollectorHandler() {
    RenderQueueItem_t * item = findRenderQueueItem(RQI_UID_REMOVED);
    if(item >= lastItem) {
        garbageCollectorActive = false;
        return; //No garbage collecting needs to be done
    }

    dma_hw->ch[garbageCollectorDMA].read_addr = (io_rw_32)(item + item->numPointsOrTriangles + 1);
    dma_hw->ch[garbageCollectorDMA].write_addr = (io_rw_32)item;
    dma_hw->ch[garbageCollectorDMA].al1_transfer_count_trig = ((uint32_t)lastItem - (uint32_t)(dma_hw->ch[garbageCollectorDMA].read_addr))/4;
}

bool garbageCollector(struct repeating_timer *t) {
    if(garbageCollectorActive) return true; //Don't start it again if it hasn't finished already
    garbageCollectorActive = true;
    garbageCollectorHandler();
    return true;
}

void initGarbageCollector() {
    garbageCollectorDMA = dma_claim_unused_channel(true);
    dma_hw->ch[garbageCollectorDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN) | (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) |
                                               (1 << SDK_DMA_CTRL_INCR_READ) | (1 << SDK_DMA_CTRL_INCR_WRITE) |
                                               (garbageCollectorDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL);
    dma_hw->inte1 = (1 << garbageCollectorDMA);

    irq_set_exclusive_handler(DMA_IRQ_1, garbageCollectorHandler);
    irq_set_enabled(DMA_IRQ_1, true);

    //Set the garbage collector to run every n ms
    add_repeating_timer_ms(5, garbageCollector, NULL, &garbageCollectorTimer);
}

void deInitGarbageCollector() {
    cancel_repeating_timer(&garbageCollectorTimer);

    dma_hw->abort = (1 << garbageCollectorDMA);
    while(dma_hw->abort); //wait until stopped

    dma_hw->ch[garbageCollectorDMA].al1_ctrl = 0;
    dma_channel_unclaim(garbageCollectorDMA);
    irq_set_enabled(DMA_IRQ_1, false);
}