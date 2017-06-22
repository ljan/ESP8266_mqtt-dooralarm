# ESP8266 MQTT Door-Alarm
Battery powered dooralarm with an ESP8266

ESP peridically checks if a reed switch is open and goes back to deepsleep. If the reed switch is open it will connect to te WiFI and will publish to a MQTT server until the reed switch is closed again.

Designed to use the cheap [ESP-01 Module](https://github.com/esp8266/esp8266-wiki/wiki/Hardware_versions#esp-01-versions-1-and-2). For normal Boot GPIO0 and GPIO2 need a Pull-Up, also Chip_En needs to be connected to VCC.
To achieve deep sleep mode the Power-LED has to be disconnected and an extra Connection between GPIO16 and Reset hast to be made.
