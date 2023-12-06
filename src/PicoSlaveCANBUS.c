/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%	This program is part of the research project "Automationssystem für 
%	Fassadenbegrünung zur Optimierung der Energieeffizienz von Gebäuden",
%	from the Hochschule Flensbug.
%	Based on Software CANbus implementation for rp2040 from Kevin O'Connor 
%	<kevin@koconnor.net>
%   Name:           Raspberry Pi Pico slave script
%   Description:    Script for CANBUS communication protocol using a 
%					Raspberry Pi Pico and a CAN tranceiver to make the
%					communication possible. In this case a MCP2551 tranceiver
%					was using with every Raspberry Pi Pico.  The script waits
%					for a remote frame canbus message with an specific
%					message ID, in case the  remote/request messages come
%					then the system sends a temperature and humidity measurement
%					in the CANBUS line. This messages are supossed to be
%					further procesed by the ESP32 master in the CANBUS line.
%   Date:           06/12/2023      
%   Programmer:     Hugo Valentin Castro Saenz
%   History:
%	V01:			Send temeprature and humidy messages when remote message acquired.
%	
%  
% 
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#include "can2040.c" // can2040_setup
#include <stdio.h>   //IO std library
#include "pico/stdlib.h" //Pico Library for GPIO use

//CONFIG Variablen
#define CONFIG_CANBUS_FREQUENCY 125000
#define CONFIG_RP2040_CANBUS_GPIO_RX    4  // GPIO 4 <=> CAN TX
#define CONFIG_RP2040_CANBUS_GPIO_TX    5  // GPIO 5 <=> CAN RX
#define CONFIG_RP2040_CANBUS_GPIO_CORE    0  // GPIO 5 <=> CAN RX
#define FREQ_SYS 125000000

static struct can2040 cbus; //Variable for the CANBUS
typedef struct can2040_msg CANMsg; //Struct for cleaner coding below
CANMsg latest_RXmsg = {};
static volatile uint32_t latest_notify = 0;
static volatile bool newRX_message = false;

const uint LED_PIN = PICO_DEFAULT_LED_PIN; //Set the LED for the PICO board
static int RemoteFrameID = 0x40000063; //RemoteFrameID 99 ist equal to HEX 0x63, bit at the beginning 4, is for the remote transmission request flag
static float Temperature = 10.5;
static float Humidity = 80;


static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg)
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
		newRX_message = true;
		latest_notify = notify;
		latest_RXmsg = *msg;
	  
    } else if (notify == CAN2040_NOTIFY_TX) {

    } else if (notify & CAN2040_NOTIFY_ERROR) {

    }
}

// Main PIO irq handler
static void PIOx_IRQHandler(void)
{
    can2040_pio_irq_handler(&cbus);
}

//CANBUS setup und start method
void canbus_setup(void)
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

//CANBUS send message routine
void canbus_sendmessage(struct can2040_msg msg){
	//Check if the TX buffer is available for new TX messages
	if (can2040_check_transmit(&cbus)) {
		//Transmit the created messages
		int res = can2040_transmit(&cbus, &msg);
		if (!res) {
			printf("Message sended\n");
			gpio_put(LED_PIN, 1); //printf("LED ON!\n");
		}
		else{
			printf("Failed\n");
			gpio_put(LED_PIN, 0); //printf("LED OFF!\n");
		}
	}
}

//Main method
int main()
{
	//Init all and start the CANBUS
    stdio_init_all();
    canbus_setup();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
	
	//Initialize the temperature message.
	CANMsg TempMsg = {
		.id = 0xff01 | CAN2040_ID_EFF, //CAN2040_ID_EFF and CAN2040_ID_RTR configuration bis possible
		.dlc = 4,
		.data32[0] = Temperature
	};
	
	CANMsg HumidMsg = {
		.id = 0xaa01 | CAN2040_ID_EFF, //CAN2040_ID_EFF and CAN2040_ID_RTR configuration bis possible
		.dlc = 4,
		.data32[0] = Humidity
	};
	
    while (true) {
		//Check for new RX messages, check if the ID is the ID of the message is the one according to the remote message asking for the sensors information
		if(newRX_message){
			newRX_message = false;
			if(latest_RXmsg.id == RemoteFrameID){ //If True, the master sent a remote message to ask for sensor information
				canbus_sendmessage(TempMsg);
				canbus_sendmessage(HumidMsg);
			}
			else{
				printf("WrongID\n");
				printf("MsgIDHex: %02x \n",latest_RXmsg.id);
				printf("\n");
			}
		}
    }
    return 0;
}
