# LEDStripLamp
Controlling a multi-sided lamp built with a WS281x-strip and a NodeMCU ESP32 Arduino via a matrix-keypad through MQTT.

## Overview 
This project consists of three parts:
- A Raspberry PI (or comparable micro-PC) to run the MQTT-server and the control-server for the lamp.
- A WS281x-strip wrapped around a frame, controlled by a NodeMCU ESP32 Arduino.
- An optional 16-key-matrix-keypad connected to an ESP8266-Arduino for a comfortable way to load pre-defined settings.

## I) Node-Server
Central control unit of the project. Listens for 

# Installation

## Basic software
- On your Raspberry PI: install, configure and run an MQTT-broker and install Node.js with npm.
- On your working station: install the Arduino IDE.

## Node-Server
- Clone this repository somewhere onto your Raspberry PI (you only need the ```Node-Server``` directory): ```git clone https://github.com/JoachimGoebgen/LEDStripLamp/```
- Configure your connection_conf.h in ```/LEDStripLamp/connection_conf.h``` as described below
- Install the Node.js-MQTT-client: ```npm install mqtt --save```
- Run the control-server: ```node /LEDStripLamp/Node-Server/node-server.js```
- If you want to make sure the server is up and running at all times, make yourself familiar with [PM2](http://pm2.keymetrics.io/)

## Arduino-LEDLamp and Arduino-Keypad
Clone this repository somewhere onto your working station: ```git clone https://github.com/JoachimGoebgen/LEDStripLamp/```

### Install FastLED as LED-control in Arduino IDE
Clone ```https://github.com/FastLED/FastLED``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/FastLED```

### Install PubSubClient as MQTT-client in Arduino IDE
Clone ```https://github.com/knolleary/pubsubclient``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/pubsubclient```

### Install NodeMCU ESP32 Board in Arduino IDE
- Clone ```https://github.com/espressif/arduino-esp32``` into ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32``` 
- Run ```git submodule update --init --recursive``` in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32```
- Double-click get.exe (windows) or run ```python2 get.py``` (linux) in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32/tools```
- (Re-)start Arduino IDE
- Select ```NodeMCU-32S``` under Tools -> Boards
- Finally, load the ```/LEDStripLamp/Arduino-LEDLamp/Arduino-LEDLamp.ino``` onto your ESP32-board, which will run your LED-strip.

### Install NodeMCU ESP8266 Board in Arduino IDE
- Clone ```https://github.com/espressif/arduino-esp32``` into ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32``` 
- Run ```git submodule update --init --recursive``` in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32```
- Double-click get.exe (windows) or run ```python2 get.py``` (linux) in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32/tools```
- (Re-)start Arduino IDE
- Select ```NodeMCU-32S``` under Tools -> Boards
- Finally, load the ```/LEDStripLamp/Arduino-LEDLamp/Arduino-LEDLamp.ino``` onto your ESP32-board, which will run your LED-strip.


## connection_conf.h	
This header-file is used as config for your sensible information like your WiFi-password.
Just create it in ```/LEDStripLamp/connection_conf.h``` and add the following lines:
```
#define DEBUG // the Arduino and the control-server print debug-statement if this flag is set
#define CLIENT_NAME   "some name"
#define WIFI_SSID   "your wifi name"
#define WIFI_KEY    "your wifi key"
#define MQTT_SRV_IP   "the IP your MQTT-server is running on"
#define MQTT_SRV_PORT   1833 //default
#define MQTT_COLOR_TOPIC    "some topic like /SmartHome/LEDStripLamp/color"
#define MQTT_SETTINGS_TOPIC   "some topic like /SmartHome/LEDStripLamp/settings"
#define MQTT_BRIGHTNESS_TOPIC   "some topic like /SmartHome/LEDStripLamp/brightness"
#define MQTT_LOADPRESET_TOPIC   "some topic like /SmartHome/LEDStripLamp/loadpreset"
#define MQTT_SAVEPRESET_TOPIC   "some topic like /SmartHome/LEDStripLamp/savepreset"
```

# Communication
This section defines the message-formats sent via MQTT. Each message has its own topic.

## COLOR Message
The COLOR-msg makes the lamp change its colors to the specified values. It is published by the control-server after a preset is loaded (received PRESET-msg) or the color of one side is changed (received SIDECOLOR-msg). It is received by the Arduino connected to the WS281x-strip.
- MQTT-Topic: ```MQTT_COLOR_TOPIC```
- Format: ```r g b r g b r g b r g b ...``` where each ```r g b```-triplet represents the RGB-color of one side of the lamp. Each r-/g-/b-value is in ```[00...FF]``` and is seperated to consecutive values by a whitespace.
- Example: ```255 255 255 255 0 0 0 147 147 0 0 0``` results in the lamp displaying white on side 1, red on side 2, yellow on side 3 and is off on side 4.

## SIDECOLOR Message
The SIDECOLOR-msg makes the lamp change one sides' color to the specified value. It can be published by the user/keypad and is received by the control-server, which afterwards publishes a COLOR-msg containing the new colors on all sides.
- MQTT-Topic: ```MQTT_COLOR_TOPICn``` where n is the number of the side to be changed
- Format: ```r g b``` where each r-/g-/b-value is in ```[0...255]``` and is seperated to consecutive values by a whitespace.
- Alternative Format: ```#RRGGBB``` where each RR-/GG-/BB-value is the hexadecimal representation of the r-/g-/b-color in ```[00...FF]```
- Example: ```0 255 0``` or ```#00FF00``` sent to ```/SmartHome/LEDStripLamp/color3``` results in the lamp displaying full green on side 3 whereas the colors on the other sides remain untouched.


## BRIGHTNESS Message
The BRIGHTNESS-msg makes the lamp change its brightness on all sides by a fix percentage. It can be published by the user/keypad and is received by the control-server, which afterwards publishes a COLOR-msg containing the previous colors but with changed brightness.
- MQTT-Topic: ```MQTT_BRIGHTNESS_TOPIC```
- Format: ```+``` or ```-``` indicating if the brightness should increase or decrease.
- Hint: The percentual change can be set in the ```node-server.js```. This message has no effect if one r-/g-/b-value is already 255 (```+```-msg), respective 0 (```-```-msg)


## SAVEPRESET Message
The SAVEPRESET-msg makes the server remember the current configuration (colors and mode) for later restoring. It can be published by the user and is received by the control-server, which afterwards saves the current configuration together with the given preset-ID and -name to the ```presets```-file
- MQTT-Topic: ```MQTT_SAVEPRESET_TOPIC```
- Format: ```ID name``` where ID is a number in ```[0...9]``` and name is an arbitrary string (optional).
- Example: ```5 sunset``` associates the current configuration with the number "5" and the name "sunset" and saves it to the disk.


## LOADPRESET Message
The LOADPRESET-msg makes the lamp restore the configuration (colors and mode) associated with the given ID or name. It can be published by the user/keypad and is received by the control-server, which afterwards publishes the saved configuration via a COLOR-msg.
- MQTT-Topic: ```MQTT_LOADPRESET_TOPIC```
- Format: ```ID``` or ```name``` where ID is a number in ```[0...9]``` and name is an arbitrary string, previously saved with a SAVEPRESET-msg.
- Example: ```5``` or ```sunset``` both restore the configuration that was saved in the SAVEPRESET-example.


# Troubleshooting

## Why using two different Arduinos?!
There were just two different Arduinos laying around when I started this project. It is theoretically possible to use ESP32/ESP8266 or other WiFi-supporting Arduinos for both the lamp-control and the keypad-control. Notice that the ESP8266 made my WS281x-strip flicker, no matter how hard I tried to fix it so you might be better off with the newer ESP32.

## Espressifs Wifi.h is colliding with Arduinos Wifi.h when compiling?!
It is important to name the ESP32_DIR ```esp32``` and not ```arduino-esp32``` as it will be named automatically. See [this thread](https://github.com/espressif/arduino-esp32/issues/20) for further information.

## My LED-Strip is flickering?!
See [this thread](https://github.com/FastLED/FastLED/issues/306) for problem with flickering LEDs.
