#include "sdk.h"
#include "font.h"

#include "pico/malloc.h"

RenderQueueItem * drawPixel(RenderQueueItem *prev, uint16_t x, uint16_t y, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'p';
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

    item->type = 'l';
    item->x1 = x1 <= x2 ? x1 : x2; //Makes sure the point closer to x = 0 is set to (x1, y1) not (x2, y2)
    item->y1 = x1 <= x2 ? y1 : y2;
    item->x2 = x1 >  x2 ? x1 : x2;
    item->y2 = x1 >  x2 ? y1 : y2;
    item->color = color;
    item->thickness = thickness - 1;

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

    item->type = 'r';
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

    side1->type = 't';
    side2->type = 't';
    side3->type = 't';

    //sort points by x value
    //assign

    return NULL;
}

RenderQueueItem * drawCircle(RenderQueueItem *prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'o';
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

RenderQueueItem * drawNPoints(RenderQueueItem *prev, uint16_t points[][2], uint8_t len, uint8_t color) { //draws a path between all points in the list
    return NULL;
}

RenderQueueItem * drawFilledRectangle(RenderQueueItem *prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'R';
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

    item->type = 'O';
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

    item->type = 'f';
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
    background.type = 'f';
    background.color = 0;

    RenderQueueItem *item = background.next; //DO NOT REMOVE the first element of the linked list! (background)
    while(item != NULL) {
        item->type = 'n';
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

    item->type = 'c';
    item->x1 = x1;
    item->y1 = y;
    item->x2 = x2;
    item->y2 = bgColor;
    item->color = color;

    if(strSizeOverride) {
        item->obj = (uint8_t *) malloc((strSizeOverride + 1)*sizeof(char));
    }
    else {
        item->obj = (uint8_t *) malloc((strlen(str) + 1)*sizeof(char));
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

    item->type = 's';
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
//Set the parameters for the background.
void setBackground(uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color) {
    if(obj == NULL) { //set background to solid color
        background.obj = NULL;
        background.color = color;
    }
    else { //set the background to a picture/sprite/something not a solid color
        background.obj = (uint8_t *)obj;
    }
    background.flags |= RQI_UPDATE;
}

//Set item to be hidden (true = hidden, false = showing)
void setHidden(RenderQueueItem *item, uint8_t hidden) {
    if(hidden) item->flags |= (1u << 1);
    else item->flags &= ~(1u << 1);
    item->flags |= RQI_UPDATE;
}

//Set the color for item.
void setColor(RenderQueueItem *item, uint8_t color) {
    item->color = color;
    item->flags |= RQI_UPDATE;
}

//Set the coordinates for item. Set parameter to -1 to leave it unchanged.
void setCoordinates(RenderQueueItem *item, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if(x1 >= 0) item->x1 = x1;
    if(y1 >= 0) item->y1 = y1;
    if(x2 >= 0) item->x2 = x2;
    if(y2 >= 0) item->y2 = y2;
    item->flags |= RQI_UPDATE;
}

//Permanently remove item from the render queue.
void removeItem(RenderQueueItem *item) {
    item->type = 'n';
    item->flags |= RQI_UPDATE;
}