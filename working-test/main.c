#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "blink.pio.h"

//PROBLEM:
//The DMA gets all the way through currentFrame, and then runs out of data.
//Set up IRQ (interrupts) and a callback so the CPU then tells it to start reading
//from the beginning of the other array, and flip/flopping back and forth.

const uint8_t frame[2][10][10] = { { {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 1, 0} },
                                     { {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 0, 0},
                                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
                                       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 0, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 1, 0, 0, 0},
                                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 0} } };

int dma_chan;

void swapFrame() {
    static int activeFrame = 0;
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dma_chan;
    // Give the channel a new frame address to read from, and re-trigger it
    dma_channel_set_read_addr(dma_chan, frame[activeFrame], true);

    activeFrame = (activeFrame + 1) % 2; //swap activeFrame between 0 and 1
}

int main() {
    static const uint8_t led_pin = 25;
    static const float pio_freq = 2000;

    // Choose PIO instance (0 or 1)
    PIO pio = pio0;

    // Get first free state machine in PIO 0
    uint sm = pio_claim_unused_sm(pio, true);
    sm = 0;

    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio, &blink_program);

    // Calculate the PIO clock divider
    float div = (float)clock_get_hz(clk_sys) / pio_freq;

    // Initialize the program using the helper function in our .pio file
    blink_program_init(pio, sm, offset, led_pin, div);

    // Start running our PIO program in the state machine
    //pio_sm_set_enabled(pio, sm, true);


    //DMA!
    // Configure a channel to write the same word (32 bits) repeatedly to PIO0
    // SM0's TX FIFO, paced by the data request signal from that peripheral.
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8); //the amount to shift the read position by
    channel_config_set_read_increment(&c, true); //shift the "read position" every read
    channel_config_set_dreq(&c, DREQ_PIO0_TX0); //set where the data request will come from

    dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0], // Write address (only need to set this once)
        frame,             // Don't provide a read address yet
        sizeof(frame[0]), // Transfer an entire frame, then halt and interrupt
        false             // Don't start yet
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_chan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, swapFrame);
    irq_set_enabled(DMA_IRQ_0, true);

    //dma_channel_set_read_addr(dma_chan, frame[0], true);
    swapFrame(); //trigger the DMA manually


    // Do nothing
    while (true) {
        sleep_ms(1000);
    }
}