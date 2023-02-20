#include "lib-internal.h"

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

static void initSecondCore();
static void updateFramePtr();

//Widths and heights for the three base resolutions
const uint16_t frameSize[3][4] = {{800, 600, 1056, 628},
                                  {640, 480, 800, 525},
                                  {1024, 768, 1344, 806}};

//Global width and height variables. Set in the init -- faster than a getter function.
uint16_t frameWidth = 0;
uint16_t frameHeight = 0;
uint16_t frameFullWidth = 0;
uint16_t frameFullHeight = 0;

int initDisplay() {
    clocks_init();
    frameWidth = frameSize[displayConfig->baseResolution][0]/displayConfig->resolutionScale;
    frameHeight = frameSize[displayConfig->baseResolution][1]/displayConfig->resolutionScale;
    frameFullWidth = frameSize[displayConfig->baseResolution][2]/displayConfig->resolutionScale;
    frameFullHeight = frameSize[displayConfig->baseResolution][3]/displayConfig->resolutionScale;

    //Override if there are less than 2 interpolated lines
    if(displayConfig->numInterpolatedLines < 2) displayConfig->numInterpolatedLines = 2;

    if(displayConfig->baseResolution == RES_800x600) {
        //120MHz system clock frequency (multiple of 40MHz pixel clock)
        set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

        // Add PIO program to PIO instruction memory. SDK will find location and
        // return with the memory offset of the program.
        uint offset = pio_add_program(pio0, &color_program);

        // Initialize color pio program, but DON'T enable PIO state machine
        color_program_init(pio0, 0, offset, 0, displayConfig->resolutionScale);

        offset = pio_add_program(pio0, &hsync_program);
        hsync_program_init(pio0, 1, offset, HSYNC_PIN);

        offset = pio_add_program(pio0, &vsync_program);
        vsync_program_init(pio0, 2, offset, VSYNC_PIN);
    }
    else if(displayConfig->baseResolution == RES_640x480) {
        //configure clocks for 640x480 resolution
    }
    else if(displayConfig->baseResolution == RES_1024x768) {
        //130MHz system clock frequency (multiple of 65MHz pixel clock)
    }

    BLANK = (uint8_t *) malloc(frameWidth);
    frameReadAddr = (uint8_t **) malloc(frameFullHeight*displayConfig->resolutionScale*sizeof(uint8_t *));
    uint32_t numBufferedLines = 0;

    //If the specified frame buffer size is larger than a single frame (say 400x300 = 120kB), override it and set (to 120kB for example)
    if(displayConfig->frameBufferSizeKB >= frameWidth*frameHeight) {
        frame = (uint8_t *) malloc(frameWidth*frameHeight);
    }
    //Cap frame buffer size at 256kB
    else if(displayConfig->frameBufferSizeKB > 256) {
        numBufferedLines = 256000/frameWidth;
        frame = (uint8_t *) malloc(256000);
        line = (uint8_t *) malloc(displayConfig->numInterpolatedLines*frameWidth);
    }
    else {
        numBufferedLines = displayConfig->frameBufferSizeKB/frameWidth;
        frame = (uint8_t *) malloc(numBufferedLines*frameWidth);
        line = (uint8_t *) malloc(displayConfig->numInterpolatedLines*frameWidth);
    }

    //If any of these pointers are still NULL, then there isn't enough memory to allocate. Return an error.
    if(frame == NULL || frameReadAddr == NULL || BLANK == NULL || (numBufferedLines != 0 && line == NULL)) {
        free((void *)frame);
        free((void *)frameReadAddr);
        free((void *)BLANK);
        free((void *)line);
        return 1;
    }

    if(numBufferedLines != 0) {
        /*
            Even distribution algorithm:
        - Finds the distance between each of the interpolated lines (segmentLen)
        - There will always be spares, since the number of interpolated lines will almost never perfectly
          divide into the number of total lines. This is numSpares.
        - Instead of putting an interpolated line every segmentLen and putting all of the spares at the end
          (making the CPU work harder in the beginning of the frame) this adds the 1 element to numSpares
          number of segments.
        - This reddit post explains it: https://www.reddit.com/r/learnprogramming/comments/31ns44/splitting_the_contents_of_an_array_into_as_evenly/
        */
        uint16_t segmentLen = frameHeight/(frameHeight - numBufferedLines);
        uint16_t numSpares = frameHeight % (frameHeight - numBufferedLines);
        for(int i = 0, j = 0, k = 0; i < frameHeight*displayConfig->resolutionScale; i++) {
            if(numSpares != 0 && i % (segmentLen + 1) == 0) {
                frameReadAddr[i] = &line[k];
                k = (k + 1) % displayConfig->numInterpolatedLines;
                numSpares--;
            }
            else if(i % segmentLen == 0) {
                frameReadAddr[i] = &line[k];
                k = (k + 1) % displayConfig->numInterpolatedLines;
            }
            else {
                frameReadAddr[i] = (uint8_t *)&frame[j/displayConfig->resolutionScale];
                j++;
            }
        }
    }
    else {
        for(int i = 0; i < frameHeight*displayConfig->resolutionScale; i++) {
            frameReadAddr[i] = (uint8_t *)&frame[i/displayConfig->resolutionScale];
        }
    }
    for(int i = frameHeight*displayConfig->resolutionScale; i < frameFullHeight*displayConfig->resolutionScale; i++) {
        frameReadAddr[i] = BLANK;
    }

    multicore_launch_core1(initSecondCore);
    while(multicore_fifo_pop_blocking() != 13); //busy wait while the core is initializing
}

static void initDMA() {
    dma_claim_mask((1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA)); //mark channels as used in the SDK

    dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddr;
    dma_hw->ch[frameCtrlDMA].write_addr = (io_rw_32) &dma_hw->ch[frameDataDMA].al3_read_addr_trig;
    dma_hw->ch[frameCtrlDMA].transfer_count = 1;
    dma_hw->ch[frameCtrlDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL);

    dma_hw->ch[frameDataDMA].read_addr = (io_rw_32) NULL;
    dma_hw->ch[frameDataDMA].write_addr = (io_rw_32) &pio0_hw->txf[0];
    dma_hw->ch[frameDataDMA].transfer_count = frameWidth/4;
    dma_hw->ch[frameDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (blankDataDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL) |
                                        (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->ch[blankDataDMA].read_addr = (io_rw_32) BLANK;
    dma_hw->ch[blankDataDMA].write_addr = (io_rw_32) &pio0_hw->txf[0];
    dma_hw->ch[blankDataDMA].transfer_count = (frameFullWidth - frameWidth)/4;
    dma_hw->ch[blankDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) |
                                        (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL)| (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->inte0 = (1 << frameCtrlDMA);

    // Configure the processor to update the line count when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateFramePtr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_hw->multi_channel_trigger = (1 << frameCtrlDMA); //Start!
}

static void initSecondCore() {
    initDMA(); //Must be run here so the IRQ runs on the second core
    pio_enable_sm_mask_in_sync(pio0, 0b0111); //start all 4 state machines

    render();
}

//DMA Interrupt Callback
static void updateFramePtr() {
    /*// Clear the interrupt request.
    dma_hw->ints0 = 1u << frameCtrlDMA;

    if(dma_hw->ch[frameCtrlDMA].read_addr >= (io_rw_32) &frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER]) {
        dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddr;
    }*/
}