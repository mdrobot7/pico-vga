#include "lib-internal.h"

void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

//Dot product of a 1x4 row vector (v1) and a 4x1 column vector (v2)
int16_t dotProduct(int16_t * v1, int16_t * v2) {
    return (v1[0]*v2[0])>>(2*NUM_FRAC_BITS) + (v1[1]*v2[4])>>(2*NUM_FRAC_BITS) + 
           (v1[2]*v2[8])>>(2*NUM_FRAC_BITS) + (v1[3]*v2[12])>>(2*NUM_FRAC_BITS);
}

//Multiply two 4x4 matrices (used for affine transformations in R3) and store in 4x4 matrix res
void multiplyMatrices(int16_t m1[4][4], int16_t m2[4][4], int16_t res[4][4]) {
    for(uint8_t i = 0; i < 4; i++) {
        for(uint8_t j = 0; j < 4; j++) {
            res[i][j] = dotProduct(&m1[i][0], &m2[0][j]);
        }
    }
}

//Returns the number of bytes available in the render queue before hitting another block of data.
int32_t renderQueueNumBytesFree() {
    uint8_t * ptr = frameReadAddrStart;
    if(sdStart != NULL && (uint8_t *)lastItem <= sdStart) ptr = sdStart;
    else if(audioStart != NULL && (uint8_t *)lastItem <= audioStart) ptr = audioStart;
    else if(controllerStart != NULL && (uint8_t *)lastItem <= controllerStart) ptr = controllerStart;

    return ptr - (uint8_t *)lastItem;
}

void clearRenderQueueItemData(RenderQueueItem_t * item) {
    for(int i = 0; i < sizeof(RenderQueueItem_t)/4; i++) {
        *((uint32_t *)item + i) = 0; //Treat the RenderQueueItem as bare memory, write zeros to it in 32bit chunks
    }
}

RenderQueueItem_t * findRenderQueueItem(uint32_t itemUID) {
    for(RenderQueueItem_t * i = (RenderQueueItem_t *)renderQueueStart; i < lastItem; i++) {
        if(i->uid == itemUID) return i;
        i += i->numPointsOrTriangles; //skip over the Points_t or Triangle_t structs
    }
}

uint16_t min(uint16_t a, uint16_t b, uint16_t c) {
    if(a < b && a < c) return a;
    if(b < a && b < c) return b;
    else return c;
}

uint16_t max(uint16_t a, uint16_t b, uint16_t c) {
    if(a > b && a > c) return a;
    if(b > a && b > c) return b;
    else return c;
}