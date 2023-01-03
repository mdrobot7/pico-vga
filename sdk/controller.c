#include "sdk.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"

volatile Controller C1;
volatile Controller C2;
volatile Controller C3;
volatile Controller C4;

static bool updateAllControllers(struct repeating_timer *t);

void initController() {
    sio_hw->gpio_oe_set = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Enable outputs on pins 22 and 26
    static struct repeating_timer controllerTimer;
    add_repeating_timer_ms(1, updateAllControllers, NULL, &controllerTimer);
}

static void updateController(volatile Controller *c) {
    c->u = (sio_hw->gpio_in >> UP_PIN) & 1u;
    c->d = (sio_hw->gpio_in >> DOWN_PIN) & 1u;
    c->l = (sio_hw->gpio_in >> LEFT_PIN) & 1u;
    c->r = (sio_hw->gpio_in >> RIGHT_PIN) & 1u;
    c->a = (sio_hw->gpio_in >> A_PIN) & 1u;
    c->b = (sio_hw->gpio_in >> B_PIN) & 1u;
    c->x = (sio_hw->gpio_in >> X_PIN) & 1u;
    c->y = (sio_hw->gpio_in >> Y_PIN) & 1u;
}

/*
    Controller Select Truth Table
A | B | Out
0   0   C1
1   0   C2
0   1   C3
1   1   C4
*/
static bool updateAllControllers(struct repeating_timer *t) {
    sio_hw->gpio_clr = (1u << CONTROLLER_SEL_A_PIN) | (1u << CONTROLLER_SEL_B_PIN); //Connect Controller 1
    updateController(&C1);

    sio_hw->gpio_set = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 2
    updateController(&C2);

    sio_hw->gpio_clr = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 3
    sio_hw->gpio_set = (1u << CONTROLLER_SEL_B_PIN);
    updateController(&C3);

    sio_hw->gpio_set = (1u << CONTROLLER_SEL_A_PIN); //Connect Controller 4
    updateController(&C4);
 
    return true; //not sure if this is required or not, but it's in the sample code
}