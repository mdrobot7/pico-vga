#include "lib-internal.h"
#include "font.h"

RenderQueueUID_t drawPixel(uint16_t x, uint16_t y, uint8_t color) {
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

RenderQueueUID_t drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_LINE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->scaleZ = (int8_t)thickness;
    lastItem->thetaZ = (int8_t)color;

    lastItem->numPointsOrTriangles = 4;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y1;
    ((Points_t *)lastItem)->points[2] = x2;
    ((Points_t *)lastItem)->points[3] = y2;

    (lastItem - 1)->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

RenderQueueUID_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_RECTANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->scaleZ = (int8_t)thickness;
    lastItem->thetaZ = (int8_t)color;

    lastItem->numPointsOrTriangles = 8;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
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

RenderQueueUID_t drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_TRIANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2 + x3)/3;
    lastItem->y = (y1 + y2 + y3)/3;
    lastItem->scaleZ = (int8_t)thickness;
    lastItem->thetaZ = (int8_t)color;

    lastItem->numPointsOrTriangles = 6;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
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

RenderQueueUID_t drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_CIRCLE;
    lastItem->uid = uid;
    lastItem->x = x;
    lastItem->y = y;
    lastItem->z = radius; //Can't use scaleX/Y/Z because they are 8 bit signed (not big enough).
    lastItem->scaleZ = thickness;
    lastItem->thetaZ = color;

    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

//Draws lines between all points in the list. Points must be in clockwise order.
RenderQueueUID_t drawPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t thickness, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_POLYGON;
    lastItem->uid = uid;
    lastItem->scaleZ = (int8_t)thickness;
    lastItem->thetaZ = (int8_t)color;
    lastItem->flags = RQI_UPDATE;

    lastItem->numPointsOrTriangles = numPoints;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    /* This convoluted loop copies the data from points[][] into the Points_t structs. Each Points_t holds 12
     * values (6 coordinate pairs in 2D space). This copies 6 points in, then goes to the next Points_t. This
     * is done with pointer math, treating the points[][] array as a flat 1D array, because it's easier.
    */
    for(int i = 0; i < numPoints*2; i++) {
        if(i % 12 == 0) {
            lastItem++;
            clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
        }
        ((Points_t *)lastItem)->points[i % 12] = *((uint16_t *)points + i);
    }

    lastItem++;
    return uid++;
}

RenderQueueUID_t drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_RECTANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2)/2;
    lastItem->y = (y1 + y2)/2;
    lastItem->thetaZ = (int8_t)color;

    lastItem->numPointsOrTriangles = 8;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
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

RenderQueueUID_t drawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_TRIANGLE;
    lastItem->uid = uid;
    lastItem->x = (x1 + x2 + x3)/3;
    lastItem->y = (y1 + y2 + y3)/3;
    lastItem->thetaZ = (int8_t)color;

    lastItem->numPointsOrTriangles = 6;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    //lastItem now points to a Points_t struct right after the RenderQueueItem.
    clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
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

RenderQueueUID_t drawFilledCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_CIRCLE;
    lastItem->uid = uid;
    lastItem->x = x;
    lastItem->y = y;
    lastItem->z = radius; //Can't use scaleX/Y/Z because they are 8 bit signed (not big enough).
    lastItem->thetaZ = color;
    lastItem->flags = RQI_UPDATE;
    lastItem++;
    return uid++;
}

//Draws lines and fills between all points in the list. Points must be in clockwise order.
RenderQueueUID_t fillPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t color) {
    if(renderQueueNumBytesFree() < sizeof(RenderQueueItem_t)) return 0;
    clearRenderQueueItemData(lastItem);

    lastItem->type = RQI_T_FILLED_POLYGON;
    lastItem->uid = uid;
    lastItem->thetaZ = (int8_t)color;
    lastItem->flags = RQI_UPDATE;

    lastItem->numPointsOrTriangles = numPoints;
    lastItem->pointsOrTriangles = (Points_t *)++lastItem;

    /* This convoluted loop copies the data from points[][] into the Points_t structs. Each Points_t holds 12
     * values (6 coordinate pairs in 2D space). This copies 6 points in, then goes to the next Points_t. This
     * is done with pointer math, treating the points[][] array as a flat 1D array, because it's easier.
    */
    for(int i = 0; i < numPoints*2; i++) {
        if(i % 12 == 0) {
            lastItem++;
            clearRenderQueueItemData(lastItem); //Clear out garbage data in the Points_t struct
        }
        ((Points_t *)lastItem)->points[i % 12] = *((uint16_t *)points + i);
    }

    lastItem++;
    return uid++;
}

RenderQueueUID_t fillScreen(uint8_t * obj, uint8_t color) {
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
        item->uid = RQI_UID_REMOVED;
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

RenderQueueUID_t drawText(uint16_t x1, uint16_t y, uint16_t x2, char *str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverrideBytes) {
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
    ((Points_t *)lastItem)->points[0] = x1;
    ((Points_t *)lastItem)->points[1] = y;
    ((Points_t *)lastItem)->points[2] = x2;
    strcpy((char *)(&((Points_t *)lastItem)->points[3]), str); //Copy the string in *after* the metadata above

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
RenderQueueUID_t drawSprite(uint8_t * sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, int8_t scaleX, int8_t scaleY) {
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
void setHidden(RenderQueueUID_t itemUID, bool hidden) {
    RenderQueueItem_t * item = findRenderQueueItem(itemUID);
    if(item->flags & RQI_HIDDEN) item->flags &= ~RQI_HIDDEN;
    else item->flags |= RQI_HIDDEN;

    updateFullDisplay();
}

//Permanently remove item from the render queue.
void removeItem(RenderQueueUID_t itemUID) {
    if(itemUID == 0) return;
    RenderQueueItem_t * item = findRenderQueueItem(itemUID);
    item->uid = RQI_UID_REMOVED;
    updateFullDisplay();
}