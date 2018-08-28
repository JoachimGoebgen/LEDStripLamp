# LEDStripLamp
Controlling a multi-sided lamp built with a WS281x-strip and a NodeMCU ESP32 Arduino via MQTT and an optional matrix-keypad.

## Overview 
This project consists of three parts:
- A Raspberry PI (or comparable micro-PC) to run the MQTT-server and the control-server for the lamp.
- A WS281x-strip wrapped around a frame, controlled by a NodeMCU ESP32 Arduino.
- An optional 16-key-matrix-keypad connected to an ESP8266-Arduino for a comfortable way to load pre-defined settings.

## I) Node-Server
Central control unit of the project. Listens for 

# Installation

## Installing FastLED as LED-control in Arduino IDE
Clone ```https://github.com/FastLED/FastLED``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/FastLED```

## Installing PubSubClient as MQTT-client in Arduino IDE
Clone ```https://github.com/knolleary/pubsubclient``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/pubsubclient```

## Installing NodeMCU ESP32 Board in Arduino IDE
- Clone ```https://github.com/espressif/arduino-esp32``` into ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32``` 
- Run ```git submodule update --init --recursive``` in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32```
- Double-click get.exe (windows) or run ```python2 get.py``` (linux) in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32/tools```
- (Re-)start Arduino IDE
- Select ```NodeMCU-32S``` under Tools -> Boards

## Creating connection_conf.h	
This header-file is used as config for your sensible information like your WiFi-password.
Just create it in the ```LEDStripLampArduino```-folder and add the following lines:
```
#define CLIENT_NAME		"some name"
#define WIFI_SSID		"your wifi name"
#define WIFI_KEY		"your wifi key"
#define MQTT_SRV_IP		"the IP your MQTT-server is running on"
#define MQTT_SRV_PORT		1833 //default
#define MQTT_TOPIC		"some topic like /SmartHome/LEDStripLamp"
```

# Communication

## MQTT_COLOR_TOPIC (color-msg)
The color-msg makes the lamp change its colors to the specified values. It is either sent by the control-server after loading a preset or manually by the user.

Format: ```r g b r g b ...``` where one ```r g b```-triplet represents the RGB-color of one side of the lamp. Each value has to be between 0 and 255 (one byte) and is seperated to consecutive values by a whitespace.

Example: ```255 255 255 255 0 0 0 0 147 147 0 0 0``` results in the lamp displaying white on side 1, red on side two, yellow on side 4 and is off on side 4.

# Troubleshooting

## Espressifs Wifi.h is colliding with Arduinos Wifi.h when compiling?!
It is important to name the ESP32_DIR ```esp32``` and not ```arduino-esp32``` as it will be named automatically. See [this thread](https://github.com/espressif/arduino-esp32/issues/20) for further information.

## My LED-Strip is flickering?!
See [this thread](https://github.com/FastLED/FastLED/issues/306) for problem with flickering LEDs.
