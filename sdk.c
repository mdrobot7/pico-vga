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
static uint16_t currentLine = 0; //The current line that the renderer is on

//Constants for the DMA channels
#define line0CtrlDMA 0
#define line0DataDMA 1
#define line1CtrlDMA 2
#define line1DataDMA 3

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
    //Clock configuration -- 120MHz system clock frequency
    clocks_init();
    set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
        if(j % 40 < 20) line[0][j] = 255;
        else line[0][j] = 0;
    }
    line[0][FRAME_WIDTH - 1] = 1;
    for(uint16_t j = FRAME_WIDTH; j < FRAME_FULL_WIDTH; j++) {
        line[0][j] = 0;
    }

    for(uint16_t i = 0; i < FRAME_FULL_WIDTH; i++) {
        line[1][i] = 0;
    }
    line[1][0] = 255;
    line[1][FRAME_WIDTH - 1] = 255;

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
    dma_channel_config dma0Conf = dma_channel_get_default_config(0);
    channel_config_set_transfer_data_size(&dma0Conf, DMA_SIZE_32); //the amount to shift the read position by (4 bytes)
    channel_config_set_dreq(&dma0Conf, 0x3f); //set where the data request will come from
    channel_config_set_read_increment(&dma0Conf, false);
    //channel_config_set_high_priority(&dma0Conf, true);

    static uint32_t linePtr = (uint32_t)line;

    dma_channel_configure(
        0,
        &dma0Conf,
        &dma_hw->ch[1].al3_read_addr_trig, // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        &linePtr,             // Initial read address
        1,     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );

    dma_channel_config dma1Conf = dma_channel_get_default_config(1);
    channel_config_set_transfer_data_size(&dma1Conf, DMA_SIZE_32); //the amount to shift the read position by (4 bytes)
    channel_config_set_read_increment(&dma1Conf, true); //shift the "read position" every read
    channel_config_set_dreq(&dma1Conf, DREQ_PIO0_TX0); //set where the data request will come from
    //channel_config_set_high_priority(&dma1Conf, true);
    channel_config_set_chain_to(&dma1Conf, 2);
    channel_config_set_irq_quiet(&dma1Conf, false);
    
    dma_channel_set_irq0_enabled(1, true);

    dma_channel_configure(
        1,
        &dma1Conf,
        &pio0_hw->txf[0], // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        NULL,             // Initial read address
        (FRAME_FULL_WIDTH/4),     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );

    dma_channel_config dma2Conf = dma_channel_get_default_config(2);
    channel_config_set_transfer_data_size(&dma2Conf, DMA_SIZE_32); //the amount to shift the read position by (4 bytes)
    channel_config_set_dreq(&dma2Conf, 0x3f); //set where the data request will come from
    channel_config_set_read_increment(&dma2Conf, false);
    //channel_config_set_high_priority(&dma0Conf, true);

    static uint32_t line1Ptr = (uint32_t)line[1];

    dma_channel_configure(
        2,
        &dma2Conf,
        &dma_hw->ch[3].al3_read_addr_trig, // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        &line1Ptr,             // Initial read address
        1,     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );

    dma_channel_config dma3Conf = dma_channel_get_default_config(3);
    channel_config_set_transfer_data_size(&dma3Conf, DMA_SIZE_32); //the amount to shift the read position by (4 bytes)
    channel_config_set_read_increment(&dma3Conf, true); //shift the "read position" every read
    channel_config_set_dreq(&dma3Conf, DREQ_PIO0_TX0); //set where the data request will come from
    //channel_config_set_high_priority(&dma1Conf, true);
    channel_config_set_chain_to(&dma3Conf, 0);
    channel_config_set_irq_quiet(&dma3Conf, false); //explicitly false
    
    dma_channel_set_irq0_enabled(3, true);

    dma_channel_configure(
        3,
        &dma3Conf,
        &pio0_hw->txf[0], // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        NULL,             // Initial read address
        (FRAME_FULL_WIDTH/4),     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );

    // Configure the processor to run updateDMA() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateLineCount);
    irq_set_enabled(DMA_IRQ_0, true);


    //printf("%p, %x, %p, %x\n", line, linePtr, &linePtr, dma_hw->ch[0].read_addr);
    //printf("%x, %p\n", dma_hw->ch[0].write_addr, &dma_hw->ch[1].al3_read_addr_trig);
    //printf("%d, %d\n\n", dma_debug_hw->ch[0].tcr, dma_debug_hw->ch[1].tcr);

    dma_start_channel_mask(1u << 0);

    //printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    //printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    //printf("%d, %d, %d, %d, %x\n\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);

    //start all 4 state machines at once, sync clocks
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b0111);

    //for(int i = 0; i < 100; i++) {
    //printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    //}

    //printf("%x\n", dma_hw->ch[1].al3_read_addr_trig);
    //printf("%d, %d\n", dma_debug_hw->ch[0].tcr, dma_debug_hw->ch[1].tcr);
    //printf("%d, %d\n\n\n", dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count);

    //start all 4 state machines at once, sync clocks
    //pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b1111);
}

void initSDK(Controller *c) {
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