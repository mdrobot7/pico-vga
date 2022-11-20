#include "sdk.h"
#include "main.h"
#include "game/game.h"

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

int main() {
    stdio_init_all();
    for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
        printf("Waiting for user to open serial...\n");
        sleep_ms(250);
    }
    printf("\n");
    
    initSDK(&controller);
    
    game();
}