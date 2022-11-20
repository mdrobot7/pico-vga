#include "../sdk.h"
#include "game.h"

void game()
{
    setRendererState(1);
    printf("%p", drawRectangle(NULL, 100, 100, 500, 500, 255));
    while(1) {
        tight_loop_contents();
    }
}