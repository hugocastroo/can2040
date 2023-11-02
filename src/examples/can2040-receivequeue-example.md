## Adding a receiver queue

Can20040 implements the receiver portion through using a callback mechanism. An incoming message results in a callback. This way, a user can provide some kind of filtering before passing on the message and only deal with the messages of interest at a higher level. However, since we still run as part of an interrupt handler, the actions taken in the callback should rather be short and quick.

One approach to further deal with the filtered message is to write them to a queue from which the application can fetch them outside of the critical message receive processing. The following example shows a simple implementation using the queue library available in the PICO C++ SDK. Here is the setup code and callback handler. It uses the previous setup example and just adds the queue part.

```CPP

static struct     can2040 cbus;
static queue_t    rx_queue;

static void
can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    if ( notify == CAN2040_NOTIFY_RX ) {

	  // do some message filtering is needed

      if ( ! queue_try_add( &rx_queue, msg )) {

        // we could not add, application dependent handling of the condition
      }
    }
	else if ( notify == CAN2040_NOTIFY_TX ) {

      // a message was successfully sent
    }
    else if ( notify == CAN2040_NOTIFY_ERROR ) {

      // handle an error condition
    }
  }
}

static void
PIOx_IRQHandler(void)
{
    can2040_pio_irq_handler(&cbus);
}

void
canbus_setup(void)
{
    uint32_t pio_num = 0;
    uint32_t sys_clock = 125000000, bitrate = 500000;
    uint32_t gpio_rx = 4, gpio_tx = 5;
    uint32_t rx_queue_size = 4;

	// setup the receiver queue
	queue_init(&rx_queue, sizeof(can2040_msg), rx_queue_size);

    // Setup canbus
    can2040_setup(&cbus, pio_num);
    can2040_callback_config(&cbus, can2040_cb);

    // Enable irqs
    irq_set_exclusive_handler(PIO0_IRQ_0_IRQn, PIOx_IRQHandler);
    irq_set_priority( PIO0_IRQ_0, 1 );
    irq_set_enabled( PIO0_IRQ_0, true );

    // Start canbus
    can2040_start(&cbus, sys_clock, bitrate, gpio_rx, gpio_tx);
}

```

Once the callback queued the message, an application could just read from the queue. A typical example are application that do a lot of periodic work and use a "check" and "receive" model as part of the outer loop. The Arduino environment is a typical example of such a model.

```CPP

#include "can2040.h"

// setup CAN bus ..
struct can2040 cbus;
queue_t        rx_queue;

void
can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
	...
}

void
canbus_setup(void)
{
	...
}

// Arduino main routines...
void setup( )
{
    // as part of the setup ...
	canbus_setup();
}

void loop( )
{

	// as part of the loop...
 	if ( queue_try_remove( &rx_queue, &msg )) {

 	   // there is a message in the queue, go handle it ...
	}
}

```



