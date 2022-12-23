#include "sdk.h"
#include "sdk-chars.h"

/*
TODO:
- Finish draw functions
- text scaling
- handle out of bounds coordinates for rendering
- make the DMA reconfig interrupt the NMI
- change the interrupt to fire after the DATA channel instead of after the ctrl channel
*/

/*
Ideas:
- GIMP can output C header, raw binary, etc files, make that the sprite format
*/

/*
        Initializers and Prototypes
===========================================
*/
static volatile uint8_t renderState = 0; //Turn the renderer on or off

static volatile uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];

static volatile uint8_t * frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER];

static volatile uint8_t BLANK[FRAME_WIDTH];

//Constants for the DMA channels
#define frameCtrlDMA 0
#define frameDataDMA 1
#define blankDataDMA 2

//LSBs for DMA CTRL register bits (pico's SDK constants weren't great)
#define SDK_DMA_CTRL_EN 0
#define SDK_DMA_CTRL_HIGH_PRIORITY 1
#define SDK_DMA_CTRL_DATA_SIZE 2
#define SDK_DMA_CTRL_INCR_READ 4
#define SDK_DMA_CTRL_INCR_WRITE 5
#define SDK_DMA_CTRL_RING_SIZE 6
#define SDK_DMA_CTRL_RING_SEL 10
#define SDK_DMA_CTRL_CHAIN_TO 11
#define SDK_DMA_CTRL_TREQ_SEL 15
#define SDK_DMA_CTRL_IRQ_QUIET 21
#define SDK_DMA_CTRL_BSWAP 22
#define SDK_DMA_CTRL_SNIFF_EN 23
#define SDK_DMA_CTRL_BUSY 24

volatile RenderQueueItem background = { //First element of the linked list, can be reset to any background
    .type = 'f',
    .color = 0,
    .obj = NULL,
    .next = NULL
};
static volatile RenderQueueItem *lastItem = &background; //Last item in linked list, used to set *last in RenderQueueItem

volatile Controller controller;
static Controller *cPtr = &controller;

//Prototypes, for functions not defined in sdk.h (NOT for use by the user)
static void updateFramePtr();

static bool updateControllerStruct(struct repeating_timer *t);
static void initController();

static void render();

static void initPIO() {
    /*for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
        uint8_t r = (uint8_t) 4*cos(((2*M_PI)/400)*j) + 4;
        uint8_t g = (uint8_t) 4*cos(((2*M_PI)/400)*j + ((4*M_PI)/3)) + 4;
        uint8_t b = (uint8_t) 2*cos(((2*M_PI)/400)*j + ((2*M_PI)/3)) + 2;

        uint8_t color = (r << 5) | (g << 2) | b;

        for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
            frame[i][j] = color;
        }
    }*/
    
    /*for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        if(i % 10 == 0) {
            for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
                frame[i][j] = 255;
            }
        }
        else {
            for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
                if(j % 10 == 0) frame[i][j] = 255;
            }
        }
    }*/

    /*for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
            frame[i][j] = 255;
        }
    }*/

    for(uint16_t i = 0; i < FRAME_FULL_HEIGHT*FRAME_SCALER; i++) {
        if(i >= FRAME_HEIGHT*FRAME_SCALER) frameReadAddr[i] = BLANK;
        else frameReadAddr[i] = frame[i/FRAME_SCALER];
    }

    //PIO Configuration
    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio0, &color_program);

    // Initialize color pio program, but DON'T enable PIO state machine
    color_program_init(pio0, 0, offset, 0, FRAME_SCALER);

    offset = pio_add_program(pio0, &hsync_program);
    hsync_program_init(pio0, 1, offset, HSYNC_PIN);

    offset = pio_add_program(pio0, &vsync_program);
    vsync_program_init(pio0, 2, offset, VSYNC_PIN);


    //DMA Configuration
    dma_claim_mask((1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA)); //mark channels as used in the SDK

    dma_hw->ch[frameCtrlDMA].read_addr = frameReadAddr;
    dma_hw->ch[frameCtrlDMA].write_addr = &dma_hw->ch[frameDataDMA].al3_read_addr_trig;
    dma_hw->ch[frameCtrlDMA].transfer_count = 1;
    dma_hw->ch[frameCtrlDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL);

    dma_hw->ch[frameDataDMA].read_addr = NULL;
    dma_hw->ch[frameDataDMA].write_addr = &pio0_hw->txf[0];
    dma_hw->ch[frameDataDMA].transfer_count = FRAME_WIDTH/4;
    dma_hw->ch[frameDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (blankDataDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL) |
                                        (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->ch[blankDataDMA].read_addr = BLANK;
    dma_hw->ch[blankDataDMA].write_addr = &pio0_hw->txf[0];
    dma_hw->ch[blankDataDMA].transfer_count = (FRAME_FULL_WIDTH - FRAME_WIDTH)/4;
    dma_hw->ch[blankDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) |
                                        (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL)| (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->inte0 = (1 << frameCtrlDMA);

    // Configure the processor to update the line count when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateFramePtr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_hw->multi_channel_trigger = (1 << frameCtrlDMA); //Start!

    //start all 4 state machines at once, sync clocks
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b0111);
}

void initSDK(Controller *c) {
    //Clock configuration -- 120MHz system clock frequency
    clocks_init();
    set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    cPtr = c;

    static struct repeating_timer controllerTimer;
    add_repeating_timer_ms(1, updateControllerStruct, NULL, &controllerTimer);
    sio_hw->gpio_oe_set = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Enable outputs on pins 22 and 26
    initController();

    initPIO();

    //multicore_launch_core1(render);
    //while(multicore_fifo_pop_blocking() != 13); //busy wait while the core is initializing

    sleep_ms(500);

    /*for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        frame[i][0] = 255;
    }
    for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        for(uint16_t j = 1; j < FRAME_WIDTH; j++) {
            frame[i][j] = 255;
            for(uint64_t w = 0; w < 100000; w++) asm("nop");
        }
    }
    while(1);*/

    for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        frame[i][0] = 255;
    }
    while(1) {
        for(uint8_t i = 0; i < 255; i++) {
            uint8_t COLOR = hsvToRGB(i, 255, 255);
            for(uint16_t y = 0; y < FRAME_HEIGHT; y++) {
                for(uint16_t x = 1; x < FRAME_WIDTH; x++) {
                    frame[y][x] = COLOR;
                }
            }
            //for(uint64_t w = 0; w < 500000; w++) asm("nop");
        }
    }

    /*RenderQueueItem *fill = fillScreen(NULL, NULL, 255, false);
    while(1) {
        for(uint8_t i = 0; i < 255; i++) {
            fill->color = hsvToRGB(i, 255, 255);
            render();
            for(uint64_t w = 0; w < 100000; w++) asm("nop");
        }
    }*/

    /*for(uint16_t i = 0; i < FRAME_HEIGHT; i += 100) {
        for(uint16_t j = 0; j < FRAME_WIDTH; j += 100) {
            drawRectangle(NULL, j, i, j + 50, i + 50, 255);
            updateDisplay();
            sleep_ms(2000);
        }
    }*/
    while(1);
}


/*
        Backend PIO Functions
=====================================
*/

static void updateFramePtr() {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << frameCtrlDMA;

    if(dma_hw->ch[frameCtrlDMA].read_addr >= &frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER]) {
        dma_hw->ch[frameCtrlDMA].read_addr = frameReadAddr;
    }
}


/*
        Backend SDK Functions
=====================================

 (The stuff that makes this SDK work)
*/
/*
    Controller Select Truth Table
A | B | Out
0   0   C1
1   0   C2
0   1   C3
1   1   C4
*/
static bool updateControllerStruct(struct repeating_timer *t) {
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


/*
        Color Functions
===============================
*/
uint8_t rgbToByte(uint8_t r, uint8_t g, uint8_t b) {
    return ((r/32) << 5) | ((g/32) << 2) | (b/64);
}

//Hue, saturation, and value are all 0-255 integers
uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value) {
    double max = value;
    double min = max*(255 - saturation);

    double x = (max - min)*(1 - fabs(fmod(hue/(255*(1.0/6.0)), 2) - 1));

    uint8_t r, g, b;
    if(hue < 255*(1.0/6.0)) {
        r = max;
        g = x + min;
        b = min;
    }
    else if(hue < 255*(2.0/6.0)) {
        r = x + min;
        g = max;
        b = min;
    }
    else if(hue < 255*(3.0/6.0)) {
        r = min;
        g = max;
        b = x + min;
    }
    else if(hue < 255*(4.0/6.0)) {
        r = min;
        g = x + min;
        b = max;
    }
    else if(hue < 255*(5.0/6.0)) {
        r = x + min;
        g = min;
        b = max;
    }
    else if(hue < 255*(6.0/6.0)) {
        r = max;
        g = min;
        b = x + min;
    }
    else return 0;

    return ((r/32) << 5) | ((g/32) << 2) | (b/64);
}


/*
        Drawing Functions
=================================
*/
//Checks (and corrects for) overflow on the coordinates (i.e. drawing something at invalid location)
static void checkOvf(uint16_t *x, uint16_t *y) {
    if(*x < 0) *x = 0;
    else if(*x >= FRAME_WIDTH) *x = FRAME_WIDTH - 1;
    if(*y < 0) *y = 0;
    if(*y >= FRAME_HEIGHT) *y = FRAME_HEIGHT - 1;
}

void setBackground(uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color) {
    if(obj == NULL) { //set background to solid color
        background.obj = NULL;
        background.color = color;
    }
    else { //set the background to a picture/sprite/something not a solid color
        background.obj = (uint8_t *)obj;
    }
}

RenderQueueItem * drawPixel(RenderQueueItem* prev, uint16_t x, uint16_t y, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    checkOvf(&x, &y);

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

    return item;
}

RenderQueueItem * drawLine(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'l';
    item->x1 = x1 < x2 ? x1 : x2; //Makes sure the point closer to (0,0) is assigned to (x,y) and not (w,h)
    item->y1 = y1 < y2 ? y1 : y2;
    item->x2 = x1 > x2 ? x1 : x2;
    item->y2 = y1 > y2 ? y1 : y2;
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

    return item;
}

RenderQueueItem * drawRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'r';
    item->x1 = x1 < x2 ? x1 : x2; //Makes sure the point closer to (0,0) is assigned to (x,y) and not (w,h)
    item->y1 = y1 < y2 ? y1 : y2;
    item->x2 = x1 > x2 ? x1 : x2;
    item->y2 = y1 > y2 ? y1 : y2;
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

    return item;
}

RenderQueueItem * drawTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    return NULL;
}

RenderQueueItem * drawCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
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

    return item;
}

RenderQueueItem * drawNPoints(RenderQueueItem* prev, uint16_t points[][2], uint8_t len, uint8_t color) { //draws a path between all points in the list
    return NULL;
}

RenderQueueItem * fillRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'R';
    item->x1 = x1 < x2 ? x1 : x2; //Makes sure the point closer to (0,0) is assigned to (x,y) and not (w,h)
    item->y1 = y1 < y2 ? y1 : y2;
    item->x2 = x1 > x2 ? x1 : x2;
    item->y2 = y1 > y2 ? y1 : y2;
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

    return item;
}

RenderQueueItem * fillTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill) {
    return NULL;
}

RenderQueueItem * fillCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill) {
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

    return item;
}

RenderQueueItem * fillScreen(RenderQueueItem* prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color, bool clearRenderQueue) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'f';
    if(obj == NULL) {
        item->obj = NULL;
        item->color = color;
    }
    else {
        item->obj = (uint8_t *)obj;
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

    return item;
}

//Removes everything from the linked list, free()'s all memory, resets background to black
void clearScreen() {
    background.type = 'f';
    background.color = 0;

    RenderQueueItem *item = background.next; //DO NOT REMOVE the first element of the linked list! (background)
    while(item != NULL) {
        item->type = 'n';
        item = item->next;
    }
}


/*
        Text Functions
==============================
*/
static uint8_t (*font)[256][8] = cp437; //The current font in use by the system
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8

RenderQueueItem * drawText(RenderQueueItem* prev, uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 'c';
    item->x1 = x;
    item->y1 = y;
    item->color = color;
    item->obj = str;

    if(prev == NULL) {
        item->next = NULL; //Set *next to NULL, means it is the last item in linked list

        lastItem->next = item; //Link the last item to this one
        lastItem = item; //Reset lastItem
    }
    else {
        item->next = prev->next; //insert item into the linked list
        prev->next = item;
    }

    return item;
}

void changeFont(uint8_t (*newFont)[256][8]) {
    font = newFont;
}


/*
        Sprite Drawing Functions
========================================
*/
RenderQueueItem * drawSprite(RenderQueueItem* prev, uint16_t x, uint16_t y, uint8_t *sprite, uint16_t dimX, uint16_t dimY, uint8_t colorOverride, uint8_t scale) {
    RenderQueueItem *item = (RenderQueueItem *) malloc(sizeof(RenderQueueItem));
    if(item == NULL) return NULL;

    item->type = 's';
    item->x1 = x; //Makes sure the point closer to (0,0) is assigned to (x,y) and not (w,h)
    item->y1 = y;
    item->x2 = x + dimX;
    item->y2 = y + dimY;
    item->color = colorOverride != COLOR_NULL ? colorOverride : 0; //color = 0: use colors from sprite, else use override
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

    return item;
}


/*
        Renderer!
=========================
*/
static uint8_t update = 0;

void updateDisplay() {
    update = 1;
}

static void render() {
    //multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    RenderQueueItem *item;
    RenderQueueItem *previousItem;

    //while(1) {
        //while(!update); //wait until it's told to update the display

        item = &background;

        while(item != NULL) {
            switch(item->type) {
                case 'p': //Pixel
                    frame[item->y1][item->x1] = item->color;
                    break;
                case 'l': //Line
                    if(item->x1 == item->x2) {
                        for(uint16_t y = item->y1; y <= item->y2; y++) frame[y][item->x1] = item->color;
                    }
                    else {
                        uint16_t m = (uint16_t)((item->y2 - item->y1)/(item->x2 - item->x1));
                        for(uint16_t x = item->x1; x <= item->x2; x++) {
                            frame[(m*(x - item->x1)) + item->y1][x] = item->color;
                        }
                    }
                    break;
                case 'r': //Rectangle
                    for(uint16_t x = item->x1; x <= item->x2; x++) frame[item->y1][x] = item->color; //top
                    for(uint16_t x = item->x1; x <= item->x2; x++) frame[item->y2][x] = item->color; //bottom
                    for(uint16_t y = item->y1; y <= item->y2; y++) frame[y][item->x1] = item->color; //left
                    for(uint16_t y = item->y1; y <= item->y2; y++) frame[y][item->x2] = item->color; //right
                    break;
                case 't': //Triangle
                    break;
                case 'o': //Circle
                    //x1, y1 = center, x2 = radius
                    for(uint16_t y = item->y1 - item->x2; y <= item->y1 + item->x2; y++) {
                        uint16_t x = sqrt(pow(item->x2, 2.0) - pow(y - item->y1, 2.0));
                        frame[y][x + item->x1] = item->color; //handle the +/- from the sqrt (the 2 sides of the circle)
                        frame[y][x - item->x1] = item->color;
                    }
                    break;
                case 'R': //Filled Rectangle
                    for(uint16_t y = item->y1; y <= item->y2; y++) {
                        for(uint16_t x = item->x1; x <= item->x2; x++) {
                            frame[y][x] = item->color;
                        }
                    }
                    break;
                case 'T': //Filled Triangle
                    break;
                case 'O': //Filled Circle
                    //x1, y1 = center, x2 = radius
                    for(uint16_t y = item->y1 - item->x2; y <= item->y1 + item->x2; y++) {
                        uint16_t x = sqrt(pow(item->x2, 2.0) - pow(y - item->y1, 2.0));
                        for(uint16_t i = x - item->x1; i <= x + item->x1; i++) {
                            frame[y][i] = item->color;
                        }
                    }
                    break;
                case 'c': //Character/String
                    /*for(uint16_t i = 0; item->obj[i] != '\0'; i++) {
                        x = item->x1 + (i*CHAR_WIDTH);
                        for(uint8_t j; j < CHAR_WIDTH; j++) {
                            //font[][] is a bit array, so check if a particular bit is true, if so, set the pixel array
                            if(((*font)[item->obj[i]][currentLine - item->y1]) & ((1 << (CHAR_WIDTH - 1)) >> j)) {
                                frame[l][x + j] = item->color;
                            }
                        }
                    }*/
                    break;
                case 's': //Sprites
                    /*for(uint16_t i = item->x1; i < item->x2; i++) {
                        //Skip changing the pixel if it's set to the COLOR_NULL value
                        if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;

                        if(item->color == 0) {
                            //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                            frame[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                        else {
                            frame[l][i] = item->color;
                        }
                    }*/
                    break;
                case 'f': //Fill the screen
                    for(uint16_t y = 0; y < FRAME_HEIGHT; y++) {
                        for(uint16_t x = 0; x < FRAME_WIDTH; x++) {
                            frame[y][x] = item->color;
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
                case 'n': //Deleted item (garbage collector)
                    previousItem->next = item->next;
                    free(item);
                    item = previousItem; //prevent the code from skipping items
                    break;
            }
            
            previousItem = item;
            item = item->next;
        }

        //update = 0;
    //}
}


/*static void render() {
    //multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    uint8_t l; //which line the code is writing to
    uint16_t x; //scratch variable
    RenderQueueItem *item;
    RenderQueueItem *previousItem;

    while(1) {
        if(!renderState) continue;
        l = (l + 1) % FRAME_HEIGHT; //update which line is being written to

        item = &background;
        previousItem = NULL;
        while(item != NULL) {
            //If the item is hidden or does not cross the current line being rendered, ignore it
            if(item->type == 'h' || item->y1 > currentLine || item->y2 < currentLine) {
                previousItem = item;
                item = item->next;
                continue;
            }

            switch(item->type) {
                case 'p': //Pixel
                    frame[l][item->x1] = item->color;
                    break;
                case 'l': //Line
                     //Point slope form solved for x -- SHOULD y (currentLine) BE NEGATIVE?
                    x = ((currentLine - item->y1)/((item->y2 - item->y1)/(item->x2 - item->x1))) + item->x1;
                    frame[l][x] = item->color;
                    break;
                case 'r': //Rectangle
                    if(item->y1 == currentLine || item->y2 == currentLine) {
                        for(uint16_t i = item->x1; i <= item->x2; i++) {
                            frame[l][i] = item->color;
                        }
                    }
                    else {
                        frame[l][item->x1] = item->color; //the two sides of the rectangle
                        frame[l][item->x2] = item->color;
                    }
                    break;
                case 't': //Triangle
                    break;
                case 'o': //Circle
                    //Standard form of circle solved for x-h (left here because of +/- from sqrt)
                    x = sqrt(pow(item->x2, 2.0) - pow(currentLine - item->y1, 2.0));
                    frame[l][x + item->x1] = item->color; //handle the +/- from the sqrt (the 2 sides of the circle)
                    frame[l][x - item->x1] = item->color;
                    break;
                case 'R': //Filled Rectangle
                    for(uint16_t i = item->x1; i <= item->x2; i++) {
                        frame[l][i] = item->color;
                    }
                    break;
                case 'T': //Filled Triangle
                    break;
                case 'O': //Filled Circle
                    //Standard form of circle solved for x-h (left here because of +/- from sqrt)
                    x = sqrt(pow(item->x2, 2.0) - pow(currentLine - item->y1, 2.0));
                    for(uint16_t i = x - item->x1; i < x + item->x1; i++) {
                        frame[l][i] = item->color;
                    }
                    break;
                case 'c': //Character/String
                    for(uint16_t i = 0; item->obj[i] != '\0'; i++) {
                        x = item->x1 + (i*CHAR_WIDTH);
                        for(uint8_t j; j < CHAR_WIDTH; j++) {
                            //font[][] is a bit array, so check if a particular bit is true, if so, set the pixel array
                            if(((*font)[item->obj[i]][currentLine - item->y1]) & ((1 << (CHAR_WIDTH - 1)) >> j)) {
                                frame[l][x + j] = item->color;
                            }
                        }
                    }
                    break;
                case 's': //Sprites
                    for(uint16_t i = item->x1; i < item->x2; i++) {
                        //Skip changing the pixel if it's set to the COLOR_NULL value
                        if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;

                        if(item->color == 0) {
                            //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                            frame[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                        else {
                            frame[l][i] = item->color;
                        }
                    }
                    break;
                case 'f': //Fill the screen
                    if(item->obj == NULL) {
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
                    }
                    break;
                case 'n': //Deleted item (garbage collector)
                    previousItem->next = item->next;
                    free(item);
                    item = previousItem; //prevent the code from skipping items
                    break;
            }

            previousItem = item;
            item = item->next; //increment linked list
        }
    }
}*/