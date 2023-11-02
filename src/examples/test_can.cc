// source: https://gitea.predevolution-technologies.de/anme/CAN2040_Test

#include <stdint.h>
#include <stdio.h>

#include <iostream>

#include "RP2040.h"
extern "C" {
#include "can2040/src/can2040.h"
}
#include "hardware/irq.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/time.h"

constexpr uint32_t kSysClock = 125'000'000;
constexpr uint32_t kBitRate = 1'000'000;
constexpr uint32_t kGPIORX = 0, kGPIOTX = 1;
constexpr uint32_t kPIONum = 0;

constexpr uint64_t kToggleLEDTime = 500 * 1000;
constexpr uint64_t kSendMessageTime = 1 * 1000;
constexpr uint32_t kSleepUS = 1;

static struct can2040 cbus;

static struct can2040_msg latest_msg = {};
static volatile uint32_t latest_notify = 0;
static volatile bool new_message = false;

static void can2040_cb(struct can2040 *cd, uint32_t notify,
                       struct can2040_msg *msg) {
  new_message = true;
  latest_notify = notify;
  latest_msg = *msg;
}

static void PIOx_IRQHandler(void) { can2040_pio_irq_handler(&cbus); }

void canbus_setup(void) {
  // Setup canbus
  can2040_setup(&cbus, kPIONum);
  can2040_callback_config(&cbus, can2040_cb);

  // Enable irqs
  irq_set_exclusive_handler(PIO0_IRQ_0_IRQn, PIOx_IRQHandler);
  NVIC_SetPriority(PIO0_IRQ_0_IRQn, 1);
  NVIC_EnableIRQ(PIO0_IRQ_0_IRQn);

  // Start canbus
  can2040_start(&cbus, kSysClock, kBitRate, kGPIORX, kGPIOTX);
}

int main(void) {
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  int32_t ledState = 0;
  stdio_init_all();

  canbus_setup();

  int count = 0;

  uint64_t last_led_toggle = time_us_64();
  uint64_t last_msg_sent = time_us_64();
  while (1) {
    if (time_us_64() - last_led_toggle > kToggleLEDTime) {
      last_led_toggle = time_us_64();
      std::cout << "count: " << count << "\n";
      count++;

      gpio_put(LED_PIN, ledState);
      if (ledState == 0) {
        ledState = 1;
      } else {
        ledState = 0;
      }
    }

    if (time_us_64() - last_msg_sent > kSendMessageTime) {
      last_msg_sent = time_us_64();
      //   std::cout << "Last msg sent: " << last_msg_sent << "\n";
      struct can2040_msg msg = {};
      msg.id = 100;
      msg.dlc = 8;
      int data[8] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
      for (int i = 0; i < 8; i++) {
        msg.data[i] = data[i];
      }

      can2040_transmit(&cbus, &msg);
    }

    if (new_message) {
      new_message = false;
      if (latest_notify == CAN2040_NOTIFY_RX) {
        std::cout << ("got can message!\n");
      } else if (latest_notify == CAN2040_NOTIFY_TX) {
        std::cout << ("sent can message!\n");
      } else if (latest_notify == CAN2040_NOTIFY_ERROR) {
        std::cout << ("can error!\n");
      } else {
        std::cout << ("INVALID NOTIFY\n");
      }
    }
    sleep_us(kSleepUS);
  }
}
