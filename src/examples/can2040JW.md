#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/irq.h"
#include "can2040.h"
#include "RP2040.h"

static struct can2040 cbus;
struct can2040_msg rx_msg;
bool rx_received = false;

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    if (notify == CAN2040_NOTIFY_RX) {
    rx_received = true;
    rx_msg = *msg;
  }
}

static void PIOx_IRQHandler(void) {
    can2040_pio_irq_handler(&cbus);
}

void canbus_setup(void) {
    uint32_t pio_num = 0;
    uint32_t sys_clock = 125000000, bitrate = 1000000;
    uint32_t gpio_rx = 18, gpio_tx = 19;

    // Setup canbus
    can2040_setup(&cbus, pio_num);
    can2040_callback_config(&cbus, can2040_cb);

    // Enable irqs
    irq_set_exclusive_handler(PIO0_IRQ_0_IRQn, PIOx_IRQHandler);
    NVIC_SetPriority(PIO0_IRQ_0_IRQn, 1);
    NVIC_EnableIRQ(PIO0_IRQ_0_IRQn);

    // Start canbus
    can2040_start(&cbus, sys_clock, bitrate, gpio_rx, gpio_tx);
}

int main(void) {

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    bool ledState = 1;
    stdio_init_all();
    canbus_setup();
    gpio_put(LED_PIN, ledState);

    while(1){
        if (rx_received) {
            rx_received = false;
            ledState = !ledState;
            gpio_put(LED_PIN, ledState);
        }
    }
}


