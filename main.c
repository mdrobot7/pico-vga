#include "main.h"
#include "game.h"

//PROBLEM:
//The DMA gets all the way through currentFrame, and then runs out of data.
//Set up IRQ (interrupts) and a callback so the CPU then tells it to start reading
//from the beginning of the other array, and flip/flopping back and forth.

uint8_t frame[2][240][320];

int dmaChan;

static void updateDMA() {
    static uint8_t activeFrame = 0;
    static bool currentLineDoubled = false;
    static uint8_t currentLine = 0;

    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dmaChan;
    // Give the channel a new frame address to read from, and re-trigger it
    dma_channel_set_read_addr(dmaChan, frame[activeFrame][currentLine], true);

    activeFrame = (activeFrame + 1) % 2; //swap activeFrame between 0 and 1
    if(currentLineDoubled) { //handle line doubling
        currentLine++; 
        currentLineDoubled = false;
    }
    else currentLineDoubled = true;
}

int main() {
    for(uint8_t i = 0; i < 2; i++) {
        for(uint16_t j = 0; j < 320; j++) {
            for(uint8_t k = 0; k < 240; k++) {
                frame[i][j][k] = 0; //fill the frame array
            }
        }
    }

    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio0, &color_program);

    // Initialize color pio program, start PIO state machine
    color_program_init(pio0, 0, offset, 0);

    offset = pio_add_program(pio0, &vsync_program);
    vsync_program_init(pio0, 1, offset, 8);

    offset = pio_add_program(pio0, &hsync_program);
    hsync_program_init(pio0, 2, offset, 9);


    //DMA!
    // Configure a channel to write the same word (32 bits) repeatedly to PIO0
    // SM0's TX FIFO, paced by the data request signal from that peripheral.
    dmaChan = dma_claim_unused_channel(true);
    dma_channel_config dmaConf = dma_channel_get_default_config(dmaChan);
    channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_32); //the amount to shift the read position by
    channel_config_set_read_increment(&dmaConf, true); //shift the "read position" every read
    channel_config_set_dreq(&dmaConf, DREQ_PIO0_TX0); //set where the data request will come from

    dma_channel_configure(
        dmaChan,
        &dmaConf,
        &pio0_hw->txf[0], // Write address (only need to set this once)
        frame,             // Don't provide a read address yet
        sizeof(frame[0][0]), // Transfer a line, then halt and interrupt
        false             // Don't start yet
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dmaChan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, updateDMA);
    irq_set_enabled(DMA_IRQ_0, true);

    //dma_channel_set_read_addr(dma_chan, frame[0], true);
    updateDMA(); //trigger the DMA manually

    game();
}