#include "sdk.h"
#include "sdk-chars.h"

/*
TODO:
- Finish draw functions
- text scaling
*/

/*
Ideas:
- GIMP can output C header, raw binary, etc files, make that the sprite format
*/

/*
        Initializers and Prototypes
===========================================
*/
static uint8_t renderState = 0; //Turn the renderer on or off

static uint8_t line[2][FRAME_FULL_WIDTH]; //The pixel arrays for rendering (even lines = line[0], odd lines = line[1])
static volatile uint16_t currentLine = 0; //The current line that the renderer is on

//Constants for the DMA channels
#define line0CtrlDMA 0
#define line0DataDMA 1
#define line1CtrlDMA 2
#define line1DataDMA 3

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

RenderQueueItem background = { //First element of the linked list, can be reset to any background
    .type = 'f',
    .color = 0,
    .obj = NULL,
    .next = NULL
};
static RenderQueueItem *lastItem = &background; //Last item in linked list, used to set *last in RenderQueueItem

Controller controller;

static Controller *cPtr = &controller;

//Prototypes, for functions not defined in sdk.h (NOT for use by the user)
static void updateLineCount();

static bool updateControllerStruct(struct repeating_timer *t);
static void initController();

static void render();

static void initPIO() {
    for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
        line[0][j] = 0b11100011;
    }
    for(uint16_t j = FRAME_WIDTH; j < FRAME_FULL_WIDTH; j++) {
        line[0][j] = 0;
    }
    line[0][0] = 1;
    line[0][FRAME_WIDTH - 1] = 1;

    for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
        line[1][i] = 0b00011100;
    }
    for(uint16_t j = FRAME_WIDTH; j < FRAME_FULL_WIDTH; j++) {
        line[1][j] = 0;
    }
    line[1][0] = 1;
    line[1][FRAME_WIDTH - 1] = 1;


    //PIO Configuration

    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio0, &color_program);

    // Initialize color pio program, but DON'T enable PIO state machine
    color_program_init(pio0, 0, offset, 0);

    offset = pio_add_program(pio0, &hsync_program);
    hsync_program_init(pio0, 1, offset, HSYNC_PIN);

    offset = pio_add_program(pio0, &vsync_program);
    vsync_program_init(pio0, 2, offset, VSYNC_PIN);


    //DMA Configuration

    dma_claim_mask((1 << line0CtrlDMA) | (1 << line0DataDMA) | (1 << line1CtrlDMA) | (1 << line1DataDMA)); //mark channels as used in the SDK
    static uint32_t line0Ptr = (uint32_t)line[0];
    static uint32_t line1Ptr = (uint32_t)line[1];

    dma_hw->ch[line0CtrlDMA].read_addr = &line0Ptr;
    dma_hw->ch[line0CtrlDMA].write_addr = &dma_hw->ch[line0DataDMA].al3_read_addr_trig;
    dma_hw->ch[line0CtrlDMA].transfer_count = 1;
    dma_hw->ch[line0CtrlDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (line0CtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | 
                                        (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL)   | (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->ch[line0DataDMA].read_addr = NULL;
    dma_hw->ch[line0DataDMA].write_addr = &pio0_hw->txf[0];
    dma_hw->ch[line0DataDMA].transfer_count = FRAME_FULL_WIDTH/4;
    dma_hw->ch[line0DataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (line1CtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL);
    
    dma_hw->ch[line1CtrlDMA].read_addr = &line1Ptr;
    dma_hw->ch[line1CtrlDMA].write_addr = &dma_hw->ch[line1DataDMA].al3_read_addr_trig;
    dma_hw->ch[line1CtrlDMA].transfer_count = 1;
    dma_hw->ch[line1CtrlDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (line1CtrlDMA << SDK_DMA_CTRL_CHAIN_TO) |
                                        (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL)   | (1 << SDK_DMA_CTRL_IRQ_QUIET);
    
    dma_hw->ch[line1DataDMA].read_addr = NULL;
    dma_hw->ch[line1DataDMA].write_addr = &pio0_hw->txf[0];
    dma_hw->ch[line1DataDMA].transfer_count = FRAME_FULL_WIDTH/4;
    dma_hw->ch[line1DataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (line0CtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL);

    dma_hw->inte0 = (1 << line0DataDMA) | (1 << line1DataDMA);

    // Configure the processor to update the line count when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateLineCount);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_hw->multi_channel_trigger = (1 << line0CtrlDMA); //Start!

    //start all 4 state machines at once, sync clocks
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b0111);
}

void initSDK(Controller *c) {
    //Clock configuration -- 120MHz system clock frequency
    clocks_init();
    set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    cPtr = c;

    struct repeating_timer controllerTimer;
    add_repeating_timer_ms(1, updateControllerStruct, NULL, &controllerTimer);
    sio_hw->gpio_oe_set = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Enable outputs on pins 22 and 26
    initController();

    initPIO();
    
    //multicore_launch_core1(render);
    //while(multicore_fifo_pop_blocking() != 13); //busy wait while the core is initializing
}

//Turn the renderer on or off
void setRendererState(uint8_t state) {
    renderState = state;
}


/*
        Backend PIO Functions
=====================================
*/
static void updateLineCount() {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << line0DataDMA;
    dma_hw->ints0 = 1u << line1DataDMA;

    currentLine++;
    if(currentLine == FRAME_HEIGHT) {
        currentLine = 0;
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
    return 0;
}

uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value) {
    return 0;
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
static void render() {
    multicore_fifo_push_blocking(13); //tell core 0 that everything is ok/it's running

    uint8_t l; //which index of line[][] I'm writing to (line[0] or line[1])
    uint16_t x; //scratch variable
    RenderQueueItem *item;
    RenderQueueItem *previousItem;

    while(1) {
        if(!renderState) continue;
        l = (currentLine + 1) % 2; //update which line is being written to

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
                    line[l][item->x1] = item->color;
                    break;
                case 'l': //Line
                     //Point slope form solved for x -- SHOULD y (currentLine) BE NEGATIVE?
                    x = ((currentLine - item->y1)/((item->y2 - item->y1)/(item->x2 - item->x1))) + item->x1;
                    line[l][x] = item->color;
                    break;
                case 'r': //Rectangle
                    if(item->y1 == currentLine || item->y2 == currentLine) {
                        for(uint16_t i = item->x1; i <= item->x2; i++) {
                            line[l][i] = item->color;
                        }
                    }
                    else {
                        line[l][item->x1] = item->color; //the two sides of the rectangle
                        line[l][item->x2] = item->color;
                    }
                    break;
                case 't': //Triangle
                    break;
                case 'o': //Circle
                    //Standard form of circle solved for x-h (left here because of +/- from sqrt)
                    x = sqrt(pow(item->x2, 2.0) - pow(currentLine - item->y1, 2.0));
                    line[l][x + item->x1] = item->color; //handle the +/- from the sqrt (the 2 sides of the circle)
                    line[l][x - item->x1] = item->color;
                    break;
                case 'R': //Filled Rectangle
                    for(uint16_t i = item->x1; i <= item->x2; i++) {
                        line[l][i] = item->color;
                    }
                    break;
                case 'T': //Filled Triangle
                    break;
                case 'O': //Filled Circle
                    //Standard form of circle solved for x-h (left here because of +/- from sqrt)
                    x = sqrt(pow(item->x2, 2.0) - pow(currentLine - item->y1, 2.0));
                    for(uint16_t i = x - item->x1; i < x + item->x1; i++) {
                        line[l][i] = item->color;
                    }
                    break;
                case 'c': //Character/String
                    for(uint16_t i = 0; item->obj[i] != '\0'; i++) {
                        x = item->x1 + (i*CHAR_WIDTH);
                        for(uint8_t j; j < CHAR_WIDTH; j++) {
                            //font[][] is a bit array, so check if a particular bit is true, if so, set the pixel array
                            if(((*font)[item->obj[i]][currentLine - item->y1]) & ((1 << (CHAR_WIDTH - 1)) >> j)) {
                                line[l][x + j] = item->color;
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
                            line[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                        else {
                            line[l][i] = item->color;
                        }
                    }
                    break;
                case 'f': //Fill the screen
                    if(item->obj == NULL) {
                        for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                            line[l][i] = item->color;
                        }
                    }
                    else {
                        for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                            //Skip changing the pixel if it's set to the COLOR_NULL value
                            if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;

                            //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                            line[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                        }
                    }
                    break;
                case 'n': //Deleted item (garbage collector)
                    previousItem->next = item->next;
                    free(item);
                    item = previousItem; //prevent the code from skipping items
            }

            previousItem = item;
            item = item->next; //increment linked list
        }

        //busy wait for currentline to change, then start rendering next line
        while(l == ((currentLine + 1) % 2));
    }
}