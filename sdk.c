#include "main.h"
#include "sdk.h"

/*
        Backend SDK Functions
=====================================

 (The stuff that makes this SDK work)
*/

static uint16_t queueIndex = 0;
static Controller *cPtr = &controller;

//Prototypes, for functions not defined in sdk.h (NOT for use by the user)
bool updateControllerStruct(struct repeating_timer *t);
static void initController();

void initSDK(Controller *c) {
    setController(c);

    struct repeating_timer controllerTimer;
    add_repeating_timer_ms(1, updateControllerStruct, NULL, &controllerTimer);
    sio_hw->gpio_oe_set = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Enable outputs on pins 22 and 26
    initController();
}

void setController(Controller *c) {
    cPtr = c;
}

/*
    Controller Select Truth Table
A | B | Out
0   0   C1
1   0   C2
0   1   C3
1   1   C4
*/
bool updateControllerStruct(struct repeating_timer *t) {
    sio_hw->gpio_clr = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Connect Controller 1
    cPtr->C1.u = (sio_hw->gpio_in >> UP_PIN) & 1u;
    cPtr->C1.d = (sio_hw->gpio_in >> DOWN_PIN) & 1u;
    cPtr->C1.l = (sio_hw->gpio_in >> LEFT_PIN) & 1u;
    cPtr->C1.r = (sio_hw->gpio_in >> RIGHT_PIN) & 1u;
    cPtr->C1.a = (sio_hw->gpio_in >> A_PIN) & 1u;
    cPtr->C1.b = (sio_hw->gpio_in >> B_PIN) & 1u;
    cPtr->C1.x = (sio_hw->gpio_in >> X_PIN) & 1u;
    cPtr->C1.y = (sio_hw->gpio_in >> Y_PIN) & 1u;

    sio_hw->gpio_set = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 2
    cPtr->C2.u = (sio_hw->gpio_in >> UP_PIN) & 1u;
    cPtr->C2.d = (sio_hw->gpio_in >> DOWN_PIN) & 1u;
    cPtr->C2.l = (sio_hw->gpio_in >> LEFT_PIN) & 1u;
    cPtr->C2.r = (sio_hw->gpio_in >> RIGHT_PIN) & 1u;
    cPtr->C2.a = (sio_hw->gpio_in >> A_PIN) & 1u;
    cPtr->C2.b = (sio_hw->gpio_in >> B_PIN) & 1u;
    cPtr->C2.x = (sio_hw->gpio_in >> X_PIN) & 1u;
    cPtr->C2.y = (sio_hw->gpio_in >> Y_PIN) & 1u;

    sio_hw->gpio_clr = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 3
    sio_hw->gpio_set = (1u << CONTROLLER_SEL_B_PIN);
    cPtr->C3.u = (sio_hw->gpio_in >> UP_PIN) & 1u;
    cPtr->C3.d = (sio_hw->gpio_in >> DOWN_PIN) & 1u;
    cPtr->C3.l = (sio_hw->gpio_in >> LEFT_PIN) & 1u;
    cPtr->C3.r = (sio_hw->gpio_in >> RIGHT_PIN) & 1u;
    cPtr->C3.a = (sio_hw->gpio_in >> A_PIN) & 1u;
    cPtr->C3.b = (sio_hw->gpio_in >> B_PIN) & 1u;
    cPtr->C3.x = (sio_hw->gpio_in >> X_PIN) & 1u;
    cPtr->C3.y = (sio_hw->gpio_in >> Y_PIN) & 1u;

    sio_hw->gpio_set = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 4
    cPtr->C4.u = (sio_hw->gpio_in >> UP_PIN) & 1u;
    cPtr->C4.d = (sio_hw->gpio_in >> DOWN_PIN) & 1u;
    cPtr->C4.l = (sio_hw->gpio_in >> LEFT_PIN) & 1u;
    cPtr->C4.r = (sio_hw->gpio_in >> RIGHT_PIN) & 1u;
    cPtr->C4.a = (sio_hw->gpio_in >> A_PIN) & 1u;
    cPtr->C4.b = (sio_hw->gpio_in >> B_PIN) & 1u;
    cPtr->C4.x = (sio_hw->gpio_in >> X_PIN) & 1u;
    cPtr->C4.y = (sio_hw->gpio_in >> Y_PIN) & 1u;
 
    return true; //not sure if this is required or not, but it's in the sample code
}

static void initController() {
    cPtr->C1.a = 0;
    cPtr->C1.b = 0;
    cPtr->C1.x = 0;
    cPtr->C1.y = 0;
    cPtr->C1.u = 0;
    cPtr->C1.d = 0;
    cPtr->C1.l = 0;
    cPtr->C1.r = 0;

    cPtr->C2.a = 0;
    cPtr->C2.b = 0;
    cPtr->C2.x = 0;
    cPtr->C2.y = 0;
    cPtr->C2.u = 0;
    cPtr->C2.d = 0;
    cPtr->C2.l = 0;
    cPtr->C2.r = 0;

    cPtr->C3.a = 0;
    cPtr->C3.b = 0;
    cPtr->C3.x = 0;
    cPtr->C3.y = 0;
    cPtr->C3.u = 0;
    cPtr->C3.d = 0;
    cPtr->C3.l = 0;
    cPtr->C3.r = 0;

    cPtr->C4.a = 0;
    cPtr->C4.b = 0;
    cPtr->C4.x = 0;
    cPtr->C4.y = 0;
    cPtr->C4.u = 0;
    cPtr->C4.d = 0;
    cPtr->C4.l = 0;
    cPtr->C4.r = 0;
}

//GARBAGE COLLECTOR
void cleanRenderQueue() {
    //Remove indexes that contain removed elements, collapse render queue
    for(int i = 0; i < RENDER_QUEUE_LEN; i++) {
        if(renderQueue[i].type == 'n') {
            for(int j = i; j < RENDER_QUEUE_LEN; j++) {
                renderQueue[j] = renderQueue[j + 1]; //shift each index down by one
            }
        }
    }

    //Implement this LATER, it might actually make things worse.
    //check every index, see if one is "useless" -- all of its pixels are overwritten later
}


/*
        Drawing Functions
=================================
*/

//If the render queue is full, returns false. Else true.
static bool renderQueueAvailable(uint16_t index) {
    if(index == RENDER_QUEUE_LEN) return false;
    else return true;
}

int16_t drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'p';
    renderQueue[queueIndex].x = x;
    renderQueue[queueIndex].y = y;
    renderQueue[queueIndex].w = 1;
    renderQueue[queueIndex].h = 1;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

int16_t drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'l';
    renderQueue[queueIndex].x = x1;
    renderQueue[queueIndex].y = y1;
    renderQueue[queueIndex].w = x2;
    renderQueue[queueIndex].h = y2;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

int16_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'r';
    renderQueue[queueIndex].x = x1;
    renderQueue[queueIndex].y = y1;
    renderQueue[queueIndex].w = x2;
    renderQueue[queueIndex].h = y2;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

int16_t drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    return;
}

int16_t drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'o';
    renderQueue[queueIndex].x = x;
    renderQueue[queueIndex].y = y;
    renderQueue[queueIndex].w = radius;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

int16_t drawNPoints(uint16_t points[][2], uint8_t len, uint8_t color) { //draws a path between all points in the list
    return;
}

int16_t fillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'R';
    renderQueue[queueIndex].x = x1;
    renderQueue[queueIndex].y = y1;
    renderQueue[queueIndex].w = x2;
    renderQueue[queueIndex].h = y2;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

int16_t fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill) {
    return;
}

int16_t fillCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 'O';
    renderQueue[queueIndex].x = x;
    renderQueue[queueIndex].y = y;
    renderQueue[queueIndex].w = radius;
    renderQueue[queueIndex].color = color;
    queueIndex++;

    return queueIndex - 1;
}

void fillScreen(uint8_t color, bool clearRenderQueue) {
    if(!renderQueueAvailable) return -1;

    if(clearRenderQueue) {
        for(int i = 0; i < queueIndex; i++) {
            renderQueue[i].type = 'n';
        }
    }

    renderQueue[queueIndex].type = 'f';
    renderQueue[queueIndex].color = color;
    queueIndex++;
}

void clearScreen() {
    for(int i = 0; i < RENDER_QUEUE_LEN; i++) {
        renderQueue[i].type = 'n';
    }
}


/*
        Text Functions
==============================
*/

static uint8_t *font = &cp437[0][0]; //The current font in use by the system

int16_t drawText(uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale) {
    if(!renderQueueAvailable) return -1;

    int i = 0;
    while(str[i] != '\0') {
        printf("%c", str[i]);
        i++;
    }
}

void changeFont(uint8_t *newFont) {
    font = newFont;
}


/*
        Sprite Drawing Functions
========================================
*/

int16_t drawSprite(uint16_t x, uint16_t y, uint8_t *sprite, uint16_t dimX, uint16_t dimY, uint8_t colorOverride, uint8_t scale) {
    if(!renderQueueAvailable) return -1;

    renderQueue[queueIndex].type = 's';
    renderQueue[queueIndex].obj = sprite;
    renderQueue[queueIndex].x = x;
    renderQueue[queueIndex].y = y;
    renderQueue[queueIndex].w = dimX;
    renderQueue[queueIndex].h = dimY;
    renderQueue[queueIndex].color = colorOverride;
    queueIndex++;

    return queueIndex - 1;
}


/*
        Renderer!
=========================
*/