//The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef LIB_INTERNAL_H
#define LIB_INTERNAL_H

#include "pico-vga.h"
#include "pinout.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "color.pio.h"
#include "vsync_640x480.pio.h"
#include "hsync_640x480.pio.h"
#include "vsync_800x600.pio.h"
#include "hsync_800x600.pio.h"
#include "vsync_1024x768.pio.h"
#include "hsync_1024x768.pio.h"

//Unified Pico-VGA memory buffer
extern uint8_t buffer[PICO_VGA_MAX_MEMORY_BYTES];

//Constants for the width and height of base resolutions
extern const uint16_t frameSize[3][4];

/* Pointers to each block in the Pico VGA unified memory buffer. Written in order of how it's organized
 * (render queue first, at buffer[0]). End pointers are where the next element of the render queue, for example,
 * will be added, *not* where the block ends (it ends at the start of the next one).
*/
extern uint8_t * renderQueueStart;
extern uint8_t * renderQueueEnd;
extern uint8_t * sdStart;
extern uint8_t * sdEnd;
extern uint8_t * audioStart;
extern uint8_t * audioEnd;
extern uint8_t * controllerStart;
extern uint8_t * controllerEnd;
extern uint8_t * frameReadAddrStart; //Pointer to an array of pointers, each pointing to the start of a line in the frame buffer OR an interpolated line in line (size = full height of the BASE resolution).
extern uint8_t * frameReadAddrEnd;
extern uint8_t * blankLineStart; //Pointer to an array of zeros the size of a line.
extern uint8_t * interpolatedLineStart; //Pointer to the interpolated lines (a 2d array, displayConfig->numInterpolatedLines*frameWidth)
extern uint8_t * frameBufferStart; //Pointer to the frame buffer (a 2d array, frameHeight*frameWidth)
extern uint8_t * frameBufferEnd;

extern uint8_t frameCtrlDMA;
extern uint8_t frameDataDMA;
extern uint8_t blankDataDMA;
extern uint8_t controllerDataDMA;

//Configuration Options
extern DisplayConfig_t * displayConfig;
extern ControllerConfig_t * controllerConfig;
extern AudioConfig_t * audioConfig;
extern SDConfig_t * sdConfig;
extern USBHostConfig_t * usbConfig;

int initDisplay();
int initController();
int initAudio();
int initSD();
int initUSB();
int initPeripheralMode();

int deInitDisplay();
int deInitController();
int deInitAudio();
int deInitSD();
int deInitUSB();
int deInitPeripheralMode();

/*
        Render Queue
============================
*/
//Number of bits in some fields below that make up the "fractional" part of the fixed-point field
#define NUM_FRAC_BITS 6

typedef struct __packed {
    //Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
    //individually instead of only by clearing the screen.
    uint32_t uid;

    //The type of object this RenderQueueItem represents.
    RenderQueueItemType_t type;

    //Bit register for configuration options.
    uint8_t flags;

    //Center point (reference point for the entire object, center of rotation)
    //Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
    //the least significant 6 bits being the fractional part (one bit left because it's signed).
    int16_t x, y, z;

    //Rotation in x, y, z -- negative value means negative rotation
    int8_t thetaX, thetaY, thetaZ;

    //Scaling/stretching in the x, y, and z directions
    int8_t scaleX, scaleY, scaleZ;
    
    //Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
    //mesh for 3D rendering (Either Points_t or Triangle_t)
    void * pointsOrTriangles;
    uint16_t numPointsOrTriangles;

    //Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
    void (* animate)(RenderQueueItem_t *);
} RenderQueueItem_t;

//Double the size of a normal RenderQueueItem_t. The extra space goes to an extended points[] buffer, which
//allows this RenderQueueItem to be drawn in multiple places around the screen.
typedef struct __packed {
    //Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
    //individually instead of only by clearing the screen.
    uint32_t uid;

    //The type of object this RenderQueueItem represents.
    RenderQueueItemType_t type;

    //Bit register for configuration options.
    uint8_t flags;

    //Center point (reference point for the entire object, center of rotation)
    //Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
    //the least significant 6 bits being the fractional part (one bit left because it's signed).
    int16_t points[16];
    uint16_t numPoints;

    //Rotation in x, y, z -- negative value means negative rotation
    int8_t thetaX, thetaY, thetaZ;

    //Scaling/stretching in the x, y, and z directions
    int8_t scaleX, scaleY, scaleZ;
    
    //Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
    //mesh for 3D rendering (Either Points_t or Triangle_t)
    void * pointsOrTriangles;
    uint16_t numPointsOrTriangles;

    //Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
    void (* animate)(RenderQueueItem_t *);
} RenderQueueItemExt_t;

//Double-extended RenderQueueItem, with even more space allocated to the points[] buffer (4x the size of normal)
typedef struct __packed {
    //Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
    //individually instead of only by clearing the screen.
    uint32_t uid;

    //The type of object this RenderQueueItem represents.
    RenderQueueItemType_t type;

    //Bit register for configuration options.
    uint8_t flags;

    //Center point (reference point for the entire object, center of rotation)
    //Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
    //the least significant 6 bits being the fractional part (one bit left because it's signed).
    int16_t points[44];
    uint16_t numPoints;

    //Rotation in x, y, z -- negative value means negative rotation
    int8_t thetaX, thetaY, thetaZ;

    //Scaling/stretching in the x, y, and z directions
    int8_t scaleX, scaleY, scaleZ;
    
    //Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
    //mesh for 3D rendering (Either Points_t or Triangle_t)
    void * pointsOrTriangles;
    uint16_t numPointsOrTriangles;

    //Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
    void (* animate)(RenderQueueItem_t *);
} RenderQueueItemDblExt_t;

//The type of a particular render queue item (RenderQueueItem.type)
typedef enum {
    RQI_T_REMOVED,
    RQI_T_FILL,
    RQI_T_PIXEL,
    RQI_T_LINE,
    RQI_T_RECTANGLE,
    RQI_T_FILLED_RECTANGLE,
    RQI_T_TRIANGLE,
    RQI_T_FILLED_TRIANGLE,
    RQI_T_CIRCLE,
    RQI_T_FILLED_CIRCLE,
    RQI_T_STRING,
    RQI_T_SPRITE,
    RQI_T_BITMAP,
    RQI_T_POLYGON, 
    RQI_T_FILLED_POLYGON,
    RQI_T_LIGHT,
    RQI_T_SVG,
    RQI_T_EXTENDED = 64,
    RQI_T_DBL_EXTENDED = 128;
    RQI_T_MAX = 255, //Ensure that a RenderQueueItemType_t variable is 8 bits
} RenderQueueItemType_t;

//RenderQueueItem.flags macros
#define RQI_UPDATE   (1u << 0)
#define RQI_HIDDEN   (1u << 1)
#define RQI_WORDWRAP (1u << 2)

//A list of points in 2D space, used for saving the vertices of RenderQueueItems. Same size as RenderQueueItem_t
//and Triangle_t.
typedef struct __packed {
    int16_t points[12];
    uint8_t numPoints;
    uint8_t color;
    uint8_t hidden;
    uint8_t __SPACER;
} Points_t;

//A single triangle, used for polygon meshes in 3D rendering. Same size as RenderQueueItem_t and Points_t.
typedef struct __packed {
    //Coordinates of the triangle's vertices.
    int16_t x1, y1, z1;
    int16_t x2, y2, z2;
    int16_t x3, y3, z3;

    //Coordinates of the normal vector to the triangle (indicates which side is "up", where to apply textures).
    int16_t normX1, normX2, normX3;

    //The color of this triangle.
    uint8_t color;

    //Set to true to not shade this triangle.
    uint8_t hidden;

    //2 bytes of blank space so RenderQueueItem_t and Triangle_t are the same size (28 bytes)
    uint16_t __SPACER;
} Triangle_t;

extern RenderQueueItem_t * lastItem;
extern uint32_t uid;

void render();

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8
extern uint8_t *font; //The current font in use by the system

/*
        Utilities
=========================
*/
inline void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

//Dot product of a 1x4 row vector (v1) and a 4x1 column vector (v2)
inline int16_t dotProduct(int16_t * v1, int16_t * v2) {
    return (v1[0]*v2[0])>>(2*NUM_FRAC_BITS) + (v1[1]*v2[4])>>(2*NUM_FRAC_BITS) + 
           (v1[2]*v2[8])>>(2*NUM_FRAC_BITS) + (v1[3]*v2[12])>>(2*NUM_FRAC_BITS);
}

//Multiply two 4x4 matrices (used for affine transformations in R3) and store in 4x4 matrix res
inline void multiplyMatrices(int16_t m1[4][4], int16_t m2[4][4], int16_t res[4][4]) {
    for(uint8_t i = 0; i < 4; i++) {
        for(uint8_t j = 0; j < 4; j++) {
            res[i][j] = dotProduct(&m1[i][0], &m2[0][j]);
        }
    }
}

//Returns the number of bytes available in the render queue before hitting another block of data.
inline int32_t renderQueueNumBytesFree() {
    uint8_t * ptr = frameReadAddrStart;
    if(sdStart != NULL && (uint8_t *)lastItem <= sdStart) ptr = sdStart;
    else if(audioStart != NULL && (uint8_t *)lastItem <= audioStart) ptr = audioStart;
    else if(controllerStart != NULL && (uint8_t *)lastItem <= controllerStart) ptr = controllerStart;

    return ptr - (uint8_t *)lastItem;
}

inline void clearRenderQueueItemData(RenderQueueItem_t * item) {
    for(int i = 0; i < sizeof(RenderQueueItem_t)/4; i++) {
        *((uint32_t *)item + i) = 0; //Treat the RenderQueueItem as bare memory, write zeros to it in 32bit chunks
    }
}

inline RenderQueueItem_t * findRenderQueueItem(uint32_t itemUID) {
    for(RenderQueueItem_t * i = renderQueueStart; i < lastItem; i++) {
        if(i->uid == itemUID) return i;
        i += i->numPointsOrTriangles; //skip over the Points_t or Triangle_t structs
    }
}

#endif