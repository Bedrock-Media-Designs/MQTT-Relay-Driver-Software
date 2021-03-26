#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_MCP23017.h>


/**
 * Topic: relays/{{relay-number: 1-99}}
 * Payload relay on: 1
 * Payload relay off: 0
 */



/*--------------------------- CONFIGURATION ------------------------------*/
// Configuration should be done in the included file:
#include "config.h"



/*--------------------------- SETUP -------------------------------------*/
EthernetClient network_client;

PubSubClient mqtt_client(network_client);

Adafruit_MCP23017 mcps[6];

unsigned long watchdog_timer = 0;
unsigned long mqtt_timer = 0;
unsigned long heart_timer = 0;
char mqtt_buffer[100];
/*------------------------------------------------------------------------*/



/**
 * Determine if given timer has reached given interval
 *
 * @param unsigned long tmr The current timer
 * @param unsigned long interval Length of time to run timer
 * @return bool True when timer is complete
 * @return bool False when timer is counting
 */
bool getTimer(unsigned long &tmr, unsigned long interval)
{
  // Set initial value
  if (tmr < 1)
  {
    tmr = millis();
  }

  // Determine difference of our timer against millis()
  if (millis() - tmr >= interval)
  {
    // Complete. Reset timer
    tmr = 0;
    return true;
  }

  // Still needs to "count"
  return false;
}



/**
 * Converts IP address to char array
 *
 * @param IPAddress address
 * @return char*
 */
char *ip2CharArray(IPAddress address)
{
  static char ip[16];
  sprintf(ip, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
  return ip;
}



/**
 * Switch given relay ON or OFF
 *
 * @param byte relay_no The relay to switch
 * @param bool is_on True for ON, False for OFF
 * @return void
 */
void turnRelayOnOrOff(byte relay_no, bool is_on)
{
  #ifdef RELAY_16
    byte shield_number = ceil(((int)relay_no - 1) / 16);
    byte relay_number = (int)relay_no % 16;
    if ((int)relay_number == 0) relay_number = 16;
  #else
    byte shield_number = ceil(((int)relay_no - 1) / 8);
    byte relay_number = (int)relay_no % 8;
    if ((int)relay_number == 0) relay_number = 8;
  #endif

  mcps[(uint16_t)shield_number].digitalWrite((uint16_t)relay_number-1, (uint16_t) is_on);
}



/**
 * Reconnect to MQTT broker
 * @return bool True on successful connection
 * @return bool False on failure to connect
 */
bool mqttReconnect()
{
  // Too early to reconnect
  if (!getTimer(mqtt_timer, 1000))
  {
    return false;
  }

  // Connection successful
  if (mqtt_client.connect(String(mqtt_name + String(random(0xffff), HEX)).c_str(), mqtt_user, mqtt_pass))
  {
    // Publish json-string payload of connection status: event type, local IP, controller ID
    sprintf(mqtt_buffer, "{\"event\":\"reconnected\",\"ip\":\"%s\",\"device\":\"relay%d\"}", ip2CharArray(Ethernet.localIP()), controller_id);
    mqtt_client.publish(mqtt_status_topic, mqtt_buffer);
    mqtt_client.subscribe(mqtt_sub);

    return true;
  }

  // Connection failed
  return false;
}



/**
 * Receive MQTT message
 *
 * @param char* topic The incomnig topic
 * @param byte* payload The incoming message
 * @param int leng Length of payload
 * @return void
 */
void mqttReceive(char *topic, byte *payload, unsigned int leng)
{
  turnRelayOnOrOff(getRelayFromTopic(topic), ((char)payload[0] == '1'));
}



/**
 * Reset watch-dog module
 * Prevents Arduino from resetting if everything is fine
 *
 * @return void
 */
#ifdef ENABLE_WATCHDOG
void resetWatchDog()
{
  if (getTimer(watchdog_timer, 30000))
  {
    digitalWrite(watchdog_pin, HIGH);
    delay(20);
    digitalWrite(watchdog_pin, LOW);
  }
}
#endif



/**
 * Reads byte value from given register
 *
 * @param byte reg The register
 * @return byte The value at given register
 */
byte readRegister(byte reg)
{
  Wire.beginTransmission(MAC_I2C_ADDR);

  // Register to read
  Wire.write(reg);
  Wire.endTransmission();

  // Read a byte
  Wire.requestFrom(MAC_I2C_ADDR, 1);
  while (!Wire.available())
  {
    // Wait
  }

  return Wire.read();
}



/**
 * Fetch relay number from topic
 *
 * @param String topic The given topic
 * @return int Relay number
 */
int getRelayFromTopic(const String &topic)
{
  int s, e = -1;
  for (s = topic.length() - 1; s > 0; --s)
  {
    if (isDigit(topic[s]))
    {
      if (e == -1)
        e = s;
    }
    else if (e != -1)
      break;
  }
  return topic.substring(s + 1, e + 1).toInt();
}



/**
 * Initial setup
 *
 * @return void
 */
void setup()
{
  Wire.begin();
  Serial.begin(9600);

  #ifdef ENABLE_WATCHDOG
    pinMode(watchdog_pin, OUTPUT);
    digitalWrite(watchdog_pin, LOW);
  #endif

  #ifdef ENABLE_MAC_ADDRESS_ROM
    ardunio_mac[0] = readRegister(0xFA);
    ardunio_mac[1] = readRegister(0xFB);
    ardunio_mac[2] = readRegister(0xFC);
    ardunio_mac[3] = readRegister(0xFD);
    ardunio_mac[4] = readRegister(0xFE);
    ardunio_mac[5] = readRegister(0xFF);
  #endif

  #ifdef ENABLE_DHCP
    // Wait for network connection
    while (Ethernet.begin(ardunio_mac) == 0)
    {
      delay(1000);
    }
  #else
    Ethernet.begin(ardunio_mac, ardunio_ip);
  #endif

  // Initializes the pseudo-random number generator
  randomSeed(micros());

  // Setup MQTT
  mqtt_client.setServer(mqtt_broker, 1883);
  mqtt_client.setCallback(mqttReceive);

  // Setup pins
  for (signed char i = 0; i < 6; i++)
  {
    mcps[i].begin(i);

    for (signed char p = 0; p < 16; p ++)
    {
      mcps[i].pinMode(p, OUTPUT);
    }
  }
}



/**
 * Main program loop
 *
 * @return void
 */
void loop()
{
  // Keep network connection
  Ethernet.maintain();

  // Reconnect to MQTT server
  if (!mqtt_client.connected())
  {
    if (!mqttReconnect())
    {
      // Exit early of main loop
      return;
    }
  }

  // Check for incoming MQTT messages
  mqtt_client.loop();

  // Prevent arduino from resetting if everything is fine
  #ifdef ENABLE_WATCHDOG
    resetWatchDog();
  #endif

  // Heartbeat report
  if (getTimer(heart_timer, HEARTBEAT))
  {
      // Publish json-string payload of connection status: event type, local IP, controller ID
      sprintf(mqtt_buffer, "{\"event\":\"status\",\"ip\":\"%s\",\"device\":\"relay%d\"}", ip2CharArray(Ethernet.localIP()), controller_id);
      mqtt_client.publish(mqtt_status_topic, mqtt_buffer);
  }
}