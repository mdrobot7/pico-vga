#ifndef PV_CONTROLLER_H
#define PV_CONTROLLER_H

#include <pico/stdlib.h>

#define CONTROLLER_MAX 4
typedef struct __packed {
    uint8_t a;
} ControllerConfig_t;

//Default configuration for ControllerConfig_t
#define CONTROLLER_CONFIG_DEFAULT {\
    .a = 0,\
}

bool controller_pair();
void controller_unpair(uint8_t controller);
void controller_unpair_all();
uint8_t controller_get_num_paired_controllers();

typedef enum {
    CONTROLLER_BUTTON_UP,
    CONTROLLER_BUTTON_DOWN,
    CONTROLLER_BUTTON_LEFT,
    CONTROLLER_BUTTON_RIGHT,
    CONTROLLER_BUTTON_A,
    CONTROLLER_BUTTON_B,
    CONTROLLER_BUTTON_X,
    CONTROLLER_BUTTON_Y,
} controller_button_t;

bool controller_get_button_state(uint8_t controller, controller_button_t button);

#endif