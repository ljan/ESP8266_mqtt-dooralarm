# ESP8266 MQTT Door-Alarm
Battery powered dooralarm with an ESP8266

ESP peridically checks if a reed switch is open and goes back to deepsleep. If the reed switch is open it will connect to te WiFI and will publish to a MQTT server until te reed switch is closed again.
