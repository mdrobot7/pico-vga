#include "main.h"
#include "game/game.h"

/*
        PINOUT (OLD)
GPIO0 - Color MSB (RED)
GPIO 1-6 - Color
GPIO7 - Color LSB (BLUE)
GPIO8 - HSync
GPIO9 - VSync
*/

/*
        PINOUT (NEW)
GPIO0 - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7 - Color MSB (RED 1)
GPIO8 - HSync
GPIO9 - VSync
GPIO10 - DPad Up
GPIO11 - DPad Down
GPIO12 - DPad Left
GPIO13 - DPad Right
GPIO14 - A
GPIO15 - B
GPIO16 - SD Card MISO (RX)
GPIO17 - SD Card Chip Select
GPIO18 - SD Card CLK
GPIO19 - SD Card MOSI (TX)
GPIO20 - X
GPIO21 - Y
GPIO22 - Controller Select A

GPIO26 - Controller Select B
GPIO27 - Audio L
GPIO28 - Audio R
*/


//NEXT:
// - Timing for state machines, clock divider?
// - TEST! cut up a vga cable, tie it to a monitor, actually display an image


/* Timing guide:
- HSync and VSync are started, and then left alone. They just do their own thing.
- Color needs to be stopped during the sync portions of the signal, otherwise it will keep outputting color
  data when it can't be seen by the user.
- It's controlled by 2 things: State machine enable/disable, and whether or not it has data (aka DMA enable/disable)
- Currently, when it reaches the end of a line, it will keep outputting blank data from the frame array to fill
  the space. When it reaches the end of the frame, it loads DMA with the first line of the next frame, but DOES NOT
  start it.
- colorHandler.pio is responsible for waiting the entire duration of the frame, and then when it reaches the end,
  raising an IRQ to tell the CPU to restart DMA and feed the color PIO with data.

*/


uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];

int dmaChan;

static void updateDMA() {
    static bool currentLineDoubled = false;
    static uint16_t currentLine = 0;
    bool dmaStart = true;

    if(currentLineDoubled) { //handle line doubling
        currentLine++; 
        currentLineDoubled = false;
    }
    else currentLineDoubled = true;

    if(currentLine == FRAME_HEIGHT) { //if it reaches the end of the frame, reload DMA, but DON'T start yet
        dmaStart = false;
        currentLine = 0;
    }

    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dmaChan;
    // Give the channel a new frame address to read from, and re-trigger it
    dma_channel_set_read_addr(dmaChan, frame[currentLine], dmaStart);
}

static void restartColor() {
    pio0_hw->irq = 0b1; //clear interrupt 0 (set bit 0)
    dma_channel_start(dmaChan); //restart DMA -- the color PIO will stop if it runs out of data, this is how
                                //I'm stopping it during sync periods
    printf("x");
}

int main() {
    //Clock configuration
    clocks_init();
    set_sys_clock_pll(1440000000, 6, 2); //VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    stdio_init_all();
    for(uint8_t i = 0; i < 32; i++) { //8 seconds to open serial communication
        printf("Waiting for user to open serial...");
        printf("%d\n", clock_get_hz(clk_sys));
        //sleep_ms(250);
    }
    
    for(uint16_t i = 0; i < FRAME_HEIGHT; i++) {
        for(uint16_t j = 0; j < FRAME_WIDTH; j++) {
            if(j % 2 == 0) frame[i][j] = 255; //fill the frame array
            else frame[i][j] = 0;
        }
    }

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

    offset = pio_add_program(pio0, &colorHandler_program);
    colorHandler_program_init(pio0, 3, offset);

    pio_set_irq0_source_enabled(pio0, pis_interrupt0, true); //pipe pio0 interrupt 0 to the system
    irq_set_exclusive_handler(PIO0_IRQ_0, restartColor); //tie pio0 irq channel 0 to restartColor()
    irq_set_enabled(PIO0_IRQ_0, true); //enable irq

    //DMA Configuration

    // Configure a channel to write the same word (32 bits) repeatedly to PIO0
    // SM0's TX FIFO, paced by the data request signal from that peripheral.
    dmaChan = dma_claim_unused_channel(true);
    dma_channel_config dmaConf = dma_channel_get_default_config(dmaChan);
    channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_32); //the amount to shift the read position by (4 bytes)
    channel_config_set_read_increment(&dmaConf, true); //shift the "read position" every read
    channel_config_set_dreq(&dmaConf, DREQ_PIO0_TX0); //set where the data request will come from

    dma_channel_configure(
        dmaChan,
        &dmaConf,
        &pio0_hw->txf[0], // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        frame,             // Initial read address
        FRAME_WIDTH/4,     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dmaChan, true);

    // Configure the processor to run updateDMA() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateDMA);
    irq_set_enabled(DMA_IRQ_0, true);

    //start all 4 state machines at once, sync clocks
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b1111);

    updateDMA(); //trigger the DMA manually

    gpio_init(25);
    gpio_set_dir(25, true);
    while(true)
    {
        printf("asdf\n");
        gpio_put(25, true);
        sleep_ms(250);
        gpio_put(25, false);
        sleep_ms(250);
    } //lock the processor into inf loop
    game();
}