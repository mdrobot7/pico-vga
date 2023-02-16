#include "sdk.h"
#include "font.h"

#include "pico/malloc.h"

RenderQueueItem * drawPixel(RenderQueueItem *prev, uint16_t x, uint16_t y, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_PIXEL;
    item->x1 = x;
    item->y1 = y;
    item->color = color;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * drawLine(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_LINE;
    item->x1 = x1 <= x2 ? x1 : x2; //Makes sure the point closer to x = 0 is set to (x1, y1) not (x2, y2)
    item->y1 = x1 <= x2 ? y1 : y2;
    item->x2 = x1 >  x2 ? x1 : x2;
    item->y2 = x1 >  x2 ? y1 : y2;
    item->color = color;
    item->scale = thickness;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * drawRectangle(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_RECTANGLE;
    item->x1 = x1 <= x2 ? x1 : x2; //Makes sure the point closer to x = 0 is set to (x1, y1) not (x2, y2)
    item->y1 = x1 <= x2 ? y1 : y2;
    item->x2 = x1 >  x2 ? x1 : x2;
    item->y2 = x1 >  x2 ? y1 : y2;
    item->color = color;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * drawTriangle(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    RenderQueueItem *side1 = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    RenderQueueItem *side2 = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    RenderQueueItem *side3 = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(side1 == NULL || side2 == NULL || side3 == NULL) return NULL;

    side1->type = RQI_T_TRIANGLE;
    side2->type = RQI_T_TRIANGLE;
    side3->type = RQI_T_TRIANGLE;

    //sort points by x value
    //assign

    return NULL;
}

RenderQueueItem * drawCircle(RenderQueueItem *prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_CIRCLE;
    item->x1 = x;
    item->y1 = y;
    item->x2 = radius;
    item->color = color;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * drawPolygon(RenderQueueItem *prev, uint16_t points[][2], uint8_t len, uint8_t color) { //draws a path between all points in the list
    return NULL;
}

RenderQueueItem * drawFilledRectangle(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_FILLED_RECTANGLE;
    item->x1 = x1 <= x2 ? x1 : x2; //Makes sure the point closer to x = 0 is set to (x1, y1) not (x2, y2)
    item->y1 = x1 <= x2 ? y1 : y2;
    item->x2 = x1 >  x2 ? x1 : x2;
    item->y2 = x1 >  x2 ? y1 : y2;
    item->color = color;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * drawFilledTriangle(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill) {
    return NULL;
}

RenderQueueItem * drawFilledCircle(RenderQueueItem *prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_FILLED_CIRCLE;
    item->x1 = x;
    item->y1 = y;
    item->x2 = radius;
    item->color = color;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

RenderQueueItem * fillScreen(RenderQueueItem *prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_FILL;
    if(obj == NULL) {
        item->obj = NULL;
        item->color = color;
    }
    else {
        item->obj = (uint8_t *)obj;
        item->color = color;
    }

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}

//Removes everything from the linked list, marks all elements for deletion (handled in render()), resets background to black
void clearScreen() {
    background.type = RQI_T_FILL;
    background.color = 0;

    RenderQueueItem *item = background.next; //DO NOT REMOVE the first element of the linked list! (background)
    while(item != NULL) {
        item->type = RQI_T_REMOVED;
        item = item->next;
    }

    background.flags = RQI_UPDATE;
}


/*
        Text Functions
==============================
*/
uint8_t *font = (uint8_t *)cp437; //The current font in use by the system

RenderQueueItem * drawText(RenderQueueItem *prev, uint16_t x1, uint16_t y, uint16_t x2, char *str,
                           uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverride) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_STR;
    item->x1 = x1;
    item->y1 = y;
    item->x2 = x2;
    item->y2 = bgColor;
    item->color = color;

    if(strSizeOverride) {
        item->obj = (uint8_t *) malloc((strSizeOverride + 1)*sizeof(uint8_t));
    }
    else {
        item->obj = (uint8_t *) malloc((strlen(str) + 1)*sizeof(uint8_t));
    }
    strcpy(item->obj, str);

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;
    if(wrap) item->flags |= RQI_WORDWRAP;

    return item;
}

void setTextFont(uint8_t *newFont) {
    font = newFont;
}


/*
        Sprite Drawing Functions
========================================
*/
RenderQueueItem * drawSprite(RenderQueueItem *prev, uint8_t *sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, uint8_t scale) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = RQI_T_SPRITE;
    item->x1 = x;
    item->y1 = y;
    item->x2 = dimX;
    item->y2 = dimY;
    item->color = nullColor;
    item->obj = sprite;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    item->flags = RQI_UPDATE;

    return item;
}


/*
        Render Queue Modifiers
======================================
*/
//Set item to be hidden (true = hidden, false = showing)
void setHidden(RenderQueueItem *item, uint8_t hidden) {
    if(hidden) item->flags |= RQI_HIDDEN;
    else item->flags &= ~RQI_HIDDEN;
    updateFullDisplay();
}

//Set the color for item.
void setColor(RenderQueueItem *item, uint8_t color) {
    item->color = color;
    item->flags |= RQI_UPDATE;
}

//Set the coordinates for item. Set parameter to -1 to leave it unchanged.
void setCoordinates(RenderQueueItem *item, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if(item->x1 >= 0) item->x1 = x1 <= x2 ? x1 : x2; //Makes sure the point closer to x = 0 is set to (x1, y1) not (x2, y2)
    if(item->y1 >= 0) item->y1 = x1 <= x2 ? y1 : y2;
    if(item->x2 >= 0) item->x2 = x1 >  x2 ? x1 : x2;
    if(item->y2 >= 0) item->y2 = x1 >  x2 ? y1 : y2;
    updateFullDisplay();
}

//Set an item's scale (line thickness, sprite scale, text scale)
void setScale(RenderQueueItem *item, uint8_t scale) {
    item->scale = scale;
    item->flags |= RQI_UPDATE;
}
 
//Set the rotation of an item with a byte
void setRotationByte(RenderQueueItem *item, uint8_t rotation) {
    return;
}

//Set the rotation of an item with radians (range: [0-2pi])
void setRotationRadians(RenderQueueItem *item, double rotation) {
    return;
}

//Set a sprite or bitmap's object
void setSpriteObj(RenderQueueItem *item, uint8_t *obj, uint8_t dimX, uint8_t dimY) {
    if(item->x2 != dimX || item->y2 != dimY) updateFullDisplay(); //handle if the sprite shrunk, leaving artifacts
    
    item->x2 = dimX;
    item->y2 = dimY;
    item->obj = obj;
    item->flags |= RQI_UPDATE;
}

//Set a string item's text (This copies the string given into the previously allocated memory space. Make sure
//your string RenderQueueItem has enough space allocated for the new string, otherwise the program may have
//issues. If unsure, make a new RenderQueueItem using drawText and delete this one using removeItem.)
void setText(RenderQueueItem *item, char *str) {
    strcpy(item->obj, str);
    item->flags |= RQI_UPDATE;
}

//Permanently remove item from the render queue.
void removeItem(RenderQueueItem *item) {
    item->type = RQI_T_REMOVED;
    updateFullDisplay();
}