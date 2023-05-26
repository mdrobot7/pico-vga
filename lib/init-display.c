#include "lib-internal.h"

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "color.pio.h"

static void initSecondCore();
static irq_handler_t updateFramePtr();

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
RenderQueueUID_t uid = 1;
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

static uint8_t color_pio_sm = 0;

static DisplayConfigBaseResolution_t lastResolution = 7; //Random unused enum value

int initDisplay() {
    clocks_init();
    frameWidth = frameSize[displayConfig->baseResolution][0]/displayConfig->resolutionScale;
    frameHeight = frameSize[displayConfig->baseResolution][1]/displayConfig->resolutionScale;
    frameFullWidth = frameSize[displayConfig->baseResolution][2]/displayConfig->resolutionScale;
    frameFullHeight = frameSize[displayConfig->baseResolution][3]/displayConfig->resolutionScale;

    //Override if there are less than 2 interpolated lines
    if(displayConfig->numInterpolatedLines < 2) displayConfig->numInterpolatedLines = 2;

    if(displayConfig->disconnectDisplay || (!displayConfig->disconnectDisplay && lastResolution != displayConfig->baseResolution)) {
        color_pio_sm = pio_claim_unused_sm(displayConfig->pio, true);

        // Add PIO program to PIO instruction memory. SDK will find location and
        // return with the memory offset of the program.
        uint offset = pio_add_program(displayConfig->pio, &color_program);

        gpio_set_function(HSYNC_PIN, GPIO_FUNC_PWM);
        gpio_set_function(VSYNC_PIN, GPIO_FUNC_PWM);
        pwm_config default_pwm_conf = pwm_get_default_config();
        pwm_init(HSYNC_PWM_SLICE, &default_pwm_conf, false); //reset to known state
        pwm_init(VSYNC_PWM_SLICE, &default_pwm_conf, false);

        if(!displayConfig->disconnectDisplay && lastResolution != displayConfig->baseResolution) {
            displayConfig->baseResolution = lastResolution;
        }

        if(displayConfig->baseResolution == RES_800x600 && displayConfig->disconnectDisplay) {
            //120MHz system clock frequency (multiple of 40MHz pixel clock)
            set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

            // Initialize color pio program, but DON'T enable PIO state machine
            //CPU Base clock (after reconfig): 120.0 MHz. 120/3 = 40Mhz pixel clock
            color_program_init(displayConfig->pio, color_pio_sm, offset, COLOR_LSB_PIN, 3*displayConfig->resolutionScale);

            pwm_set_clkdiv(HSYNC_PWM_SLICE, 3.0); //pixel clock
            pwm_set_wrap(HSYNC_PWM_SLICE, 1056 - 1); //full line width - 1
            pwm_set_counter(HSYNC_PWM_SLICE, 128 + 88); //sync pulse + back porch
            pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 128); //sync pulse

            pwm_set_clkdiv(VSYNC_PWM_SLICE, 198.0); //clkdiv maxes out at /256, so this (pixel clock * full line width)/16 = (3*1056)/16, dividing by 16 to compensate
            pwm_set_wrap(VSYNC_PWM_SLICE, (628*16) - 1); //full frame height, adjusted for clkdiv fix
            pwm_set_counter(VSYNC_PWM_SLICE, (4 + 23)*16); //sync pulse + back porch
            pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 4*16); //back porch
        }
        else if(displayConfig->baseResolution == RES_640x480) {
            //125MHz system clock frequency -- possibly bump to 150MHz later
            set_sys_clock_pll(1500000000, 6, 2);

            color_program_init(displayConfig->pio, color_pio_sm, offset, COLOR_LSB_PIN, 5*displayConfig->resolutionScale);

            //Sources for these timings, because they are slightly different (for a 25MHz pixel clock, not 25.175MHz)
            //https://www.eevblog.com/forum/microcontrollers/implementing-a-vga-controller-on-spartan-3-board-with-25-0-mhz-clock/
            //pg 24: https://docs.xilinx.com/v/u/en-US/ug130

            pwm_set_output_polarity(HSYNC_PWM_SLICE, !HSYNC_PWM_CHAN ? true : false, HSYNC_PWM_CHAN ? true : false);
            pwm_set_clkdiv(HSYNC_PWM_SLICE, 5.0);
            pwm_set_wrap(HSYNC_PWM_SLICE, 800 - 1);
            pwm_set_counter(HSYNC_PWM_SLICE, 96 + 48);
            pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 96);

            pwm_set_output_polarity(VSYNC_PWM_SLICE, !VSYNC_PWM_CHAN ? true : false, VSYNC_PWM_CHAN ? true : false);
            pwm_set_clkdiv(VSYNC_PWM_SLICE, 250.0); //see above for reasoning behind this -- (5*800)/16
            pwm_set_wrap(VSYNC_PWM_SLICE, (521*16) - 1);
            pwm_set_counter(VSYNC_PWM_SLICE, (2 + 29)*16);
            pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 2*16);
        }
        else if(displayConfig->baseResolution == RES_1024x768) {
            //150MHz system clock frequency
            set_sys_clock_pll(1500000000, 5, 2);

            color_program_init(displayConfig->pio, color_pio_sm, offset, COLOR_LSB_PIN, 2*displayConfig->resolutionScale);

            pwm_set_output_polarity(HSYNC_PWM_SLICE, !HSYNC_PWM_CHAN ? true : false, HSYNC_PWM_CHAN ? true : false);
            pwm_set_clkdiv(HSYNC_PWM_SLICE, 2.0);
            pwm_set_wrap(HSYNC_PWM_SLICE, 1328 - 1);
            pwm_set_counter(HSYNC_PWM_SLICE, 136 + 144);
            pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 136);

            pwm_set_output_polarity(VSYNC_PWM_SLICE, !VSYNC_PWM_CHAN ? true : false, VSYNC_PWM_CHAN ? true : false);
            pwm_set_clkdiv(VSYNC_PWM_SLICE, 166.0); //see above for reasoning behind this -- (2*800)/16
            pwm_set_wrap(VSYNC_PWM_SLICE, (806*16) - 1);
            pwm_set_counter(VSYNC_PWM_SLICE, (6 + 29)*16);
            pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 6*16);
        }
    }

    lastResolution = displayConfig->baseResolution;

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
    multicore_reset_core1(); //Stop core 1

    //TODO: Init causes hardfaults
    //deInitGarbageCollector();

    deInitLineInterpolation();
    
    dma_hw->abort = (1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA);
    while(dma_hw->abort); //Wait until all channels are stopped

    dma_hw->ch[frameCtrlDMA].al1_ctrl = 0; //clears the CTRL.EN bit (along with the rest of the reg)
    dma_hw->ch[frameDataDMA].al1_ctrl = 0;
    dma_hw->ch[blankDataDMA].al1_ctrl = 0;
    
    dma_unclaim_mask((1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA));
    irq_set_enabled(DMA_IRQ_0, false);

    pio_sm_set_enabled(displayConfig->pio, color_pio_sm, false);
    pio_sm_clear_fifos(displayConfig->pio, color_pio_sm);

    if(displayConfig->disconnectDisplay) {
        pwm_set_enabled(HSYNC_PWM_SLICE, false);
        pwm_set_enabled(VSYNC_PWM_SLICE, false);
    }

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
    irq_set_exclusive_handler(DMA_IRQ_0, (irq_handler_t)updateFramePtr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_hw->multi_channel_trigger = (1 << frameCtrlDMA); //Start!
}

static void initSecondCore() {
    initDMA(); //Must be run here so the IRQ runs on the second core
    //TODO: Causes hardfaults
    //initGarbageCollector();
    if(displayConfig->interpolatedRenderingMode) {
        initLineInterpolation();
    }
    pio_enable_sm_mask_in_sync(displayConfig->pio, 1u << color_pio_sm); //start color state machine
    pwm_set_enabled(HSYNC_PWM_SLICE, true); //start hsync and vsync signals
    pwm_set_enabled(VSYNC_PWM_SLICE, true);

    if(displayConfig->antiAliasing) renderAA();
    else render();
}

//DMA Interrupt Callback
static irq_handler_t updateFramePtr() {
    dma_hw->ints0 = 1u << frameCtrlDMA; //Clear interrupt

    //Grab the element in frameReadAddr that frameCtrlDMA just wrote (hence the -1) and see if it was an
    //interpolated line. If so, incrememt the interpolated line counter and flag the renderer so it can start
    //re-rendering it.
    if(displayConfig->interpolatedRenderingMode && 
       (((uint8_t *)dma_hw->ch[frameCtrlDMA].read_addr) - 1) < frameBufferStart && 
       (((uint8_t *)dma_hw->ch[frameCtrlDMA].read_addr) - 1) >= (blankLineStart + frameWidth)) {
        interpolatedLine = (interpolatedLine + 1) % displayConfig->numInterpolatedLines;
        irq_set_pending(lineInterpolationIRQ);
    }

    if(dma_hw->ch[frameCtrlDMA].read_addr >= (io_rw_32) &frameReadAddrStart[frameFullHeight*displayConfig->resolutionScale]) {
        dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddrStart;
    }
}