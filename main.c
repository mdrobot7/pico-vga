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
    // stdio_init_all();
    // for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
    //     //printf("Waiting for user to open serial...\n");
    //     //sleep_ms(250);
    // }
    //printf("\n");

    Controller P1;
    Controller P2;
    Controller P3;
    Controller P4;
    
    initDisplay(&P1, &P2, &P3, &P4, true);
    while(true){}

    fillScreen(NULL, NULL, 0b11100000);
    drawFilledRectangle(NULL, 0, 0, 10, FRAME_HEIGHT - 1, 0b00011100, 0b00011100);
    drawFilledRectangle(NULL, FRAME_WIDTH - 11, 0, FRAME_WIDTH - 1, FRAME_HEIGHT - 1, 0b11, 0b11);
    while(1);

    RenderQueueItem *temp = drawRectangle(NULL, 150, 150, 350, 250, 0b00011100);
    //updateDisplay();

    for(uint8_t x = 0; x < 4; x++) {
        for(uint8_t y = 0; y < 3; y++) {
            drawFilledRectangle(NULL, 25 + x*100, 25 + y*100, 75 + x*100, 75 + y*100, 0b11100011, 0);
            drawPixel(NULL, 25 + x*100, 25 + y*100, 0b00011100);
            drawFilledCircle(NULL, 50 + x*100, 50 + y*100, 25, 0b11100000, 0);
        }
    //    updateDisplay();
    }
    //updateDisplay();
    temp = drawLine(NULL, 0, 0, FRAME_WIDTH - 1, FRAME_HEIGHT - 1, 0b00000011);
    for(uint64_t w = 0; w < 10000000; w++) asm("nop");

    temp->type = 'h';
    temp->update = true;

    //for(uint64_t w = 0; w < 10000000; w++) asm("nop");

    //updateDisplay();
    
    while(1);
    
    game();
}