#include "sdk.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "color.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"

/*
Ideas:
- GIMP can output C header, raw binary, etc files, make that the sprite format
*/

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

uint8_t * frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER];
uint8_t BLANK[FRAME_WIDTH];

//Prototypes, for functions not defined in sdk.h (NOT for use by the user)
static void initSecondCore();
static void updateFramePtr();
static void drawLogo();


int initDisplay(bool autoRenderEn, bool antiAliasingEn) {
    //Clock configuration -- 120MHz system clock frequency
    clocks_init();
    set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    autoRender = (uint8_t)autoRenderEn;
    antiAliasing = (uint8_t)antiAliasingEn;

    initController();

    multicore_launch_core1(initSecondCore);
    while(multicore_fifo_pop_blocking() != 13); //busy wait while the core is initializing
    
    busyWait(10000000); //wait to make sure everything is stable

    drawLogo();
    busyWait(10000000);
    clearScreen();
    
    return 0;
}

static void initDMA() {
    for(uint16_t i = 0; i < FRAME_FULL_HEIGHT*FRAME_SCALER; i++) {
        if(i >= FRAME_HEIGHT*FRAME_SCALER) frameReadAddr[i] = BLANK;
        else frameReadAddr[i] = (uint8_t *) frame[i/FRAME_SCALER];
    }

    dma_claim_mask((1 << frameCtrlDMA) | (1 << frameDataDMA) | (1 << blankDataDMA)); //mark channels as used in the SDK

    dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddr;
    dma_hw->ch[frameCtrlDMA].write_addr = (io_rw_32) &dma_hw->ch[frameDataDMA].al3_read_addr_trig;
    dma_hw->ch[frameCtrlDMA].transfer_count = 1;
    dma_hw->ch[frameCtrlDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_FORCE << SDK_DMA_CTRL_TREQ_SEL);

    dma_hw->ch[frameDataDMA].read_addr = (io_rw_32) NULL;
    dma_hw->ch[frameDataDMA].write_addr = (io_rw_32) &pio0_hw->txf[0];
    dma_hw->ch[frameDataDMA].transfer_count = FRAME_WIDTH/4;
    dma_hw->ch[frameDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (1 << SDK_DMA_CTRL_INCR_READ) |
                                        (blankDataDMA << SDK_DMA_CTRL_CHAIN_TO) | (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL) |
                                        (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->ch[blankDataDMA].read_addr = (io_rw_32) BLANK;
    dma_hw->ch[blankDataDMA].write_addr = (io_rw_32) &pio0_hw->txf[0];
    dma_hw->ch[blankDataDMA].transfer_count = (FRAME_FULL_WIDTH - FRAME_WIDTH)/4;
    dma_hw->ch[blankDataDMA].al1_ctrl = (1 << SDK_DMA_CTRL_EN)                  | (1 << SDK_DMA_CTRL_HIGH_PRIORITY) |
                                        (DMA_SIZE_32 << SDK_DMA_CTRL_DATA_SIZE) | (frameCtrlDMA << SDK_DMA_CTRL_CHAIN_TO) |
                                        (DREQ_PIO0_TX0 << SDK_DMA_CTRL_TREQ_SEL)| (1 << SDK_DMA_CTRL_IRQ_QUIET);

    dma_hw->inte0 = (1 << frameCtrlDMA);

    // Configure the processor to update the line count when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateFramePtr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_hw->multi_channel_trigger = (1 << frameCtrlDMA); //Start!
}

static void initPIO() {
    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio0, &color_program);

    // Initialize color pio program, but DON'T enable PIO state machine
    color_program_init(pio0, 0, offset, 0, FRAME_SCALER);

    offset = pio_add_program(pio0, &hsync_program);
    hsync_program_init(pio0, 1, offset, HSYNC_PIN);

    offset = pio_add_program(pio0, &vsync_program);
    vsync_program_init(pio0, 2, offset, VSYNC_PIN);
}

static void initSecondCore() {
    initPIO();
    initDMA();
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b0111); //start all 4 state machines

    render();
}

//DMA Interrupt Callback
static void updateFramePtr() {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << frameCtrlDMA;

    if(dma_hw->ch[frameCtrlDMA].read_addr >= (io_rw_32) &frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER]) {
        dma_hw->ch[frameCtrlDMA].read_addr = (io_rw_32) frameReadAddr;
    }
}

static void drawLogo() {
    drawText(NULL, 50, 250, 350, "pico-vga -- Michael Drobot, 2023", COLOR_WHITE, COLOR_BLACK, false, 0);
    drawText(NULL, 50, 258, 350, "   Licensed under GNU GPL v3.", COLOR_WHITE, COLOR_BLACK, false, 0);
}