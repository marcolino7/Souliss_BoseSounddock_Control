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
#define HOSTNAME	"souliss-bose-sounddock"

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
#define POWER_OFF			  3
#define T_WIFI_STRDB		  4	//It takes 2 slots
#define T_WIFI_STR			  6	//It takes 2 slots

//Deadband for analog values
#define NODEADBAND		0				//Se la variazione è superio del 0,1% aggiorno

// **** Define here the right pin for your ESP module **** 
#define	PIN_RELE			4
#define PIN_BUTTON			0
#define PIN_LED				16
#define PIN_IR				5

//Useful Variable
byte led_status = 0;
byte joined = 0;
U8 value_hold = 0x068;

//Variable to Handle WiFio Signal
long rssi = 0;
int bars = 0;

//IR Stuff
IRsend irsend(PIN_IR);
int khz = 38; //IR Carrier Frequancy @38 KHz
// IR code to send
unsigned int Signal_VolUP[] = { 1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50304,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50276,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,50300,1000,1500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500 }; //AnalysIR Batch Export (IRremote) - RAW
unsigned int Signal_VolDW[] = { 1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50304,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50356,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1000,1500,500,500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,1500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500 }; //AnalysIR Batch Export (IRremote) - RAW
unsigned int Signal_PwrOff[] = { 1000,1500,500,1500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1500,1500,500,1500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50280,1000,1500,500,1500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50300,1500,1500,500,1500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500,50280,1500,1500,500,1500,500,1500,500,1500,500,1500,500,1500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,1500,500,1500,500,1500,500 }; //AnalysIR Batch Export (IRremote) - RAW

void setup()
{
	#ifdef SERIAL_DEBUG
		Serial.begin(115200);
		Serial.println("Node Delay Starting");
	#endif
	

	//Delay the startup. In case of power outage, this give time to router to start WiFi
	#ifndef SERIAL_DEBUG
		//Inserire qua l'ultima cifra dell'indirizzo IP per avere un delay all'avvio diverso per ogni nodo
		delay((IP_ADDRESS - 128) * 5000);
		//delay(15000);
	#endif
	Initialize();

	#ifdef SERIAL_DEBUG
		Serial.println("Node Inizializing");
	#endif


	//DHCP con indirizzo fisso con reservation
	//GetIPAddress();
	//SetAsGateway(myvNet_dhcp);

	// Connect to the WiFi network with static IP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);

	#ifdef SERIAL_DEBUG
		Serial.println("WiFi Joined, setting up stuff");
	#endif


	Set_SimpleLight(POWER_SOCKET);			// Define a T11 to hanlde the relè
	Souliss_SetT14(memory_map, VOLUME_UP);	//Set logic to turn up volume
	Souliss_SetT14(memory_map, VOLUME_DW);  //Set logic to turn down volume
	Souliss_SetT14(memory_map, POWER_OFF);  //Set logic to turn on and OFF
	Souliss_SetT51(memory_map, T_WIFI_STRDB);	//Imposto il tipico per contenere il segnale del Wifi in decibel
	Souliss_SetT51(memory_map, T_WIFI_STR);	//Imposto il tipico per contenere il segnale del Wifi in barre da 1 a 5

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

		FAST_210ms() {

			//T14 logic handling
			Souliss_Logic_T11(memory_map, VOLUME_UP, &data_changed);
			Souliss_Logic_T11(memory_map, VOLUME_DW, &data_changed);
			Souliss_Logic_T11(memory_map, POWER_OFF, &data_changed);

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

			if (mOutput(POWER_OFF) == 1) {
				irsend.sendRaw(Signal_PwrOff, sizeof(Signal_PwrOff) / sizeof(Signal_PwrOff[0]), khz); //Note the approach used to automatically calculate the size of the array.
			#ifdef SERIAL_DEBUG
				Serial.println("IR PowerOff Sent");
			#endif
				mOutput(POWER_OFF) = 0;
			}

			//Processa le logiche per il segnale WiFi
			Souliss_Logic_T51(memory_map, T_WIFI_STRDB, NODEADBAND, &data_changed);
			Souliss_Logic_T51(memory_map, T_WIFI_STR, NODEADBAND, &data_changed);
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
		}

		FAST_PeerComms();
		ArduinoOTA.handle();
	}
	EXECUTESLOW() {
		UPDATESLOW();
		SLOW_10s() {
			//Check wifi signal
			check_wifi_signal();
		}
	}
}

void check_wifi_signal() {
	rssi = WiFi.RSSI();

	if (rssi > -55) {
		bars = 5;
	}
	else if (rssi < -55 & rssi >= -65) {
		bars = 4;
	}
	else if (rssi < -65 & rssi >= -70) {
		bars = 3;
	}
	else if (rssi < -70 & rssi >= -78) {
		bars = 2;
	}
	else if (rssi < -78 & rssi > -82) {
		bars = 1;
	}
	else {
		bars = 0;
	}
	float f_rssi = (float)rssi;
	float f_bars = (float)bars;
	Souliss_ImportAnalog(memory_map, T_WIFI_STRDB, &f_rssi);
	Souliss_ImportAnalog(memory_map, T_WIFI_STR, &f_bars);
#ifdef SERIAL_DEBUG
	Serial.print("wifi rssi:");
	Serial.println(rssi);
	Serial.print("wifi bars:");
	Serial.println(bars);
#endif
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
