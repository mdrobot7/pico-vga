#include "lib-internal.h"

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

uint8_t buffer[PICO_VGA_MAX_MEMORY_BYTES];

//Next unique ID to assign to a render queue item, used so items can be modified after initialization
uint32_t uid = 1;
RenderQueueItem_t * lastItem = NULL;

uint8_t * renderQueueStart = NULL;
uint8_t * renderQueueEnd = NULL;
uint8_t * sdStart = NULL;
uint8_t * sdEnd = NULL;
uint8_t * audioStart = NULL;
uint8_t * audioEnd = NULL;
uint8_t * controllerStart = NULL;
uint8_t * controllerEnd = NULL;
uint8_t * frameReadAddrStart = NULL;
uint8_t * frameReadAddrEnd = NULL;
uint8_t * blankLineStart = NULL;
uint8_t * interpolatedLineStart = NULL;
uint8_t * frameBufferStart = NULL;
uint8_t * frameBufferEnd = NULL;

uint8_t frameCtrlDMA = 0;
uint8_t frameDataDMA = 0;
uint8_t blankDataDMA = 0;

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
        //CPU Base clock (after reconfig): 120.0 MHz. 120/3 = 40Mhz pixel clock
        color_program_init(pio0, 0, offset, 0, 3*displayConfig->resolutionScale);

        offset = pio_add_program(pio0, &hsync_800x600_program);
        hsync_800x600_program_init(pio0, 1, offset, HSYNC_PIN);

        offset = pio_add_program(pio0, &vsync_800x600_program);
        vsync_800x600_program_init(pio0, 2, offset, VSYNC_PIN);

        pio_claim_sm_mask(pio0, 0b0111);
    }
    else if(displayConfig->baseResolution == RES_640x480) {
        //configure clocks for 640x480 resolution
    }
    else if(displayConfig->baseResolution == RES_1024x768) {
        //130MHz system clock frequency (multiple of 65MHz pixel clock)
    }

    //Shift the frame left or right on the screen, configurable at runtime.
    pio_sm_put_blocking(pio0, 0, (uint32_t)(displayConfig->colorDelayCycles/32));
    pio_sm_exec_wait_blocking(pio0, 0, pio_encode_pull(false, false));
    pio_sm_exec_wait_blocking(pio0, 0, pio_encode_out(pio_x, 32));
    pio_sm_put_blocking(pio0, 0, (uint32_t)(displayConfig->colorDelayCycles % 32));
    pio_sm_exec_wait_blocking(pio0, 0, pio_encode_pull(false, false));
    pio_sm_exec_wait_blocking(pio0, 0, pio_encode_out(pio_y, 32));

    //Cap the frame buffer size at the size of one frame or 256kB, whichever is smaller
    //Fail if the frame buffer is too small
    if(displayConfig->frameBufferSizeBytes < frameWidth*5) return 1;
    if(displayConfig->frameBufferSizeBytes >= frameWidth*frameHeight && frameWidth*frameHeight < PICO_VGA_MAX_MEMORY_BYTES) {
        displayConfig->frameBufferSizeBytes = frameWidth*frameHeight;
    }
    else if(displayConfig->frameBufferSizeBytes >= 256000 && displayConfig->frameBufferSizeBytes < PICO_VGA_MAX_MEMORY_BYTES) {
        displayConfig->frameBufferSizeBytes = 256000;
    }
    else if(displayConfig->frameBufferSizeBytes > 0.75*PICO_VGA_MAX_MEMORY_BYTES) {
        displayConfig->frameBufferSizeBytes = (uint32_t)(0.75*PICO_VGA_MAX_MEMORY_BYTES);
    }

    uint32_t numBufferedLines = displayConfig->frameBufferSizeBytes/frameWidth;
    frameBufferEnd = buffer + PICO_VGA_MAX_MEMORY_BYTES;

    //If no interpolation is required
    if(numBufferedLines == frameHeight) {
        frameBufferStart = frameBufferEnd - frameHeight*frameWidth;
        interpolatedLineStart = frameBufferStart;
        blankLineStart = frameBufferStart - frameWidth;
    }
    else {
        frameBufferStart = frameBufferEnd - numBufferedLines*frameWidth;
        interpolatedLineStart = frameBufferStart - displayConfig->numInterpolatedLines*frameWidth;
        blankLineStart = interpolatedLineStart - frameWidth;
    }

    frameReadAddrStart = blankLineStart - frameFullHeight*displayConfig->resolutionScale*sizeof(uint8_t *);
    frameReadAddrEnd = blankLineStart;

    //If the allocation ran out of space (started writing to other memory), then fail
    if(frameReadAddrStart < buffer) return 1;

    if(numBufferedLines != frameHeight) {
        /*
            Even distribution algorithm:
        - Finds the distance between each of the interpolated lines (segmentLen)
        - There will always be spares, since the number of interpolated lines will almost never perfectly
          divide into the number of total lines. This is numSpares.
        - Instead of putting an interpolated line every segmentLen and putting all of the spares at the end
          (making the CPU work harder in the beginning of the frame) this adds 1 element to numSpares
          number of segments.
        - This algorithm interpolates a line every (segmentLen + 1) lines until it runs out of spares, then
          interpolates every segmentLen lines.
        - This reddit post explains it: https://www.reddit.com/r/learnprogramming/comments/31ns44/splitting_the_contents_of_an_array_into_as_evenly/
        - i = line in frame, j = buffer line counter, k = interpolated line counter, l = frameReadAddr index counter
        */
        uint16_t segmentLen = frameHeight/(frameHeight - numBufferedLines);
        uint16_t numSpares = frameHeight % (frameHeight - numBufferedLines);
        for(int i = 0, j = 0, k = 0; i < frameHeight; i++) {
            if(numSpares != 0 && i % (segmentLen + 1) == 0) {
                for(int l = 0; l < displayConfig->resolutionScale; l++) {
                    ((uint8_t **)frameReadAddrStart)[(i*displayConfig->resolutionScale) + l] = interpolatedLineStart + k*frameWidth;
                }
                k = (k + 1) % displayConfig->numInterpolatedLines;
                numSpares--;
            }
            else if(numSpares == 0 && i % segmentLen == 0) {
                for(int l = 0; l < displayConfig->resolutionScale; l++) {
                    ((uint8_t **)frameReadAddrStart)[(i*displayConfig->resolutionScale) + l] = interpolatedLineStart + k*frameWidth;
                }
                k = (k + 1) % displayConfig->numInterpolatedLines;
            }
            else {
                //If this line isn't interpolated, grab the next buffered line from the framebuffer
                for(int l = 0; l < displayConfig->resolutionScale; l++) {
                    ((uint8_t **)frameReadAddrStart)[(i*displayConfig->resolutionScale) + l] = (uint8_t *)(frameBufferStart + frameWidth*j);
                }
                j++;
            }
        }
    }
    else {
        for(int i = 0; i < frameHeight*displayConfig->resolutionScale; i++) {
            ((uint8_t **)frameReadAddrStart)[i] = frameBufferStart + frameWidth*(i/displayConfig->resolutionScale);
        }
    }
    
    for(int i = frameHeight*displayConfig->resolutionScale; i < frameFullHeight*displayConfig->resolutionScale; i++) {
        ((uint8_t **)frameReadAddrStart)[i] = blankLineStart;
    }
    //Hacky fix: Since the true height of 640x480 is 525 lines, integer division becomes an issue. This is the fix
    if(displayConfig->baseResolution == RES_640x480) {
        ((uint8_t **)frameReadAddrStart)[524] = blankLineStart;
    }

    renderQueueStart = buffer;
    if(displayConfig->renderQueueSizeBytes != 0) {
        renderQueueEnd = buffer + displayConfig->renderQueueSizeBytes;
    }
    else {
        renderQueueEnd = NULL;
    }
    lastItem = (RenderQueueItem_t *) buffer;

    //Fail if there isn't enough memory (started running into frame buffer space)
    if(renderQueueEnd > frameReadAddrStart) return 1;

    multicore_launch_core1(initSecondCore);
    while(multicore_fifo_pop_blocking() != 13); //busy wait while the core is initializing
    return 0;
}

int deInitDisplay() {
    //Stop core 1

    //TODO: Init causes hardfaults
    //deInitGarbageCollector();
    
    dma_hw->abort = (1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA);
    while(dma_hw->abort); //Wait until all channels are stopped

    dma_hw->ch[frameCtrlDMA].al1_ctrl = 0; //clears the CTRL.EN bit (along with the rest of the reg)
    dma_hw->ch[frameDataDMA].al1_ctrl = 0;
    dma_hw->ch[blankDataDMA].al1_ctrl = 0;
    
    dma_unclaim_mask((1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA));
    irq_set_enabled(DMA_IRQ_0, false);

    pio_set_sm_mask_enabled(pio0, 0b0111, false);
    pio_sm_clear_fifos(pio0, 0);
    pio_sm_clear_fifos(pio0, 1);
    pio_sm_clear_fifos(pio0, 2);
    pio_clear_instruction_memory(pio0);

    if(displayConfig->clearRenderQueueOnDeInit) {
        renderQueueStart = NULL;
        renderQueueEnd = NULL;
        lastItem = NULL;
        uid = 1;
    }

    frameReadAddrStart = NULL;
    frameReadAddrEnd = NULL;
    blankLineStart = NULL;
    interpolatedLineStart = NULL;
    frameBufferStart = NULL;
    frameBufferEnd = NULL;

    return 0;
}

static void initDMA() {
    frameCtrlDMA = dma_claim_unused_channel(true);
    frameDataDMA = dma_claim_unused_channel(true);
    blankDataDMA = dma_claim_unused_channel(true);

    dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddrStart;
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

    dma_hw->ch[blankDataDMA].read_addr = (io_rw_32) blankLineStart;
    dma_hw->ch[blankDataDMA].write_addr = (io_rw_32) &pio0_hw->txf[0];
    dma_hw->ch[blankDataDMA].transfer_count = (frameFullWidth - frameWidth)/4;
    dma_hw->ch[blankDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) |
                                        (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL)| (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->inte0 = (1 << frameCtrlDMA);
    irq_set_priority(DMA_IRQ_0, 0); //Set the DMA interrupt to the highest priority

    // Configure the processor to update the line count when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateFramePtr);
    irq_set_enabled(DMA_IRQ_0, true);

    //TODO: Causes hardfaults
    //initGarbageCollector();

    dma_hw->multi_channel_trigger = (1 << frameCtrlDMA); //Start!
}

static void initSecondCore() {
    initDMA(); //Must be run here so the IRQ runs on the second core
    pio_enable_sm_mask_in_sync(pio0, 0b0111); //start all 3 state machines

    render();
}

//DMA Interrupt Callback
static void updateFramePtr() {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << frameCtrlDMA;

    //Grab the element in frameReadAddr that frameCtrlDMA just wrote (hence the -1) and see if it was an
    //interpolated line. If so, incrememt the interpolated line counter and flag the renderer so it can start
    //re-rendering it.
    if((((uint8_t *)dma_hw->ch[frameCtrlDMA].read_addr) - 1) < frameBufferStart && 
       (((uint8_t *)dma_hw->ch[frameCtrlDMA].read_addr) - 1) >= (blankLineStart + frameWidth)) {
        interpolatedLine = (interpolatedLine + 1) % displayConfig->numInterpolatedLines;
        startInterpolation = true;
    }

    if(dma_hw->ch[frameCtrlDMA].read_addr >= (io_rw_32) &frameReadAddrStart[frameFullHeight*displayConfig->resolutionScale]) {
        dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddrStart;
    }
}