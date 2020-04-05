/*
* Home Automation Relay Controller
*
* This project aims to use an Arduino to trigger relay drivers
* attached via I2C by messages received from MQTT
*
* Written by
* Jon Oxer - Copyright 2015-2017 SuperHouse Automation Pty Ltd <info@superhouse.tv>
* Alex Ferrara <alex@receptiveit.com.au>
* James Kennewell - Copyright 2019 Bedrock Media Productions Pty Ltd <james@bedrockmediaproductions.com.au>
*
* 4th September 2019
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.	If not, see <http://www.gnu.org/licenses/>.
*
*/



#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>


/*--------------------------- Configuration ------------------------------*/
/* Core Settings */
#define SERIAL_SPEED						57600	 // Serial port speed


/* Network Settings */
#define ENABLE_DHCP						 true		// true/false
#define ENABLE_MAC_ADDRESS_ROM	true		// true/false
#define MAC_I2C_ADDR						0x50		// Microchip 24AA125E48 I2C ROM address
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };		// Set if no MAC ROM
IPAddress ip(192,168,1,35);						 // Default if DHCP is not used


/* MQTT Settings */
IPAddress broker(192,168,1,108);					 // MQTT broker
#define MQTT_USER				 "mqtt"					 // Username presented to the MQTT broker
#define MQTT_PASSWD			 "Secret"				 // Password presented to the MQTT broker
#define MQTT_TOPIC_BUFFER	40
char clientBuffer[40];										// Maximum clientID length
char messageBuffer[100];									// TODO: unused ATM
char topicBuffer[35];										 // Maximum topic length
const char* mqttClientID			= "Switchboard_12_Relays1-32";	// Client ID presented to the MQTT broker
const char* mqttEventTopic		= "events";										 // MQTT logging topic
const char* mqttTopicPrefix	 = "house/switchboard/12/relay"; // MQTT topic prefix Change the Switchboard ID Eg 12. if you have more then 1 panel.
const char* mqttCmdTopic			= "command";										// MQTT command topic suffix
const char* mqttStatusTopic	 = "status";										 // MQTT status topic suffix


/* Watchdog Timer Settings */
#define ENABLE_EXTERNAL_WATCHDOG				true			 // true / false
#define WATCHDOG_PIN										8					// Output to pat the watchdog
#define WATCHDOG_PULSE_LENGTH					 50				 // Milliseconds
#define WATCHDOG_RESET_INTERVAL				 30000			// Milliseconds. Also the period for sensor reports.
long watchdogLastResetTime = 0;


/* Relay 8 Modules */
#define SHIELD_1_I2C_ADDRESS	0x20	// 0x20 is the address with all jumpers removed
#define SHIELD_2_I2C_ADDRESS	0x21	// 0x21 is the address with a jumper on position A0
#define SHIELD_3_I2C_ADDRESS	0x22	// 0x22 is the address with a jumper on position A1
#define SHIELD_4_I2C_ADDRESS	0x23	// 0x23 is the address with a jumper on position A0, A1
byte shield1BankA = 0; // Current status of all outputs on first shield, one bit per output
byte shield2BankA = 0; // Current status of all outputs on second shield, one bit per output
byte shield3BankA = 0; // Current status of all outputs on first shield, one bit per output
byte shield4BankA = 0; // Current status of all outputs on second shield, one bit per output


/*------------------------------------------------------------------------*/


// Instantiate MQTT client
EthernetClient ethclient;
PubSubClient client(ethclient);


/**
 * Initial setup - called once
 *
 * @return void
 */
void setup()
{
	Serial.begin( SERIAL_SPEED );
	Serial.println("Home Automation Relay Controller");
	Serial.println("--------------------------------");
	Serial.println("Starting up.");

	Wire.begin();

	if( ENABLE_MAC_ADDRESS_ROM == true )
	{
		Serial.print(F("Getting MAC address from ROM: "));
		mac[0] = readRegister(0xFA);
		mac[1] = readRegister(0xFB);
		mac[2] = readRegister(0xFC);
		mac[3] = readRegister(0xFD);
		mac[4] = readRegister(0xFE);
		mac[5] = readRegister(0xFF);
	}
	else
	{
		Serial.print(F("Using static MAC address: "));
	}

	// Print the MAC address
	char macAddr[17];
	sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	Serial.println(macAddr);

	// Set up the Ethernet library to talk to the Wiznet board
	if( ENABLE_DHCP == true )
	{
		// Use DHCP
		Ethernet.begin(mac);
	}
	else
	{
		// Use static address defined above
		Ethernet.begin(mac, ip);
	}

	// Print IP address:
	Serial.print(F("My IP: "));
	for (byte thisByte = 0; thisByte < 4; thisByte++)
	{
		// print the value of each byte of the IP address:
		Serial.print(Ethernet.localIP()[thisByte], DEC);
		if( thisByte < 3 )
		{
			Serial.print(".");
		}
	}

	Serial.println();

	// Initialise Relay8 modules
	// TODO: support multiple shields
	initialiseShield(SHIELD_1_I2C_ADDRESS);
	sendRawValueToLatch(SHIELD_1_I2C_ADDRESS, 0);	// If we don't do this, channel 6 turns on! I don't know why

	initialiseShield(SHIELD_2_I2C_ADDRESS);
	sendRawValueToLatch(SHIELD_2_I2C_ADDRESS, 0);	// If we don't do this, channel 6 turns on! I don't know why

	initialiseShield(SHIELD_3_I2C_ADDRESS);
	sendRawValueToLatch(SHIELD_3_I2C_ADDRESS, 0);	// If we don't do this, channel 6 turns on! I don't know why

	initialiseShield(SHIELD_4_I2C_ADDRESS);
	sendRawValueToLatch(SHIELD_4_I2C_ADDRESS, 0);	// If we don't do this, channel 6 turns on! I don't know why

	// Set MQTT broker settings
	Serial.println("MQTT broker settings...");
	client.setServer(broker, 1883);
	client.setCallback(mqttCallback);

	 /* Set up the watchdog timer */
	if(ENABLE_EXTERNAL_WATCHDOG == true)
	{
		pinMode(WATCHDOG_PIN, OUTPUT);
		digitalWrite(WATCHDOG_PIN, LOW);
	}

	Serial.println("Ready.");
}


/**
 * Determines if Watchdog should be "pat"
 *
 * @return void
 */
void runHeartbeat()
{
	// Is it time to run yet?
	if((millis() - watchdogLastResetTime) > WATCHDOG_RESET_INTERVAL)
	{
		// Only pat the watchdog if we successfully published to MQTT
		patWatchdog();
	}
}


/**
 * "Pat" watchdog module to prevent board being reset
 *
 * @return void
 */
void patWatchdog()
{
	if( ENABLE_EXTERNAL_WATCHDOG )
	{
		digitalWrite(WATCHDOG_PIN, HIGH);
		delay(WATCHDOG_PULSE_LENGTH);
		digitalWrite(WATCHDOG_PIN, LOW);
	}
	watchdogLastResetTime = millis();
}


/**
 * Main loop program
 *
 * @return void
 */
void loop() {
	// Stay connected to MQTT
	if (!client.connected())
	{
		reconnect();
	}

	runHeartbeat();
	client.loop();
}


/**
 * Reconnect to MQTT server
 *
 * @return void
 */
void reconnect()
{
	// Loop until we're reconnected
	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection: ");
		// Attempt to connect
		if (client.connect(mqttClientID, MQTT_USER, MQTT_PASSWD))
		{
			Serial.println("Connected");

			// Construct MQTT Client ID
			sprintf(clientBuffer, "%s Connected %02X%02X%02X%02X%02X%02X", mqttClientID, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			Serial.println(sizeof(clientBuffer));
			client.publish(mqttEventTopic, clientBuffer);

			// Construct MQTT topic
			sprintf(topicBuffer, "%s/+/%s", mqttTopicPrefix, mqttCmdTopic);

			Serial.print("Subscribing to MQTT topic: ");
			Serial.println(topicBuffer);

			client.subscribe(topicBuffer);
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}


/**
 * Initialise 1C2 shield
 *
 * @param int shieldAddress Address of shield
 * @return void
 */
void initialiseShield(int shieldAddress)
{
	// Set addressing style
	Wire.beginTransmission(shieldAddress);
	Wire.write(0x12);
	Wire.write(0x20); // use table 1.4 addressing
	Wire.endTransmission();

	// Set I/O bank A to outputs
	Wire.beginTransmission(shieldAddress);
	Wire.write(0x00); // IODIRA register
	Wire.write(0x00); // Set all of bank A to outputs
	Wire.endTransmission();
}


/**
 * Reads byte value from given register
 *
 * @param byte r The register
 * @return byte The value
 */
byte readRegister(byte r)
{
	unsigned char v;
	Wire.beginTransmission(MAC_I2C_ADDR);
	Wire.write(r);	// Register to read
	Wire.endTransmission();

	Wire.requestFrom(MAC_I2C_ADDR, 1); // Read a byte
	while(!Wire.available())
	{
		// Wait
	}
	v = Wire.read();
	return v;
}


/**
 * Set given channel latch to ON
 *
 * @param byte channelId The channel
 * @return void
 */
void setLatchChannelOn (byte channelId)
{
	if ( channelId >= 1 && channelId <= 8 )
	{
		byte shieldOutput = channelId;
		byte channelMask = 1 << (shieldOutput - 1);
		shield1BankA = shield1BankA | channelMask;
		sendRawValueToLatch(SHIELD_1_I2C_ADDRESS, shield1BankA);
	}
	else if ( channelId >= 9 && channelId <= 16 )
	{
		byte shieldOutput = channelId - 8;
		byte channelMask = 1 << (shieldOutput - 1);
		shield2BankA = shield2BankA | channelMask;
		sendRawValueToLatch(SHIELD_2_I2C_ADDRESS, shield2BankA);
	}
	else if ( channelId >= 17 && channelId <= 24 )
	{
		byte shieldOutput = channelId - 16;
		byte channelMask = 1 << (shieldOutput - 1);
		shield3BankA = shield3BankA | channelMask;
		sendRawValueToLatch(SHIELD_3_I2C_ADDRESS, shield3BankA);
	}
	else if ( channelId >= 25 && channelId <= 32 )
	{
		byte shieldOutput = channelId - 24;
		byte channelMask = 1 << (shieldOutput - 1);
		shield4BankA = shield4BankA | channelMask;
		sendRawValueToLatch(SHIELD_4_I2C_ADDRESS, shield4BankA);
	}
}


/**
 * Set given channel latch to OFF
 *
 * @param byte channelId The channel
 * @return void
 */
void setLatchChannelOff (byte channelId)
{
	if ( channelId >= 1 && channelId <= 8 )
	{
		byte shieldOutput = channelId;
		byte channelMask = 255 - ( 1 << (shieldOutput - 1));
		shield1BankA = shield1BankA & channelMask;
		sendRawValueToLatch(SHIELD_1_I2C_ADDRESS, shield1BankA);
	}
	else if ( channelId >= 9 && channelId <= 16 )
	{
		byte shieldOutput = channelId - 8;
		byte channelMask = 255 - ( 1 << (shieldOutput - 1));
		shield2BankA = shield2BankA & channelMask;
		sendRawValueToLatch(SHIELD_2_I2C_ADDRESS, shield2BankA);
	}
	else if ( channelId >= 17 && channelId <= 24 )
	{
		byte shieldOutput = channelId - 16;
		byte channelMask = 255 - ( 1 << (shieldOutput - 1));
		shield3BankA = shield3BankA & channelMask;
		sendRawValueToLatch(SHIELD_3_I2C_ADDRESS, shield3BankA);
	}
	else if ( channelId >= 25 && channelId <= 32 )
	{
		byte shieldOutput = channelId - 24;
		byte channelMask = 255 - ( 1 << (shieldOutput - 1));
		shield4BankA = shield4BankA & channelMask;
		sendRawValueToLatch(SHIELD_4_I2C_ADDRESS, shield4BankA);
	}
}


/**
 * Sends given value to latch
 *
 * @param int shieldAddress Address of shield
 * @param byte rawValue The value to send
 * @return void
 */
void sendRawValueToLatch(int shieldAddress, byte rawValue)
{
	Wire.beginTransmission(shieldAddress);
	Wire.write(0x12);				// Select GPIOA
	Wire.write(rawValue);		// Send value to bank A
	Wire.endTransmission();

	// Update status
	switch (shieldAddress)
	{
		case SHIELD_1_I2C_ADDRESS:
			shield1BankA = rawValue;
			break;

		case SHIELD_2_I2C_ADDRESS:
			shield2BankA = rawValue;
			break;

		case SHIELD_3_I2C_ADDRESS:
			shield3BankA = rawValue;
			break;

		case SHIELD_4_I2C_ADDRESS:
			shield4BankA = rawValue;
			break;
	}
}


/**
 * Processes messages from subscribed MQTT topics
 *
 * @param char* topic The incoming topic
 * @param byte* payload The incoming message
 * @param int length The length of payload
 * @return void
 */
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i=0;i<length;i++)
	{
		Serial.print((char)payload[i]);
	}
	Serial.println();

	// TODO: This should be more generic and awesome

	String tmpBuf = topic;
	Serial.println(tmpBuf);
	Serial.println(String(mqttTopicPrefix).length());
	Serial.println("Matched");

	if ((char)payload[0] == '1')
	{
		setLatchChannelOn(2);
		Serial.println("Relay 2 triggered ON");
		client.publish(mqttStatusTopic, '1');

	}
	else if ((char)payload[0] == '0')
	{
		setLatchChannelOff(2);
		Serial.println("Relay 2 triggered OFF");
		client.publish(mqttStatusTopic, '0');
	}
 } 
 
 
