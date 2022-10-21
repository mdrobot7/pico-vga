#include "main.h"
#include "game/game.h"

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

int main() {
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

    initPIO();

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