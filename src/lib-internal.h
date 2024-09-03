// The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef LIB_INTERNAL_H
#define LIB_INTERNAL_H

#include "pico-vga.h"
#include "pinout.h"

// Constants for the width and height of base resolutions
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
extern uint8_t * frameReadAddrStart; // Pointer to an array of pointers, each pointing to the start of a line in the frame buffer OR an interpolated line in line (size = full height of the BASE resolution).
extern uint8_t * frameReadAddrEnd;
extern uint8_t * blankLineStart;        // Pointer to an array of zeros the size of a line.
extern uint8_t * interpolatedLineStart; // Pointer to the interpolated lines (a 2d array, displayConfig->numInterpolatedLines*frameWidth)
extern uint8_t * frameBufferStart;      // Pointer to the frame buffer (a 2d array, frameHeight*frameWidth)
extern uint8_t * frameBufferEnd;

extern uint8_t frameCtrlDMA;
extern uint8_t frameDataDMA;
extern uint8_t blankDataDMA;

extern struct repeating_timer garbageCollectorTimer;

extern volatile uint8_t interpolatedLine;        // The interpolated line currently being written to
extern volatile uint8_t interpolationIncomplete; // Number of uncompleted interpolated lines (not enough time to finish before needing to be pushed)
extern volatile uint8_t lineInterpolationIRQ;

/*
        Render Queue
============================
*/
// Number of bits in some fields below that make up the "fractional" part of the fixed-point field
#define NUM_FRAC_BITS 6

// The type of a particular render queue item (RenderQueueItem.type)
typedef enum {
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
  RQI_T_EXTENDED     = 64,
  RQI_T_DBL_EXTENDED = 128,
  RQI_T_MAX          = 255, // Ensure that a RenderQueueItemType_t variable is 8 bits
} RenderQueueItemType_t;

#define RQI_UID_REMOVED 0

struct __packed RenderQueueItem_t; // Predefinition, takes care of warnings later on from the function pointers
typedef struct __packed {
  // Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
  // individually instead of only by clearing the screen.
  RenderQueueUID_t uid;

  // The type of object this RenderQueueItem represents.
  RenderQueueItemType_t type;

  // Bit register for configuration options.
  uint8_t flags;

  // Center point (reference point for the entire object, center of rotation)
  // Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
  // the least significant 6 bits being the fractional part (one bit left because it's signed).
  int16_t x, y, z;

  // Rotation in x, y, z -- negative value means negative rotation
  int8_t thetaX, thetaY, thetaZ;

  // Scaling/stretching in the x, y, and z directions
  int8_t scaleX, scaleY, scaleZ;

  // Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
  // mesh for 3D rendering (Either Points_t or Triangle_t)
  uint16_t numPointsOrTriangles;
  void * pointsOrTriangles;

  // Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
  void (*animate)(struct RenderQueueItem_t *);
} RenderQueueItem_t;

// Currently Unused
// Double the size of a normal RenderQueueItem_t. The extra space goes to an extended points[] buffer, which
// allows this RenderQueueItem to be drawn in multiple places around the screen.
typedef struct __packed {
  // Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
  // individually instead of only by clearing the screen.
  RenderQueueUID_t uid;

  // The type of object this RenderQueueItem represents.
  RenderQueueItemType_t type;

  // Bit register for configuration options.
  uint8_t flags;

  // Center point (reference point for the entire object, center of rotation)
  // Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
  // the least significant 6 bits being the fractional part (one bit left because it's signed).
  int16_t points[16];
  uint16_t numPoints;

  // Rotation in x, y, z -- negative value means negative rotation
  int8_t thetaX, thetaY, thetaZ;

  // Scaling/stretching in the x, y, and z directions
  int8_t scaleX, scaleY, scaleZ;

  // Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
  // mesh for 3D rendering (Either Points_t or Triangle_t)
  uint16_t numPointsOrTriangles;
  void * pointsOrTriangles;

  // Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
  void (*animate)(RenderQueueItem_t *);
} RenderQueueItemExt_t;

// Currently Unused
// Double-extended RenderQueueItem, with even more space allocated to the points[] buffer (4x the size of normal)
typedef struct __packed {
  // Unique ID for this RenderQueueItem, so the user can modify it after initialization and remove
  // individually instead of only by clearing the screen.
  RenderQueueUID_t uid;

  // The type of object this RenderQueueItem represents.
  RenderQueueItemType_t type;

  // Bit register for configuration options.
  uint8_t flags;

  // Center point (reference point for the entire object, center of rotation)
  // Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
  // the least significant 6 bits being the fractional part (one bit left because it's signed).
  int16_t points[44];
  uint16_t numPoints;

  // Rotation in x, y, z -- negative value means negative rotation
  int8_t thetaX, thetaY, thetaZ;

  // Scaling/stretching in the x, y, and z directions
  int8_t scaleX, scaleY, scaleZ;

  // Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
  // mesh for 3D rendering (Either Points_t or Triangle_t)
  uint16_t numPointsOrTriangles;
  void * pointsOrTriangles;

  // Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
  void (*animate)(RenderQueueItem_t *);
} RenderQueueItemDblExt_t;

// RenderQueueItem.flags macros
#define RQI_UPDATE   (1u << 0)
#define RQI_HIDDEN   (1u << 1)
#define RQI_WORDWRAP (1u << 2)

// A list of points in 2D space, used for saving the vertices of RenderQueueItems. Same size as RenderQueueItem_t
// and Triangle_t.
typedef struct __packed {
  uint16_t points[14];
} Points_t;

// A single triangle, used for polygon meshes in 3D rendering. Same size as RenderQueueItem_t and Points_t.
typedef struct __packed {
  // Coordinates of the triangle's vertices.
  int16_t x1, y1, z1;
  int16_t x2, y2, z2;
  int16_t x3, y3, z3;

  // Coordinates of the normal vector to the triangle (indicates which side is "up", where to apply textures).
  int16_t normX, normY, normZ;

  // The color of this triangle.
  uint8_t color;

  // Set to true to not shade this triangle.
  uint8_t hidden;

  // 2 bytes of blank space so RenderQueueItem_t and Triangle_t are the same size (28 bytes)
  uint16_t __SPACER;
} Triangle_t;

extern RenderQueueItem_t * lastItem;
extern RenderQueueUID_t uid;

void render();
void renderAA();


/*
        Utilities
=========================
*/
void busyWait(uint64_t n);
int16_t dotProduct(int16_t * v1, int16_t * v2);
void multiplyMatrices(int16_t m1[4][4], int16_t m2[4][4], int16_t res[4][4]);
int32_t renderQueueNumBytesFree();
void clearRenderQueueItemData(RenderQueueItem_t * item);
RenderQueueItem_t * findRenderQueueItem(RenderQueueUID_t itemUID);
uint16_t min(uint16_t a, uint16_t b, uint16_t c);
uint16_t max(uint16_t a, uint16_t b, uint16_t c);

#endif