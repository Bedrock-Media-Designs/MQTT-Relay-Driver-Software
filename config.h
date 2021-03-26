/* ------------------ Your specific config -------------------------- */



/* ----------------- Network config --------------------------------- */
#define ENABLE_MAC_ADDRESS_ROM      true
#define MAC_I2C_ADDR                0x50
#define ENABLE_DHCP                 true
byte ardunio_mac[] = {0xDE, 0x51, 0xAF, 0x24, 0x3E, 0x4E};
IPAddress ardunio_ip(XXX, XXX, XXX, XXX);



/* ----------------- MQTT config ------------------------------------ */
IPAddress mqtt_broker(XXX, XXX, XXX, XXX);   //IP address of your MQTT Broker
String mqtt_name = "relay";
const char mqtt_user[] = "USERNAME";         //MQTT UserName
const char mqtt_pass[] = "PASSWORD";         //MQTT Password
const char mqtt_status_topic[] = "status";
const char mqtt_sub[] = "house/switchboard/01/relay/+";



/* ----------------- Watchdog config -------------------------------- */
#define ENABLE_WATCHDOG         true
const char watchdog_pin = 3;



/* ----------------- Controller config -------------------------------- */
// Remove line if only using 8 relays per I2C chip, else using 16
#define RELAY_16                true

// If or when you're using multiple relay controllers
const byte controller_id = 1;

// Interval to report heartbeat
// Set time in ms for Heartbeat 60000 = 1Min
#define HEARTBEAT               60000