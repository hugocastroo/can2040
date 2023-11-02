#include "can2040.c" // can2040_setup
#include <stdio.h>   //IO std library
#include "pico/stdlib.h" //Pico Library for GPIO use

//CONFIG Variablen
#define CONFIG_CANBUS_FREQUENCY 125000
#define CONFIG_RP2040_CANBUS_GPIO_RX    4  // GPIO 4 <=> CAN TX
#define CONFIG_RP2040_CANBUS_GPIO_TX    5  // GPIO 5 <=> CAN RX
#define CONFIG_RP2040_CANBUS_GPIO_CORE    0  // GPIO 5 <=> CAN RX
#define FREQ_SYS 125000000

typedef struct can2040_msg CANMsg; //Struct for cleaner coding below
static struct can2040 cbus;

static void
can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
{
    // Add message processing code here...
    /*printf("Test printing the 'Info in the function\n");
    printf("Pointer: %p \n",*msg);
    printf("hexPointer: %02x \n",*msg);
    printf("MsgIDInt: %d \n",msg->id);
    printf("MsgIDHex: %02x \n",msg->id);
    printf("MsgIDDLC: %d \n",msg->dlc);
    printf("MsgDataHex: %02x ",msg->data[0]);
    printf(", %02x",msg->data[1]);
    printf(", %02x",msg->data[2]);
    printf(", %02x \n",msg->data[3]);
	*/
	
	//Check if the message was sent/received/error message
	if (notify == CAN2040_NOTIFY_RX)
    {
        printf("Recv msg: id: %0x, size: %0x, data: ", msg->id, msg->dlc);
		//Loop for printing the message data
		for (int i = 0; i < msg->dlc; i++) {
			printf("%0x, ", msg->data[i]);
		}
		printf("\n");
    } else if (notify == CAN2040_NOTIFY_TX) {
        printf("Confirmed tx msg: id: %0x, size: %0x, data: ", msg->id, msg->dlc);
		//Loop for printing the message data
		for (int i = 0; i < msg->dlc; i++) {
			printf("%0x, ", msg->data[i]);
		}
		printf("\n");
    } else if (notify & CAN2040_NOTIFY_ERROR) {
        printf("Error(%d) on msg: id: %0x, size: %0x, data: ", msg->id, msg->dlc);
		//Loop for printing the message data
		for (int i = 0; i < msg->dlc; i++) {
			printf("%0x, ", msg->data[i]);
		}
		printf("\n");
    }
}

// Main PIO irq handler
static void
PIOx_IRQHandler(void)
{
    can2040_pio_irq_handler(&cbus);
}

//CANBUS setup und start method
void
canbus_setup(void)
{
    // Setup canbus
    can2040_setup(&cbus, CONFIG_RP2040_CANBUS_GPIO_CORE);
    can2040_callback_config(&cbus, can2040_cb);

    // Enable irqs
    irq_set_exclusive_handler(PIO0_IRQ_0_IRQn, PIOx_IRQHandler);
    NVIC_SetPriority(PIO0_IRQ_0_IRQn, 1);
    NVIC_EnableIRQ(PIO0_IRQ_0_IRQn);
	//irq_set_priority(PIO0_IRQ_0, 1); //Check the difference between IRQ and NVIC
    //irq_set_enabled(PIO0_IRQ_0, true);

    // Start canbus
    can2040_start(&cbus, FREQ_SYS, CONFIG_CANBUS_FREQUENCY, CONFIG_RP2040_CANBUS_GPIO_RX, CONFIG_RP2040_CANBUS_GPIO_TX);
}

//Main method
int main()
{
	//Init all and start the CANBUS
    stdio_init_all();
    canbus_setup();
    const uint LED_PIN = PICO_DEFAULT_LED_PIN; //Set the LED for the PICO board
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
	
	//Initialize the different messages that should be sent
	CANMsg ExtMsg = {
            .id = 0xffff | CAN2040_ID_EFF, //CAN2040_ID_EFF and CAN2040_ID_RTR configuration is possible
            .dlc = 8,
            .data = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}
	};
	
    while (true) {
		
		//Initialize the message to be sent
        CANMsg Test = {
			.id = 0x01, //CAN2040_ID_EFF and CAN2040_ID_RTR configuration is possible
            .dlc = 8,
            .data = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}
		};
		
		//Check if the TX buffer is available for new TX messages
		if (can2040_check_transmit(&cbus)) {
			//Transmit the created messages
			int res = can2040_transmit(&cbus, &ExtMsg);
			if (!res) {
				printf("Message sended\n");
				gpio_put(LED_PIN, 1); //printf("LED ON!\n");
			}
			else{
				printf("Failed\n");
				gpio_put(LED_PIN, 0); //printf("LED OFF!\n");
			}
		}
        sleep_ms(1000); //Delay for diff, purposes
    }
    return 0;
}