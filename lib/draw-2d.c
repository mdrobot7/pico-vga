#include "lib-internal.h"
#include "font.h"

#include "pico/malloc.h"

uint32_t drawPixel(int16_t x, int16_t y, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_PIXEL;
    lastItem->uid = uid;
    lastItem->x = x;
    lastItem->y = y;
    lastItem->thetaZ = color;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color, uint8_t thickness) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_LINE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->scaleZ = (int8_t)thickness;

    lastItem->numPointsOrTriangles = 1;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->color = color;
    ((Points_t *)lastItem)->numPoints = 4;
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y2;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_RECTANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->scaleZ = (int8_t)thickness;

    lastItem->numPointsOrTriangles = 1;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->color = color;
    ((Points_t *)lastItem)->numPoints = 8;
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y1;
    ((Points_t *)lastItem)->points[4] = x2;
    ((Points_t *)lastItem)->points[5] = y2;
    ((Points_t *)lastItem)->points[6] = x1;
    ((Points_t *)lastItem)->points[7] = y2;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_TRIANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2 + x3)/3;
    lastItem->y = (y1 + y2 + y3)/3;
    lastItem->scaleZ = (int8_t)thickness;

    lastItem->numPointsOrTriangles = 1;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->color = color;
    ((Points_t *)lastItem)->numPoints = 6;
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y2;
    ((Points_t *)lastItem)->points[4] = x3;
    ((Points_t *)lastItem)->points[5] = y3;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawEllipse(int16_t x, int16_t y, uint16_t radiusX, uint16_t radiusY, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_CIRCLE;
    lastItem->uid = uid;
    lastItem->x = x;
    lastItem->y = y;
    lastItem->z = radiusX; //Can't use scaleX/Y/Z because they are 8 bit signed (not big enough).
    lastItem->numPointsOrTriangles = radiusY;
    lastItem->scaleZ = thickness;
    lastItem->thetaZ = color;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

//Draws lines between all points in the list.
uint32_t drawPolygon(int16_t points[][2], uint8_t numPoints, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_POLYGON;
    lastItem->uid = uid;
    lastItem->scaleZ = (int8_t)thickness;
    lastItem->flags = RQI_UPDATE;

    lastItem->numPointsOrTriangles = numPoints/6;
    if(numPoints % 6 != 0) lastItem->numPointsOrTriangles++;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    /* This convoluted loop copies the data from points[][] into the Points_t structs. Each Points_t holds 12
     * values (6 coordinate pairs in 2D space). This copies 6 points in, then goes to the next Points_t. This
     * is done with pointer math, treating the points[][] array as a flat 1D array, because it's easier.
    */
    for(int i = 0; i < numPoints*2; i++) {
        if(i % 12 == 0) {
            lastItem++;
            clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
            ((Points_t *)lastItem)->color = color;
            if(numPoints - i > 6) ((Points_t *)lastItem)->numPoints = 12;
            else ((Points_t *)lastItem)->numPoints = (numPoints - i)*2;
        }
        ((Points_t *)lastItem)->points[i % 12] = *(points + i);
    }

    lastItem++;
    return uid++;
}

uint32_t drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t borderColor, uint8_t fillColor) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_RECTANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->thetaZ = (int8_t)fillColor;

    lastItem->numPointsOrTriangles = 1;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->color = borderColor;
    ((Points_t *)lastItem)->numPoints = 8;
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y1;
    ((Points_t *)lastItem)->points[4] = x2;
    ((Points_t *)lastItem)->points[5] = y2;
    ((Points_t *)lastItem)->points[6] = x1;
    ((Points_t *)lastItem)->points[7] = y2;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t borderColor, uint8_t fillColor) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_TRIANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2 + x3)/3;
    lastItem->y = (y1 + y2 + y3)/3;
    lastItem->thetaZ = (int8_t)fillColor;

    lastItem->numPointsOrTriangles = 1;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->color = borderColor;
    ((Points_t *)lastItem)->numPoints = 6;
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y2;
    ((Points_t *)lastItem)->points[4] = x3;
    ((Points_t *)lastItem)->points[5] = y3;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

uint32_t drawFilledEllipse(int16_t x, int16_t y, uint16_t radiusX, uint16_t radiusY, uint8_t borderColor, uint8_t fillColor) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_CIRCLE;
    lastItem->uid = uid;
    lastItem->x = x;
    lastItem->y = y;
    lastItem->z = radiusX; //Can't use scaleX/Y/Z because they are 8 bit signed (not big enough).
    lastItem->numPointsOrTriangles = radiusY;
    lastItem->scaleZ = borderColor;
    lastItem->thetaZ = fillColor;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

//Draws lines and fills between all points in the list.
uint32_t fillPolygon(int16_t points[][2], uint8_t numPoints, uint8_t borderColor, uint8_t fillColor) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_POLYGON;
    lastItem->uid = uid;
    lastItem->thetaZ = (int8_t) fillColor;
    lastItem->flags = RQI_UPDATE;

    lastItem->numPointsOrTriangles = numPoints/6;
    if(numPoints % 6 != 0) lastItem->numPointsOrTriangles++;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    /* This convoluted loop copies the data from points[][] into the Points_t structs. Each Points_t holds 12
     * values (6 coordinate pairs in 2D space). This copies 6 points in, then goes to the next Points_t. This
     * is done with pointer math, treating the points[][] array as a flat 1D array, because it's easier.
    */
    for(int i = 0; i < numPoints*2; i++) {
        if(i % 12 == 0) {
            lastItem++;
            clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
            ((Points_t *)lastItem)->color = borderColor;
            if(numPoints - i > 6) ((Points_t *)lastItem)->numPoints = 12;
            else ((Points_t *)lastItem)->numPoints = (numPoints - i)*2;
        }
        ((Points_t *)lastItem)->points[i % 12] = *(points + i);
    }

    lastItem++;
    return uid++;
}

uint32_t fillScreen(uint8_t * obj, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILL;
    lastItem->uid = uid;
    lastItem->thetaZ = color;
    lastItem->pointsOrTriangles = (void *)obj;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

//Removes everything from the list, marks all elements for deletion (handled in garbage collector)
void clearScreen() {
    RenderQueueItem_t * item = (RenderQueueItem_t *)buffer;
    while(item < lastItem) {
        item->type = RQI_T_REMOVED;
        item += item->numPointsOrTriangles + 1; //get past the Points_t or Triangle_t structs packed next to the item
    }

    lastItem = (RenderQueueItem_t *)buffer;
    uid = 1;
}


/*
        Text Functions
==============================
*/
uint8_t *font = (uint8_t *)cp437; //The current font in use by the system

uint32_t drawText(uint16_t x1, uint16_t y, uint16_t x2, char *str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverrideBytes) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_STRING;
    lastItem->uid = uid;
    if(wrap) {
        lastItem->x = (x1 + x2)/2;
        lastItem->y = (y + (CHAR_HEIGHT*(((x2 - x1)/(CHAR_WIDTH + 1)) + 1)))/2;
    }
    else {
        lastItem->x = (x1 + x2)/2;
        lastItem->y = y;
    }
    lastItem->scaleZ = (int8_t)bgColor;
    lastItem->thetaZ = (int8_t)color;

    if(strSizeOverrideBytes != 0) {
        if(strSizeOverrideBytes % sizeof(Points_t) == 0) {
            lastItem->numPointsOrTriangles = strSizeOverrideBytes/sizeof(Points_t);
        }
        else lastItem->numPointsOrTriangles = (strSizeOverrideBytes/sizeof(Points_t)) + 1;
    }
    else {
        if(strlen(str) % sizeof(Points_t) == 0) {
            lastItem->numPointsOrTriangles = strlen(str)/sizeof(Points_t);
        }
        else lastItem->numPointsOrTriangles = (strlen(str)/sizeof(Points_t)) + 1;
    }
    uint16_t tempNumPointsOrTriangles = lastItem->numPointsOrTriangles;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to the memory location right after the RenderQueueItem.
    strcpy((char *)lastItem, str);

    (lastItem - 1)->flags = RQI_UPDATE;
    if(wrap) (lastItem - 1)->flags |= RQI_WORDWRAP;
    lastItem += tempNumPointsOrTriangles + 1;
    return uid++;
}

void setTextFont(uint8_t *newFont) {
    font = newFont;
}


/*
        Sprite Drawing Functions
========================================
*/
uint32_t drawSprite(uint8_t * sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, int8_t scaleX, int8_t scaleY) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_SPRITE;
    lastItem->uid = uid;
    lastItem->x = (x + (x + dimX))/2;
    lastItem->y = (y + (y + dimY))/2;
    lastItem->scaleX = scaleX;
    lastItem->scaleY = scaleY;
    lastItem->thetaZ = nullColor;
    lastItem->z = dimX;                    //Using the z and numPoints... field because there are no other uint16_t fields available.
    lastItem->numPointsOrTriangles = dimY;

    lastItem->pointsOrTriangles = sprite;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}


/*
        Render Queue Modifiers
======================================
*/
//Set item to be hidden (true = hidden, false = showing)
void setHidden(uint32_t itemUID, uint8_t hidden) {
    RenderQueueItem_t * item = findRenderQueueItem(itemUID);
    if(item->flags & RQI_HIDDEN) item->flags &= ~RQI_HIDDEN;
    else item->flags |= RQI_HIDDEN;

    updateFullDisplay();
}

//Permanently remove item from the render queue.
void removeItem(uint32_t itemUID) {
    RenderQueueItem_t * item = findRenderQueueItem(itemUID);
    item->type = RQI_T_REMOVED;
    updateFullDisplay();
}