/**************************************************************************
Souliss - Power Socket Porting for Expressif ESP8266
		  Adding IR library to control Bose SoundDock III Volume
			https://github.com/markszabo/IRremoteESP8266

It use static IP Addressing

Load this code on ESP8266 board using the porting of the Arduino core
for this platform.

***************************************************************************/

//#define SERIAL_DEBUG
#define IP_ADDRESS	138
#define HOSTNAME	"bose-sounddock"

#define	VNET_RESETTIME_INSKETCH
#define VNET_RESETTIME			0x00042F7	// ((20 Min*60)*1000)/70ms = 17143 => 42F7
#define VNET_HARDRESET			ESP.reset()

#define VNET_DEBUG_INSKETCH
#define VNET_DEBUG  		0

// Configure the framework
#include "bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266

// **** Define the WiFi name and password ****
#include "D:\__User\Administrator\Documents\Privati\ArduinoWiFiInclude\wifi.h"
//To avoide to share my wifi credentials on git, I included them in external file
//To setup your credentials remove my include, un-comment below 3 lines and fill with
//Yours wifi credentials
//#define WIFICONF_INSKETCH
//#define WiFi_SSID               "wifi_name"
//#define WiFi_Password           "wifi_password"    


// Include framework code and libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "Souliss.h"
#include <IRremoteESP8266.h>

// Define the network configuration according to your router settings
uint8_t ip_address[4] = { 192, 168, 1, IP_ADDRESS };
uint8_t subnet_mask[4] = { 255, 255, 255, 0 };
uint8_t ip_gateway[4] = { 192, 168, 1, 1 };

// This identify the numbers of the logic
#define POWER_SOCKET          0
#define VOLUME_UP			  1
#define VOLUME_DW			  2	

// **** Define here the right pin for your ESP module **** 
#define	PIN_RELE			5
#define PIN_BUTTON			0
#define PIN_LED				16
#define PIN_IR				4

//Useful Variable
byte led_status = 0;
byte joined = 0;
U8 value_hold = 0x068;

//IR Stuff
IRsend irsend(PIN_IR);
int khz = 38; //IR Carrier Frequancy @38 KHz
// IR code to send
unsigned int Signal_VolUP[] = { 1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50304,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50276,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500 }; //AnalysIR Batch Export (IRremote) - RAW
unsigned int Signal_VolDW[] = { 1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50304,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50356,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500 }; //AnalysIR Batch Export (IRremote) - RAW


void setup()
{
	#ifdef SERIAL_DEBUG
		Serial.begin(115200);
		Serial.println("Node Delay Starting");
	#endif
	

	//Delay the startup. In case of power outage, this give time to router to start WiFi
	delay((IP_ADDRESS - 128) * 5000);
	Initialize();

	#ifdef SERIAL_DEBUG
		Serial.println("Node Inizialized");
	#endif

	// Connect to the WiFi network with static IP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);

	//DHCP con indirizzo fisso con reservation
	//GetIPAddress();
	//SetAsGateway(myvNet_dhcp);

	#ifdef SERIAL_DEBUG
		Serial.println("WiFi Joined, setting up stuff");
	#endif


	Set_SimpleLight(POWER_SOCKET);			// Define a T11 to hanlde the relè
	Souliss_SetT14(memory_map, VOLUME_UP);	//Set logic to turn up volume
	Souliss_SetT14(memory_map, VOLUME_DW);  //Set logic to turn down volume

	pinMode(PIN_RELE, OUTPUT);				// Use pin as output
	pinMode(PIN_BUTTON, INPUT);				// Use pin as input
	pinMode(PIN_LED, OUTPUT);				// Use pin as output
	pinMode(PIN_IR, OUTPUT);				// Use pin as output

	//Enable OTA
	ArduinoOTA.setHostname(HOSTNAME);
	ArduinoOTA.begin();

	//Inizialize IR and Serial
	irsend.begin();

	#ifdef SERIAL_DEBUG
		Serial.print("MAC: ");
		Serial.println(WiFi.macAddress());
		Serial.print("IP:  ");
		Serial.println(WiFi.localIP());
		Serial.print("Subnet: ");
		Serial.println(WiFi.subnetMask());
		Serial.print("Gateway: ");
		Serial.println("Node Initialized");
	#endif
}

void loop()
{
	// Here we start to play
	EXECUTEFAST() {
		UPDATEFAST();

		FAST_50ms() {   // We process the logic and relevant input and output every 50 milliseconds

			// Detect the button press. Short press toggle T11, long press reset the node
			U8 invalue = LowDigInHold(PIN_BUTTON, Souliss_T1n_ToggleCmd, value_hold, POWER_SOCKET);
			if (invalue == Souliss_T1n_ToggleCmd) {
				#ifdef SERIAL_DEBUG
					Serial.println("TOGGLE");
				#endif
				mInput(POWER_SOCKET) = Souliss_T1n_ToggleCmd;
			}
			else if (invalue == value_hold) {
				// reset
				#ifdef SERIAL_DEBUG
					Serial.println("REBOOT");
				#endif
				delay(1000);
				ESP.reset();
			}

			//Output Handling
			DigOut(PIN_RELE, Souliss_T1n_Coil, POWER_SOCKET);

			//Check if joined and take control of the led
			//If not Joined the led blink, if Joined the led reflect the T11 Status
			if (joined == 1) {
				if (mOutput(POWER_SOCKET) == 1) {
					digitalWrite(PIN_LED, HIGH);
				}
				else {
					digitalWrite(PIN_LED, LOW);
				}
			}
		}

		FAST_90ms() {
			//Apply logic if statuses changed
			Logic_SimpleLight(POWER_SOCKET);
		}

		FAST_510ms() {
			//Check if joined to gateway
			check_if_joined();

			#ifdef SERIAL_DEBUG
				if (!joined) {
					Serial.print("JOIN STATUS=");
					Serial.println(joined);
				}
			#endif

		}

		FAST_710ms() {
			
			//T14 logic handling
			Souliss_Logic_T11(memory_map, VOLUME_UP, &data_changed);
			Souliss_Logic_T11(memory_map, VOLUME_DW, &data_changed);

			//If one button is pressed, the relevand IR code will be sent
			if (mOutput(VOLUME_UP) == 1) {
				irsend.sendRaw(Signal_VolUP, sizeof(Signal_VolUP) / sizeof(Signal_VolUP[0]), khz); //Note the approach used to automatically calculate the size of the array.
				#ifdef SERIAL_DEBUG
					Serial.println("IR VOL+ Sent");
				#endif
				mOutput(VOLUME_UP) = 0;
			}

			if (mOutput(VOLUME_DW) == 1) {
				irsend.sendRaw(Signal_VolDW, sizeof(Signal_VolDW) / sizeof(Signal_VolDW[0]), khz); //Note the approach used to automatically calculate the size of the array.
				#ifdef SERIAL_DEBUG
					Serial.println("IR VOL- Sent");
				#endif
				mOutput(VOLUME_DW) = 0;
			}
		
		}

		FAST_PeerComms();
	}
	ArduinoOTA.handle();
}

//This routine check for peer is joined to Souliss Network
//If not blink the led every 500ms, else led is a mirror of relè status
void check_if_joined() {
	if (JoinInProgress() && joined == 0) {
		joined = 0;
		if (led_status == 0) {
			digitalWrite(PIN_LED, HIGH);
			led_status = 1;
		}
		else {
			digitalWrite(PIN_LED, LOW);
			led_status = 0;
		}
	}
	else {
		joined = 1;
	}
}
