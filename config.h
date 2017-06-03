// Setup
#define DEFAULT_SSID      "ssid"           // Enter your network SSID
#define DEFAULT_PASSWORD  "pwd"            // Enter your network WPA key
#define DEFAULT_HOSTNAME  "maindoor"       // Enter your device friendly name

#define MQTT_SERVER       "server"
#define MQTT_USER         "user"
#define MQTT_PASSWORD     "pwd"

#define REED_TOPIC        "maindoor/reed/switch"
#define BATTERY_TOPIC     "maindoor/battery/analog"
#define ARM_TOPIC         "maindoor/arm/cmd"

#define VCC_ADJ  1.225

// Pin Setup
#define REEDPIN 4     // D2
#define BUZZERPIN 14  // D5
#define BUTTONPIN 0   // D3

// Debugging
//#define DEBUG
