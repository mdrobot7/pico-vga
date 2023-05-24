#include "lib-internal.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"

//static bool updateAllControllers(struct repeating_timer *t);

int initController() {
    //static struct repeating_timer controllerTimer;
    //add_repeating_timer_ms(1, updateAllControllers, NULL, &controllerTimer);

    /*
        Pseudocode and ideas (I2C is complicated, so I'm leaving this for later)
    - Two timer interrupts (or maybe there's a way to do this within the peripheral, idk):
      - one pings the I2C bus every 1-5ms or so, sending to the default controller I2C address,
        "are there any unconnected controllers out there?"
        - a controller who has had buttons pressed will respond "yes"
        - code will assign it a new ID, in sequence with others, and save it
      - other pings all controllers in sequence every [controllerPingInterval] (probably 1ms), gets
        one byte from each one representing all of the button statuses. saves to RAM.
          - Make unpacking functions (for users that don't want to do bitwise)
    */
    i2c0_hw->enable = I2C_IC_ENABLE_ENABLE_BITS;
    i2c0_hw->con = I2C_IC_CON_IC_SLAVE_DISABLE_BITS | (I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB) |
                   I2C_IC_CON_MASTER_MODE_BITS;

    return 0;
}

int deInitController() {
    return 0;
}