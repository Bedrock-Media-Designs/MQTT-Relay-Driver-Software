#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>


/* NETWORK SETTINGS */
#define ENABLE_MAC_ADDRESS_ROM      true
#define MAC_I2C_ADDR          0x50
static uint8_t device_mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient network_device;


/* MQTT SETTINGS */
IPAddress mqtt_broker(XXX,XXX,XXX,XXX); // replace XXX,XXX,XXX,XXX with your ip like 192,168,1,108
PubSubClient mqtt(network_device);

String mqtt_client = "relays_";
const char mqtt_user[] = "username";
const char mqtt_pass[] = "password";
const char mqtt_sub[] = "house/switchboard/01/relay/+/command";
unsigned long mqtt_timmer = 0;


/* WATCH DOG SETTINGS */
#define ENABLE_EXTERNAL_WATCHDOG    true   // set this to false if you dont have a watchdog
#define WATCHDOG_PIN          8
bool watchdog_status = false;
unsigned long watchdog_timmer = 0;


/* RELAY SETTINGS */
const PROGMEM byte ic2_addresses[8] = {32,33,34,35,36,37,38,39};
uint16_t shield_statuses[8] = {0,0,0,0,0,0,0,0};



/**
 * Initial setup - called once
 *
 * @return void
 */
void setup ()
{
  // Enable watchdog
  if (ENABLE_EXTERNAL_WATCHDOG == true)
  {
    pinMode(WATCHDOG_PIN, OUTPUT);
    digitalWrite(WATCHDOG_PIN, LOW);
  }

  Wire.begin();

  // Obtain MAC address from ROM
  if (ENABLE_MAC_ADDRESS_ROM == true)
  {
    device_mac[0] = readRegister(0xFA);
    device_mac[1] = readRegister(0xFB);
    device_mac[2] = readRegister(0xFC);
    device_mac[3] = readRegister(0xFD);
    device_mac[4] = readRegister(0xFE);
    device_mac[5] = readRegister(0xFF);
  }

  // Wait for network connection
  while (Ethernet.begin(device_mac) == 0)
  {
    delay(1000);
  }

  // Initializes the pseudo-random number generator
  randomSeed(micros());

  // Configure MQTT
  mqtt_client = String(mqtt_client + device_mac[5]);
  mqtt.setServer(mqtt_broker, 1883);
  mqtt.setCallback(mqttCallback);

  // Initialise relay shields
  for (int i = 0; i < 8; i++)
  {
    initialiseShield(pgm_read_word(ic2_addresses + i));
  }
}



/**
 * Main loop program
 *
 * @return void
 */
void loop ()
{
  // Keep network connection
  Ethernet.maintain();

  // Reconnect to MQTT server
  if (!mqtt.connected())
  {
    if (!mqttReconnect())
    {
      // Exit early of loop
      return;
    }
  }

  // Check for incoming MQTT messages
  mqtt.loop();

  // Prevent arduino from resetting if everything is fine
  if (ENABLE_EXTERNAL_WATCHDOG == true)
  {
    resetWatchDog();
  }
}



/**
 * MQTT reconnect
 * @return bool True on successful connection
 * @return bool False on failure to connect
 */
bool mqttReconnect ()
{
  // Only attempt every 1 seconds to prevent congesting the network
  if (!getTimer(mqtt_timmer, 1000))
  {
    return false;
  }

  // Attempt to connect with random client name
  if (mqtt.connect(String(mqtt_client + String(random(0xffff), HEX)).c_str(), mqtt_user, mqtt_pass))
  {
    // Subscribe to topic
    mqtt.subscribe(mqtt_sub);

    // Connection was successful
    return true;
  }

  // Still unable to connect
  return false;
}


/**
 * Processes messages from subscribed MQTT topics
 *
 * @param char* topic The incoming topic
 * @param byte* payload The incoming message
 * @param int length The length of payload
 * @return void
 */
void mqttCallback (char* topic, byte* payload, unsigned int length)
{
  turnRelayOnOrOff(getRelayFromTopic(topic), ((char)payload[0] == '1'));
}



/**
 * Initialise 1C2 shield
 *
 * @param int shield_address Shield address value
 * @return void
 */
void initialiseShield (int shield_address)
{
    // Set I/O bank A to outputs
    Wire.beginTransmission(shield_address);
    Wire.write(0x00); // IODIRA register
    Wire.write(0x00); // Set all of bank A to outputs
    Wire.endTransmission();
}


/**
 * Reads byte value from given register
 *
 * @param byte reg The register
 * @return byte The value
 */
byte readRegister (byte reg)
{
  Wire.beginTransmission(MAC_I2C_ADDR);
  // Register to read
  Wire.write(reg);
  Wire.endTransmission();

  // Read a byte
  Wire.requestFrom(MAC_I2C_ADDR, 1);
  while(!Wire.available())
  {
    // Wait
  }

  return Wire.read();
}


/**
 * Sends given value to latch
 *
 * @param int shield_address Zero indexed shield address lookup
 * @param uint_16t rawValue The value to send
 * @return void
 */
void sendRawValueToLatch (int shield_address, uint16_t raw_value)
{
  Wire.beginTransmission(shield_address);
  Wire.write(0x12);
  Wire.write(raw_value & 0xFF); // Assume the bank A has relays 1-8
  Wire.endTransmission();
}



/**
 * Switch given relay ON or OFF
 *
 * @param byte relay_no The relay to switch
 * @param bool is_on True for ON, False for OFF
 * @return void
 */
void turnRelayOnOrOff (byte relay_no, bool is_on)
{
  #ifdef RELAY_16
    byte shield_number = ceil(((int) relay_no -1 ) / 16);
    byte relay_number = (int) relay_no % 16;
    if ((int) relay_number == 0) relay_number = 16;
  #else
    byte shield_number = ceil(((int) relay_no - 1) / 8);
    byte relay_number = (int) relay_no % 8;
    if ((int) relay_number == 0) relay_number = 8;
  #endif

  byte shield_i2c_address = pgm_read_word(ic2_addresses + shield_number);
  uint16_t bit_mask = 1 << ((int) relay_number - 1);

  if (is_on)
  {
    shield_statuses[shield_number] |= bit_mask;
  }
  else
  {
    shield_statuses[shield_number] &= ~bit_mask;
  }

  sendRawValueToLatch(shield_i2c_address, shield_statuses[shield_number]);
}



/**
 * Determine if given timer has reached given interval
 *
 * @param unsigned long tmr The current timer
 * @param int interval Length of time to run timer
 * @return bool True when timer is complete
 * @return bool False when timer is counting
 */
bool getTimer (unsigned long &tmr, int interval)
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
 * Fetch relay number from topic
 *
 * @param String topic The given topic
 * @return int Relay number
 */
int getRelayFromTopic(const String &topic)
{
  int s, e = -1;
  for (s = topic.length() -1; s > 0; --s)
  {
    if (isDigit(topic[s]))
    {
      if (e == -1) e = s;
    }
    else if (e != -1) break;
  }
  return topic.substring(s + 1, e + 1).toInt();
}



/**
 * Reset watch-dog module
 * Prevents Arduino from resetting if everything is fine
 *
 * @return void
 */
void resetWatchDog()
{
  if (getTimer(watchdog_timmer, 1000))
  {
    watchdog_status = !watchdog_status;
  }

  digitalWrite(WATCHDOG_PIN, watchdog_status);
}
