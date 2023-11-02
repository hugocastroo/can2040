// source: https://gitea.predevolution-technologies.de/anme/CAN2040_Test

#include <stdint.h>
#include <stdio.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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

constexpr uint32_t kGPIORXA = 0, kGPIOTXA = 1;
constexpr uint32_t kGPIORXB = 14, kGPIOTXB = 15;

constexpr uint32_t kPIONumA = 0;
constexpr uint32_t kPIONumB = 1;

constexpr uint64_t kToggleLEDTime = 500 * 1000;
constexpr uint64_t kSendMessageTime = 1 * 1000;
constexpr uint32_t kSleepUS = 1;

static struct can2040 cbus_a;
static struct can2040 cbus_b;

static struct can2040_msg latest_msg_a = {};
static volatile uint32_t latest_notify_a = 0;
static volatile bool new_message_a = false;

static struct can2040_msg latest_msg_b = {};
static volatile uint32_t latest_notify_b = 0;
static volatile bool new_message_b = false;

static void can2040_cb_a(struct can2040 *cd, uint32_t notify,
                         struct can2040_msg *msg) {
  new_message_a = true;
  latest_notify_a = notify;
  latest_msg_a = *msg;
}

static void can2040_cb_b(struct can2040 *cd, uint32_t notify,
                         struct can2040_msg *msg) {
  new_message_b = true;
  latest_notify_b = notify;
  latest_msg_b = *msg;
}

static void PIOx_IRQHandler_A(void) { can2040_pio_irq_handler(&cbus_a); }
static void PIOx_IRQHandler_B(void) { can2040_pio_irq_handler(&cbus_b); }

void canbus_setup(void) {
  // Setup canbus
  can2040_setup(&cbus_a, kPIONumA);
  can2040_setup(&cbus_b, kPIONumB);

  can2040_callback_config(&cbus_a, can2040_cb_a);
  can2040_callback_config(&cbus_b, can2040_cb_b);

  // Enable irqs
  irq_set_exclusive_handler(PIO0_IRQ_0_IRQn, PIOx_IRQHandler_A);
  NVIC_SetPriority(PIO0_IRQ_0_IRQn, 1);
  NVIC_EnableIRQ(PIO0_IRQ_0_IRQn);

  irq_set_exclusive_handler(PIO1_IRQ_0_IRQn, PIOx_IRQHandler_B);
  NVIC_SetPriority(PIO1_IRQ_0_IRQn, 1);
  NVIC_EnableIRQ(PIO1_IRQ_0_IRQn);

  // Start canbus
  can2040_start(&cbus_a, kSysClock, kBitRate, kGPIORXA, kGPIOTXA);
  can2040_start(&cbus_b, kSysClock, kBitRate, kGPIORXB, kGPIOTXB);
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
      msg.id = 0x142;
      msg.dlc = 8;
      int data[8] = {0xA2, 0x00, 0x00, 0x00, 0xBD, 0x11, 0x00, 0x00};
      for (int i = 0; i < 8; i++) {
        msg.data[i] = data[i];
      }

      can2040_transmit(&cbus_a, &msg);
      sleep_us(200);
      can2040_transmit(&cbus_a, &msg);
      sleep_us(200);
      can2040_transmit(&cbus_a, &msg);
      sleep_us(200);
      count++;
    }

    if (new_message_a) {
      new_message_a = false;
      if (latest_notify_a == CAN2040_NOTIFY_RX) {
        // printf("Got CAN message on A!\n");
      } else if (latest_notify_a == CAN2040_NOTIFY_TX) {
        // printf("Sent CAN message on A!\n");
      } else if (latest_notify_a == CAN2040_NOTIFY_ERROR) {
        printf("CAN error on A!\n");
      } else {
        printf("INVALID NOTIFY A\n");
      }
    }

    if (new_message_b) {
      new_message_b = false;
      if (latest_notify_b == CAN2040_NOTIFY_RX) {
        // printf("Got CAN message on B!\n");
        // printf("Time: %" PRIu64 "\n", time_us_64() / 1000);
        // for (int i = 0; i < latest_msg_b.dlc; i++) {
        //   printf("%d ", static_cast<int>(latest_msg_b.data[i]));
        // }
        // printf("\n");
      } else if (latest_notify_b == CAN2040_NOTIFY_TX) {
        // printf("Sent CAN message on B!\n");
      } else if (latest_notify_b == CAN2040_NOTIFY_ERROR) {
        printf("CAN error on B!\n");
      } else {
        printf("INVALID NOTIFY B\n");
      }
    }
    sleep_us(kSleepUS);
  }
}
