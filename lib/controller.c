#include "lib-internal.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

static i2c_hw_t * c_i2c_hw = NULL;
static repeating_timer_t poll_controllers_timer;

static uint8_t rx_dma_chan = 0;
static uint8_t tx_dma_chan = 0;
static uint8_t addr_dma_chan = 0;

static uint8_t controller_addr[MAX_NUM_CONTROLLERS];
static uint16_t controller_dma_read_data[MAX_NUM_CONTROLLERS*8]; //packed data to feed to the DMA to trigger repeated reads -- one per BYTE to be read
static uint8_t controller_data[MAX_NUM_CONTROLLERS][8]; //max 8 bytes of data per controller


static bool poll_controllers(repeating_timer_t * t) {
    //check if last transfer was aborted -- see i2c.c, i2c_read_blocking_internal()

    c_i2c_hw->enable = 0;
    c_i2c_hw->tar = 123; //first controller address here
    c_i2c_hw->enable = 1u;

    dma_channel_start(rx_dma_chan);
    dma_channel_start(addr_dma_chan);

    return true;
}

static irq_handler_t check_poll_status() {
    if(dma_channel_get_irq1_status(addr_dma_chan)) {
        dma_channel_acknowledge_irq1(addr_dma_chan);

        //check if the channel has read all of the controller addresses
        if(dma_hw->ch[addr_dma_chan].read_addr >= controller_addr[MAX_NUM_CONTROLLERS]) {
            dma_channel_abort(addr_dma_chan);
        }
    }
}

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

      
        Pseudocode v2:
    - First, why the above doesn't work:
      - I2C isn't an "open" bus. Devices can't talk freely. Peripherals with the same address 
      cause major issues, and if you try to send a message to an address with multiple peripherals 
      on it, it will fail. Therefore, you can't have an "unconnected address".
      - Also, I'm not doing constant pings with a timer. I'm going to make a controller_pair() function,
      which will scan for 100ms or so continuously and then stop. The user can then call this as many times
      and whenever they want.
    - Solution, as of now:
      - When controller_pair() is called, the processor starts continuously scanning the I2C bus, looping
      over every address. When it gets a reply, it pairs that controller by 1) sending back a "you're paired"
      confirmation, 2) recording its ID in RAM. It then continues looping until the 100ms expires.
      - On the controller side, the software picks an I2C address at random (using an unconnected ADC pin value
      or something). It rescrambles the I2C address it's listening on every few ms, in hopes that it will end
      up on its own I2C address eventually and get paired. Once it's paired, it stops scrambling the address
      and goes into normal operation, waiting for a request for data from the main board.
    
    - Alt solution (I'd rather not):
      - Switches on the controller to select what player you are (1, 2, 3, 4). 2 SPDTs will do, that will be
      2 bits/4 states, enough for 4 players.
    */

    c_i2c_hw = i2c_get_hw(CONTROLLER_I2C);

    gpio_set_function(CONTROLLER_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(CONTROLLER_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(CONTROLLER_SDA_PIN); //I2C requires pullups on both lines
    gpio_pull_up(CONTROLLER_SCL_PIN);
    i2c_init(CONTROLLER_I2C, 1000000); //Fast mode plus, 1MB/s

    rx_dma_chan = dma_claim_unused_channel(true);
    tx_dma_chan = dma_claim_unused_channel(true);
    addr_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config rx_conf = dma_channel_get_default_config(rx_dma_chan);
    channel_config_set_read_increment(&rx_conf, false);
    channel_config_set_write_increment(&rx_conf, true);
    channel_config_set_transfer_data_size(&rx_conf, DMA_SIZE_8);
    channel_config_set_dreq(&rx_conf, i2c_get_dreq(CONTROLLER_I2C, false));
    dma_channel_configure(rx_dma_chan, &rx_conf, controller_data, &c_i2c_hw->data_cmd, 
                          controllerConfig->numControllers*8, false);
    
    dma_channel_config tx_conf = dma_channel_get_default_config(tx_dma_chan);
    channel_config_set_read_increment(&tx_conf, true);
    channel_config_set_write_increment(&tx_conf, false);
    channel_config_set_transfer_data_size(&tx_conf, DMA_SIZE_16);
    channel_config_set_dreq(&tx_conf, i2c_get_dreq(CONTROLLER_I2C, true));
    channel_config_set_chain_to(&tx_conf, addr_dma_chan);
    dma_channel_configure(tx_dma_chan, &tx_conf, &c_i2c_hw->data_cmd, controller_dma_read_data, 8, false);

    dma_channel_config addr_conf = dma_channel_get_default_config(addr_dma_chan);
    channel_config_set_read_increment(&addr_conf, true);
    channel_config_set_write_increment(&addr_conf, false);
    channel_config_set_transfer_data_size(&addr_conf, DMA_SIZE_8);
    channel_config_set_dreq(&addr_conf, DREQ_FORCE);
    channel_config_set_chain_to(&addr_conf, tx_dma_chan);
    dma_channel_configure(addr_dma_chan, &addr_conf, &c_i2c_hw->tar, controller_addr, 1, false);

    dma_channel_set_irq1_enabled(addr_dma_chan, true);
    irq_add_shared_handler(DMA_IRQ_1, (irq_handler_t)check_poll_status, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);

    add_repeating_timer_us(-1000, poll_controllers, NULL, &poll_controllers_timer);

    return 0;
}

int deInitController() {
    return 0;
}