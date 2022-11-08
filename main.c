#include "main.h"
#include "game/game.h"

/*
        PINOUT
GPIO0 - Color MSB (RED)
GPIO 1-6 - Color
GPIO7 - Color LSB (BLUE)
GPIO8 - HSync
GPIO9 - VSync
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


uint8_t frame[FRAME_HEIGHT][FRAME_FULL_WIDTH];
uint8_t line[2][FRAME_FULL_WIDTH*2];

int dmaChan;
bool dmaStart = true;
uint16_t currentLine = 0;

static void updateDMA() {
    return;
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
    printf("\n");

    for(uint16_t j = 0; j < FRAME_WIDTH*2; j++) {
        if(j % 40 < 20) line[0][j] = 255;
        else line[0][j] = 0;
    }
    line[0][FRAME_WIDTH*2 - 1] = 1;
    for(uint16_t j = FRAME_WIDTH*2; j < FRAME_FULL_WIDTH*2; j++) {
        line[0][j] = 0;
    }

    for(uint16_t i = 0; i < FRAME_FULL_WIDTH*2; i++) {
        line[1][i] = 0;
    }
    line[1][0] = 255;
    line[1][FRAME_WIDTH*2 - 1] = 255;
    

    //PIO Configuration

    // Add PIO program to PIO instruction memory. SDK will find location and
    // return with the memory offset of the program.
    uint offset = pio_add_program(pio0, &color_program);

    // Initialize color pio program, but DON'T enable PIO state machine
    color_program_init(pio0, 0, offset, 0);

    offset = pio_add_program(pio0, &hsync_program);
    hsync_program_init(pio0, 1, offset, 8);

    offset = pio_add_program(pio0, &vsync_program);
    vsync_program_init(pio0, 2, offset, 9);


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
    channel_config_set_chain_to(&dma1Conf, 0);
    channel_config_set_irq_quiet(&dma1Conf, true);

    dma_channel_configure(
        1,
        &dma1Conf,
        &pio0_hw->txf[0], // Write address (only need to set this once) -- TX FIFO of PIO 0, state machine 0 (color state machine)
        NULL,             // Initial read address
        ((FRAME_FULL_WIDTH*2)/4)*2,     // Transfer a line, 4 bytes (32 bits) at a time, then halt and interrupt
        false             // Don't start yet
    );


    printf("%p, %x, %p, %x\n", line, linePtr, &linePtr, dma_hw->ch[0].read_addr);
    printf("%x, %p\n", dma_hw->ch[0].write_addr, &dma_hw->ch[1].al3_read_addr_trig);
    printf("%d, %d\n\n", dma_debug_hw->ch[0].tcr, dma_debug_hw->ch[1].tcr);

    dma_start_channel_mask(1u << 0);

    printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    printf("%d, %d, %d, %d, %x\n\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);

    //start all 4 state machines at once, sync clocks
    pio_enable_sm_mask_in_sync(pio0, (unsigned int)0b0111);

    for(int i = 0; i < 100; i++) {
    printf("%d, %d, %d, %d, %x\n", dma_hw->ch[0].al1_ctrl & (1u << 24), dma_hw->ch[1].al1_ctrl & (1u << 24), dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count, dma_hw->ch[1].al3_read_addr_trig);
    }

    printf("%x\n", dma_hw->ch[1].al3_read_addr_trig);
    printf("%d, %d\n", dma_debug_hw->ch[0].tcr, dma_debug_hw->ch[1].tcr);
    printf("%d, %d\n", dma_hw->ch[0].transfer_count, dma_hw->ch[1].transfer_count);

    gpio_init(25);
    gpio_set_dir(25, true);
    while(true)
    {
        printf("asdf %d %d %d %d\n", dma_channel_is_busy(0), dma_channel_is_busy(1), dma_channel_is_busy(2), dma_channel_is_busy(3));
        gpio_put(25, true);
        sleep_ms(250);
        gpio_put(25, false);
        sleep_ms(250);
    } //lock the processor into inf loop
    game();
}