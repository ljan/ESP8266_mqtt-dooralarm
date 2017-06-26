#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "config.h"
#include "debug.h"

const int sleepTimeS = 1;       // Time in Deep-Sleep between checks
unsigned long loopTimeS = 5;    // Time between checks when door is open
volatile int buttoncounter = 0;
volatile bool alarm = false;
volatile bool unarm = false;
volatile bool check = false;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

ADC_MODE(ADC_VCC);  // Read internal vcc rather than voltage on ADC pin (A0 must be floating)

// *******SETUP*******
// periodically check if door is open and go back to sleep, when open go to loop()
void setup()
{
  dbserialbegin(115200);
  dbprintln("");
  dbprintln("");
  dbprintln("BOOT");
  dbprint("VCC Voltage: ");
  dbprintln(ESP.getVcc()*VCC_ADJ/1024.00f);

  // pin setup
  pinMode(REEDPIN, INPUT);
  pinMode(BUZZERPIN, OUTPUT);
  digitalWrite(BUZZERPIN, HIGH);
  pinMode(BUTTONPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTONPIN), button_ISR, FALLING);

  dbprint("Reed: ");
  if(digitalRead(REEDPIN))
  {
    dbprintln("closed, going back to sleep");
    gotodeepsleep(sleepTimeS); // 86 ms to get here
  }
  
  dbprintln("open, begin loop");
  
  // start wifi
  setup_wifi();
  
  // setup mqtt
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback_mqtt);
}
//*************** end setup *****************************

// *******LOOP*******
// start wifi, connect mqtt and check if arm_topic is set, loop till door is closed again and publish "1" to reed_topic
// when armed toggle buzzer, falling edge on button unarms, 5 times pressed ota is initiated
// after door is closed publish "0" to reed_topic and batteryvoltage to battery_topic and go back to sleep
void loop()
{
  // connect mqtt
  if (!mqttClient.connected())
  {
    reconnect_mqtt();
    mqttClient.subscribe(ARM_TOPIC);
  }
  
  // loop until door is closed
  unsigned long last = millis()+(loopTimeS*1e3);
  unsigned long buzz = millis();
  do
  {
    if(millis()-last > loopTimeS*1e3)
    {
      last = millis();
      if(!digitalRead(REEDPIN))
      {
        dbprintln("still open");
        mqttClient.publish(REED_TOPIC, "open", false);
      }
    }
    
    if(alarm && millis()-buzz > 100)
    {
      buzz = millis();
      dbprint("buzz ");
      digitalWrite(BUZZERPIN, !digitalRead(BUZZERPIN));
    }

    mqttClient.loop();
    yield();
    delay(20);
  } while(!digitalRead(REEDPIN) || alarm || !check);
    
  if(unarm)
  {
    mqttClient.publish(ARM_TOPIC, "0", true);
  }

  digitalWrite(BUZZERPIN, HIGH);
  dbprintln("door closed, no alarm");
  mqttClient.publish(REED_TOPIC, "closed", false);
  mqttClient.publish(BATTERY_TOPIC, String(ESP.getVcc()*VCC_ADJ/1024.00).c_str(), false);
  mqttClient.loop();
  
  if(buttoncounter > 5)
  {
    mqttClient.publish("maindoor/debug/status", "OTA start", false);
    dbprintln("Button at least 5 times pressed, starting OTA");
    setup_ota();
    last = millis();
    while(millis()-last < 60*1e3)
    {
      ArduinoOTA.handle();
      yield();
      delay(200);
    }
    dbprintln("waited 60 seconds, ending OTA");
    mqttClient.publish("maindoor/debug/status", "OTA end", false);
  }
  
  yield();
  delay(100);
  gotodeepsleep(sleepTimeS);
}
//*************** end main *****************************

void button_ISR()
{
  if(alarm)
  {
    unarm = true;
    alarm = false;
  }
  buttoncounter++;
}

void setup_wifi()
{
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
  would try to act as both a client and an access-point and could cause
  network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.hostname(DEFAULT_HOSTNAME + String("-") + String(ESP.getChipId(), HEX));  
  WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD);
  
  dbprint("Connecting to ");
  dbprint(DEFAULT_SSID);

  /* This function returns following codes to describe what is going on with Wi-Fi connection: 
  0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  1 : WL_NO_SSID_AVAIL in case configured SSID cannot be reached
  3 : WL_CONNECTED after successful connection is established
  4 : WL_CONNECT_FAILED if password is incorrect
  6 : WL_DISCONNECTED if module is not configured in station mode
  Serial.printf( "Connection status: %d\n", WiFi.status() ); */
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(200); // pauses the sketch for a given number of milliseconds and allows WiFi and TCP/IP tasks to run
    dbprint(".");
  }
  dbprint("WiFi connected, IP address: ");
  dbprintln(WiFi.localIP());
}

void setup_ota()
{
//    ArduinoOTA.setHostname(DEFAULT_HOSTNAME + String("-") + String(ESP.getChipId(), HEX));
//    ArduinoOTA.setPassword((const char *)"esp8266");
    
    ArduinoOTA.onStart([]()
    {
      dbprintln("Start");
    });
    ArduinoOTA.onEnd([]()
    {
      dbprintln("\nEnd");
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
      dbprint("Error[%u]: ");dbprintln(error);
      if (error == OTA_AUTH_ERROR) dbprintln("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) dbprintln("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) dbprintln("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) dbprintln("Receive Failed");
      else if (error == OTA_END_ERROR) dbprintln("End Failed");
    });
    ArduinoOTA.begin();
    dbprintln("Ready - OTA Success!!!");
}

void reconnect_mqtt()
{
  // Loop until door is closed
  while (!mqttClient.connected())
  {
    dbprint("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (mqttClient.connect("ESP8266Client"))
    if (mqttClient.connect(DEFAULT_HOSTNAME, MQTT_USER, MQTT_PASSWORD))
    {
      dbprintln("connected");
    } else
    {
      dbprint("failed, rc=");
      dbprint(mqttClient.state());
      dbprintln(" try again in 5 seconds");
      // Wait 5 seconds before retrying
//      delay(5000);
      gotodeepsleep(5);
    }
  }
}

void callback_mqtt(char* topic, byte* payload, unsigned int length)
{
  dbprint("Message arrived [");
  dbprint(topic);
  dbprint("] ");
  for (int i = 0; i < length; i++) {
    dbprint((char)payload[i]);
  }
  dbprintln("");

  if((char)payload[0] == '1')
  {
    alarm = true;
    dbprintln("alarm");
  }
  else
  {
    alarm = false;
  }
  check = true;
}

void gotodeepsleep(int sleeptime)
{
/*ESP.deepSleep(microseconds, mode) will put the chip into deep sleep.
  mode is one of WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED.
  (GPIO16 needs to be tied to RST to wake from deepSleep.)*/
  #ifdef DEBUG
    ESP.deepSleep(sleeptime*1e6,WAKE_RF_DEFAULT);
    yield();
    delay(100);
//    delay(sleeptime*1e3);
//    ESP.reset();
  #else
    ESP.deepSleep(sleeptime*1e6,WAKE_RF_DEFAULT);
    yield();
    delay(100);
  #endif
}

// EOF
