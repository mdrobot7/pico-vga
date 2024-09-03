#include "lib-internal.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

static i2c_hw_t * c_i2c_hw = NULL;
static repeating_timer_t poll_controllers_timer;

static uint8_t rx_dma_chan = 0;
static uint8_t tx_dma_chan = 0;

static uint8_t controller_addr[MAX_NUM_CONTROLLERS];
const static uint16_t controller_dma_read_data[8] = { //packed data to feed to the DMA to trigger repeated reads -- one per BYTE to be read
    (1 << I2C_IC_DATA_CMD_RESTART_LSB) | (1 << I2C_IC_DATA_CMD_CMD_LSB),
    1 << I2C_IC_DATA_CMD_CMD_LSB, //read from device
    1 << I2C_IC_DATA_CMD_CMD_LSB,
    1 << I2C_IC_DATA_CMD_CMD_LSB,
    1 << I2C_IC_DATA_CMD_CMD_LSB,
    1 << I2C_IC_DATA_CMD_CMD_LSB,
    1 << I2C_IC_DATA_CMD_CMD_LSB,
    (1 << I2C_IC_DATA_CMD_STOP_LSB) | (1 << I2C_IC_DATA_CMD_CMD_LSB),
};
static uint8_t controller_data[MAX_NUM_CONTROLLERS][8]; //max 8 bytes of data per controller

static uint8_t num_controllers_paired = 0;

/**
 * @brief After all 8 bytes of controller data have been received, this interrupt handler is called.
 * Disables I2C, resets the target device address, and restarts everything.
 *
 * @return irq_handler_t
 */
static irq_handler_t set_i2c_read_address() {
    static uint8_t current_addr = 0;

    if(c_i2c_hw->clr_tx_abrt) { //error requesting data. controller presumed unplugged, so remove its address
        num_controllers_paired--;
        dma_channel_set_trans_count(rx_dma_chan, num_controllers_paired*8, false);
        controller_addr[current_addr] = 0;
        c_i2c_hw->enable = 0;
        return NULL; //skip the rest of this transaction
    }

    if(++current_addr >= num_controllers_paired) { //no more controllers to poll
        current_addr = 0;
        c_i2c_hw->enable = 0;
        return NULL;
    }

    if(controller_addr[current_addr] == 0) current_addr++; //0 in the middle of the addr list

    c_i2c_hw->enable = 0;
    c_i2c_hw->tar = controller_addr[current_addr];
    dma_channel_set_read_addr(tx_dma_chan, controller_dma_read_data, true);
    c_i2c_hw->enable = 1u;
}

/**
 * @brief Timer callback for polling controllers. Called every 1ms.
 *
 * @param t
 * @return true
 * @return false
 */
static bool poll_controllers(repeating_timer_t * t) {
    if(num_controllers_paired == 0) return true;

    dma_channel_set_write_addr(rx_dma_chan, controller_data, true);
    set_i2c_read_address();

    return true;
}

int initController() {
    for(int i = 0; i < MAX_NUM_CONTROLLERS; i++) {
        for(int j = 0; j < 8; j++) {
            controller_data[i][j] = 0;
        }
        controller_addr[i] = 0;
    }

    c_i2c_hw = i2c_get_hw(CONTROLLER_I2C);

    gpio_set_function(CONTROLLER_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(CONTROLLER_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(CONTROLLER_SDA_PIN); //I2C requires pullups on both lines
    gpio_pull_up(CONTROLLER_SCL_PIN);
    i2c_init(CONTROLLER_I2C, 1000000); //Fast mode plus, 1MB/s

    c_i2c_hw->tx_tl = 0; //if the i2c tx fifo is at or below this, it will fire the tx_empty IRQ
    c_i2c_hw->dma_tdlr = 1u; //if the tx fifo is at or below this, it will request more data from DMA.
                             //hopefully helps reduce false tx_empty IRQs
    c_i2c_hw->con |= 1u << I2C_IC_CON_TX_EMPTY_CTRL_LSB; //only fire tx_empty when both the tx fifo is
                                                        //empty AND the last item has been transmitted.
                                                        //hopefully reduces early tx empty IRQs
    c_i2c_hw->intr_mask = 1u << I2C_IC_INTR_MASK_M_TX_EMPTY_LSB; //enable tx empty IRQ


    rx_dma_chan = dma_claim_unused_channel(true);
    tx_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config rx_conf = dma_channel_get_default_config(rx_dma_chan);
    channel_config_set_read_increment(&rx_conf, false);
    channel_config_set_write_increment(&rx_conf, true);
    channel_config_set_transfer_data_size(&rx_conf, DMA_SIZE_8);
    channel_config_set_dreq(&rx_conf, i2c_get_dreq(CONTROLLER_I2C, false));
    dma_channel_configure(rx_dma_chan, &rx_conf, controller_data, &c_i2c_hw->data_cmd,
                          num_controllers_paired*8, false);

    dma_channel_config tx_conf = dma_channel_get_default_config(tx_dma_chan);
    channel_config_set_read_increment(&tx_conf, true);
    channel_config_set_write_increment(&tx_conf, false);
    channel_config_set_transfer_data_size(&tx_conf, DMA_SIZE_16);
    channel_config_set_dreq(&tx_conf, i2c_get_dreq(CONTROLLER_I2C, true));
    dma_channel_configure(tx_dma_chan, &tx_conf, &c_i2c_hw->data_cmd, controller_dma_read_data, 8, false);

    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, true);
    irq_set_priority(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, PICO_DEFAULT_IRQ_PRIORITY);
    irq_set_exclusive_handler(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, (irq_handler_t)set_i2c_read_address);

    add_repeating_timer_us(-1000, poll_controllers, NULL, &poll_controllers_timer);

    return 0;
}

int deInitController() {
    cancel_repeating_timer(&poll_controllers_timer);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    dma_channel_abort(rx_dma_chan);
    dma_channel_abort(tx_dma_chan);
    dma_channel_unclaim(rx_dma_chan);
    dma_channel_unclaim(tx_dma_chan);
    i2c_deinit(CONTROLLER_I2C);

    return 0;
}

/**
 * @brief Scan the I2C bus for new controllers for a 10ms. If one is detected, add its address
 * to the list of known addresses. Exits immediately after a new controller has been found.
 *
 * @return true If a controller was found.
 * @return false If no controller was found or if the maximum number of controllers has already been reached.
 */
bool controller_pair() {
    if(num_controllers_paired == MAX_NUM_CONTROLLERS) return false;

    cancel_repeating_timer(&poll_controllers_timer);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    dma_channel_abort(rx_dma_chan);
    dma_channel_abort(tx_dma_chan);

    for(int i = 0; i < 32; i++) {
        for(int addr = 0b111; addr < 0b1111000; addr++) { //some i2c addresses are reserved, 0b0000XXX and 0b1111XXX, skip them
            int j;
            for(j = 0; j < num_controllers_paired; j++) {
                if(controller_addr[j] == addr) break;
            }
            if(j != num_controllers_paired) continue; //address already found and paired, so skip it

            uint8_t data;
            if(i2c_read_blocking(CONTROLLER_I2C, addr, &data, 1, false) >= 0) {
                //may need to add something else here to tell controller "you're detected, stop scrambling your address"
                num_controllers_paired++;
                for(int j = 0; j < num_controllers_paired; j++) {
                    if(controller_addr[j] == 0) {
                        controller_addr[j] = addr;
                        *(uint64_t *)(controller_data[j]) = (uint64_t)0; //clear out any garbage data
                        break;
                    }
                }
                dma_channel_set_trans_count(rx_dma_chan, num_controllers_paired*8, false);
                irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
                add_repeating_timer_us(-1000, poll_controllers, NULL, &poll_controllers_timer);
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Unpairs a controller. This does not affect the indices of other controllers -- if controller 1
 * is removed, then controllers 2 and 3 still remain controllers 2 and 3.
 *
 * @param controller
 */
void controller_unpair(uint8_t controller) {
    cancel_repeating_timer(&poll_controllers_timer);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    dma_channel_abort(rx_dma_chan);
    dma_channel_abort(tx_dma_chan);

    controller_addr[controller] = 0;
    num_controllers_paired--;

    dma_channel_set_trans_count(rx_dma_chan, num_controllers_paired*8, false);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    add_repeating_timer_us(-1000, poll_controllers, NULL, &poll_controllers_timer);
}

/**
 * @brief Unpairs all paired controllers.
 *
 */
void controller_unpair_all() {
    cancel_repeating_timer(&poll_controllers_timer);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    dma_channel_abort(rx_dma_chan);
    dma_channel_abort(tx_dma_chan);

    for(int i = 0; i < num_controllers_paired; i++) {
        controller_addr[i] = 0;
    }
    num_controllers_paired = 0;

    dma_channel_set_trans_count(rx_dma_chan, 0, false);
    irq_set_enabled(CONTROLLER_I2C == i2c0 ? I2C0_IRQ : I2C1_IRQ, false);
    add_repeating_timer_us(-1000, poll_controllers, NULL, &poll_controllers_timer);
}

/**
 * @brief Returns the number of currently paired controllers.
 *
 * @return uint8_t
 */
uint8_t controller_get_num_paired_controllers() {
    return num_controllers_paired;
}

/**
 * @brief Returns the state (pressed/not pressed) of a particular button of a particular controller.
 * Returns false if controller parameter is invalid (i.e. controller = 3, but you only have 1 controller
 * paired) or if the button is invalid.
 *
 * @param controller The controller to get data from (0 - number of controllers paired)
 * @param button The button to get the state of
 * @return true If the button is pressed
 * @return false If the button is not pressed
 */
bool controller_get_button_state(uint8_t controller, controller_button_t button) {
    if(controller >= num_controllers_paired) return false;
    if(button > CONTROLLER_BUTTON_Y) return false;
    return controller_data[controller][button];
}