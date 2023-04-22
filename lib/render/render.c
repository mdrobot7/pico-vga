#include "render.h"

static volatile uint8_t update = 0;
volatile uint8_t interpolatedLine = 0;
volatile uint8_t interpolationIncomplete = 0;

/**
 * @brief Force-refresh the display.
 * 
 */
void updateDisplay() {
    update = 1;
    if(displayConfig->autoRender) ((RenderQueueItem_t *)renderQueueStart)->flags |= RQI_UPDATE; //force-update in autoRender mode
}


/**
 * @brief The renderer! Loops over the render queue waiting for something to update (or to be told
 * to update the framebuffer manually) and then draws it out to the display.
 * 
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
            while(!update);
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

static void interpolate(uint16_t line) {
    return;
}


/*
        Line Interpolation
==================================
*/
volatile uint8_t lineInterpolationIRQ = 0;

static irq_handler_t lineInterpolationHandler() {
    irq_clear(lineInterpolationIRQ);
}

void initLineInterpolation() {
    lineInterpolationIRQ = user_irq_claim_unused(true);
    irq_set_priority(lineInterpolationIRQ, 255); //set to lowest priority, might change later based on testing
    irq_set_exclusive_handler(lineInterpolationIRQ, lineInterpolationHandler);
    irq_set_enabled(31, true);
}

void deInitLineInterpolation() {
    irq_set_enabled(31, false);
    user_irq_unclaim(lineInterpolationIRQ);
}


/*
        Garbage Collector
=================================
*/
static volatile bool garbageCollectorActive = false;
static uint8_t garbageCollectorDMA = 0;
struct repeating_timer garbageCollectorTimer;

static irq_handler_t garbageCollectorHandler() {
    dma_hw->ints1 = 1 << garbageCollectorDMA; //Clear interrupt

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