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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

 

#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>


/*--------------------------- Configuration ------------------------------*/
/* Core Settings */
#define SERIAL_SPEED            57600   // Serial port speed

/* Network Settings */
#define ENABLE_DHCP             true    // true/false
#define ENABLE_MAC_ADDRESS_ROM  true    // true/false
#define MAC_I2C_ADDR            0x50    // Microchip 24AA125E48 I2C ROM address
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // Set if no MAC ROM
IPAddress ip(192,168,1,35);             // Default if DHCP is not used

/* MQTT Settings */
IPAddress broker(192,168,1,108);            // MQTT broker
#define MQTT_USER         "mqtt"          // Username presented to the MQTT broker
#define MQTT_PASSWD       "Secret"      // Password presented to the MQTT broker
#define MQTT_TOPIC_BUFFER  40
char clientBuffer[40];                    // Maximum clientID length
char messageBuffer[100];  // TODO: unused ATM
char topicBuffer[35];                     // Maximum topic length
const char* mqttClientID      = "ArduinoClient";  // Client ID presented to the MQTT broker
const char* mqttEventTopic    = "events";         // MQTT logging topic
const char* mqttTopicPrefix   = "house/switchboard/12/relay"; // MQTT topic prefix Change the Switchboard ID Eg 12. if you have more then 1 panel.
const char* mqttCmdTopic      = "command";        // MQTT command topic suffix
const char* mqttStatusTopic   = "status";         // MQTT status topic suffix

/* Watchdog Timer Settings */
#define ENABLE_EXTERNAL_WATCHDOG        true       // true / false
#define WATCHDOG_PIN                    8          // Output to pat the watchdog
#define WATCHDOG_PULSE_LENGTH           50         // Milliseconds
#define WATCHDOG_RESET_INTERVAL         30000      // Milliseconds. Also the period for sensor reports.
long watchdogLastResetTime = 0;

/* Relay 8 Modules */
const int RLY8_I2C_ADDR[4] = {0x20, 0x21, 0x22, 0x23};
byte BankA[4] = {0, 0, 0, 0}; // Current status of all outputs on first shield, one bit per output

/*------------------------------------------------------------------------*/

// Instantiate MQTT client
EthernetClient ethclient;
PubSubClient client(ethclient);

void setup() {
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
  } else {
    Serial.print(F("Using static MAC address: "));
  }
  // Print the MAC address
  char macAddr[17];
  sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macAddr);

  // Set up the Ethernet library to talk to the Wiznet board
  if( ENABLE_DHCP == true )
  {
    Ethernet.begin(mac);      // Use DHCP
  } else {
    Ethernet.begin(mac, ip);  // Use static address defined above
  }
  
  // Print IP address:
  Serial.print(F("My IP: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
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
  initRelay8(RLY8_I2C_ADDR[0]);

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

void runHeartbeat()
{
  if((millis() - watchdogLastResetTime) > WATCHDOG_RESET_INTERVAL)  // Is it time to run yet?
  {
   
    {
      patWatchdog();  // Only pat the watchdog if we successfully published to MQTT
    }
 
  }
}

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
void loop() {
  // Stay connected to MQTT
  if (!client.connected()) {
    reconnect();
  }
  runHeartbeat();
  client.loop();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection: ");
    // Attempt to connect
    if (client.connect(mqttClientID, MQTT_USER, MQTT_PASSWD)) {
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
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void initRelay8(int i2c_addr) {
  // Set I/O bank A to outputs
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
}

byte readRegister(byte r)
{
  unsigned char v;
  Wire.beginTransmission(MAC_I2C_ADDR);
  Wire.write(r);  // Register to read
  Wire.endTransmission();

  Wire.requestFrom(MAC_I2C_ADDR, 1); // Read a byte
  while(!Wire.available())
  {
    // Wait
  }
  v = Wire.read();
  return v;
}

void setLatchChannelOn (byte channelId)
{
  if ( channelId >= 1 && channelId <= 8 )
  {
    byte shieldOutput = channelId;
    byte channelMask = 1 << (shieldOutput - 1);
    BankA[0] = BankA[0] | channelMask;
    sendRawValueToLatch(BankA[0], RLY8_I2C_ADDR[0]);
  }
  else if ( channelId >= 9 && channelId <= 16 )
  {
    byte shieldOutput = channelId - 8;
    byte channelMask = 1 << (shieldOutput - 1);
    BankA[1] = BankA[1] | channelMask;
    sendRawValueToLatch(BankA[1], RLY8_I2C_ADDR[1]);
  }
}

void setLatchChannelOff (byte channelId)
{
  if( channelId >= 1 && channelId <= 8 )
  {
    byte shieldOutput = channelId;
    byte channelMask = 255 - ( 1 << (shieldOutput - 1));
    BankA[0] = BankA[0] & channelMask;
    sendRawValueToLatch(BankA[0], RLY8_I2C_ADDR[0]);
  }
  else if( channelId >= 9 && channelId <= 16 )
  {
    byte shieldOutput = channelId - 8;
    byte channelMask = 255 - ( 1 << (shieldOutput - 1));
    BankA[1] = BankA[1] & channelMask;
    sendRawValueToLatch(BankA[1], RLY8_I2C_ADDR[1]);  // TODO: This looks redundant and could be better
  }
}

void sendRawValueToLatch(byte rawValue, int i2cAddress)
{
  Wire.beginTransmission(i2cAddress);
  Wire.write(0x12);        // Select GPIOA
  Wire.write(rawValue);    // Send value to bank A
  Wire.endTransmission();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // TODO: This should be more generic and awesome

  String tmpBuf = topic;
  Serial.println(tmpBuf);
  Serial.println(String(mqttTopicPrefix).length());
  //if (strcmp(topic, "relay1") == 0) {
  if (true) {
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
}
