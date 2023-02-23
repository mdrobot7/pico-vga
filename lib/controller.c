#include "lib-internal.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"

//static bool updateAllControllers(struct repeating_timer *t);

int initController() {
    //static struct repeating_timer controllerTimer;
    //add_repeating_timer_ms(1, updateAllControllers, NULL, &controllerTimer);

    return 0;
}

int deInitController() {
    return 0;
}