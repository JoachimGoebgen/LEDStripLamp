# LEDStripLamp
Controlling a multi-sided lamp built with a WS281x-strip and a NodeMCU ESP32 Arduino through MQTT.  Feel free to take this as a basis for your own project with LED-strips that shall be controlled via MQTT and a keypad.

This project consists of two parts:
- The lamp which is a WS281x-strip wrapped around a frame, controlled by a NodeMCU ESP32 Arduino.
- An optional 16-key matrix-keypad connected to an ESP8266-Arduino for a comfortable way to load pre-defined settings (you can of course control it with any piece of software which is able to send MQTT-messages).

![Lamp](https://i.imgur.com/BPRM2Qz.jpg)

# Installation
This section provides short setup instructions only to get the all the software parts up and running. This is neither a full step-by-step guide, nor includes the hardware stuff (building the lamp, wiring, ...).

## Install basic software
- If not already existing, (set up an MQTT-server](https://lmgtfy.com/?q=how+to+set+up+mqtt+server) somewhere on your network or use a public one.
- Install the Arduino IDE on your working station.
- Clone this repo: ```git clone https://github.com/JoachimGoebgen/LEDStripLamp/```
- Configure your connection_conf.h as described below.

### Configure connection_conf.h
This header-file is used as config for your sensible information like your WiFi-password.
Just rename the example-file to ```connection_conf.h``` and edit it or copy the following lines:
```
#define DEBUG // the Arduino prints debug-statements to the Serial-output if this flag is set
#define CLIENT_NAME   "some name"
#define WIFI_SSID   "your wifi name"
#define WIFI_KEY    "your wifi key"
#define MQTT_SRV_IP   "the IP your MQTT-server is running on"
#define MQTT_SRV_PORT   1833 //default
#define BRIGHTNESS_STEP_PERC	0.1
#define MQTT_COLOR_TOPIC    "some topic like /SmartHome/LEDStripLamp/color"
#define MQTT_SETTINGS_TOPIC   "some topic like /SmartHome/LEDStripLamp/settings"
#define MQTT_BRIGHTNESS_TOPIC   "some topic like /SmartHome/LEDStripLamp/brightness"
#define MQTT_LOADPRESET_TOPIC   "some topic like /SmartHome/LEDStripLamp/loadpreset"
#define MQTT_SAVEPRESET_TOPIC   "some topic like /SmartHome/LEDStripLamp/savepreset"
```

### Configure the parameters of the LED-Strip
The parameters for your individually built lamp are directly configured in the ```/LEDStripLamp/Arduino-Keypad/Arduino-Keypad.ino```. The following parameters fit for my setup with a 5m 60LEDs/m WS2812-strip wrapped around a square frame with a side-length of 280mm.
```
#define DATA_PIN        27
#define LED_TYPE        WS2812
#define COLOR_ORDER     GRB

#define NUM_SIDES       4 	// how many different areas you want your lamp to have: in my case 4, maximum 9.
#define NUM_ROWS        3	// how often you wrapped your strip around the frame.
#define LEDS_PER_ROW    20	// how many individually controllable spots there are per side in a single row (be careful: on cheaper LED-strips one controller controls multiple LEDs).
#define START_OFFSET    11	// in case your LED-strip starts somewhere in the middle of one side and not at the edge, this is the number of spots before the first edge.
#define BRIGHTNESS_STEP 0.1 // how much the brightness increases or decreases with one "+" or "-" message.
#define MAX_NB_PRESETS  10 	// this much space is reserved in the Arduinos storage.
```

## Set up Arduino-LEDLamp and Arduino-Keypad
The following steps may vary depending on what Arduiono you use.

#### Install FastLED as LED-control in Arduino IDE
- Clone ```https://github.com/FastLED/FastLED``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/FastLED```

#### Install PubSubClient as MQTT-client in Arduino IDE
- Clone ```https://github.com/knolleary/pubsubclient``` into ```ARDUINO_SKETCHBOOK_DIR/libraries/pubsubclient```

#### Install NodeMCU ESP32 Board in Arduino IDE
- Clone ```https://github.com/espressif/arduino-esp32``` into ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32``` 
- Run ```git submodule update --init --recursive``` in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32```
- Double-click get.exe (windows) or run ```python2 get.py``` (linux) in ```ARDUINO_SKETCHBOOK_DIR/hardware/espressif/esp32/tools```
- (Re-)start Arduino IDE and select ```NodeMCU-32S``` under ```Tools -> Boards```
- Finally, load the ```/LEDStripLamp/Arduino-LEDLamp/Arduino-LEDLamp.ino``` onto your ESP32-board, which will run your LED-strip.

#### Install NodeMCU ESP8266 Board in Arduino IDE
- In the Arduino IDE, go to ```Files -> Preferences``` and add ```http://arduino.esp8266.com/stable/package_esp8266com_index.json``` under ```Additional Board Manager URLs```
- Search and install ```esp8266``` under ```Tools -> Boards -> Boards Manager```
- Select ```Generic NodeMCU 1.0``` under ```Tools -> Boards```
- Finally, load the ```/LEDStripLamp/Arduino-Keypad/Arduino-Keypad.ino``` onto your ESP8266-board, which will take inputs from your keypad.

# Communication
This section defines the message-formats sent via MQTT, each message has its own topic for the sake of modularity.

## COLOR Message
The COLOR-msg sets the color of each lamp-side individually to the specified value.
- MQTT-Topic: ```MQTT_COLOR_TOPIC```
- Format: ```r g b r g b r g b r g b ...``` where each ```r g b```-triplet represents the RGB-color of one side of the lamp. Each r-/g-/b-value is in ```[00...FF]``` and is seperated to consecutive values by a whitespace.
- Example: ```255 255 255 255 0 0 0 147 147 0 0 0``` results in the lamp displaying white on side 1, red on side 2, yellow on side 3 and is off on side 4.

## SINGLECOLOR Message
The SINGLECOLOR-msg sets the color of all lamp-sides to the same specified value.
- MQTT-Topic: ```MQTT_COLOR_TOPIC```
- Format: ```r g b``` Each r-/g-/b-value is in ```[00...FF]``` and is seperated to consecutive values by a whitespace.
- Example: ```132 112 255``` results in the lamp displaying violet on all sides.

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
- Hint: The percentual change can be set in the ```connection_conf.h```. This message has no effect if one r-/g-/b-value is already 255 (```+```-msg), respective 0 (```-```-msg)

## SAVEPRESET Message
The SAVEPRESET-msg makes the lamp remember the current configuration (colors and mode) for later restoring. It can be published by the user and is received by the control-server, which afterwards saves the current configuration together with the given preset-ID and -name to the ```presets```-file
- MQTT-Topic: ```MQTT_SAVEPRESET_TOPIC```
- Format: ```ID name``` where ID is a number in ```[0...9]``` and name is an arbitrary string (optional).
- Example: ```5 sunset``` associates the current configuration with the number "5" and the name "sunset" and saves it to the disk.

## LOADPRESET Message
The LOADPRESET-msg makes the lamp restore the configuration (colors and mode) associated with the given ID or name. It can be published by the user/keypad and is received by the control-server, which afterwards publishes the saved configuration via a COLOR-msg.
- MQTT-Topic: ```MQTT_LOADPRESET_TOPIC```
- Format: ```ID``` or ```name``` where ID is a number in ```[0...9]``` and name is an arbitrary string, previously saved with a SAVEPRESET-msg.
- Example: ```5``` or ```sunset``` both restore the configuration that was saved in the SAVEPRESET-example.


# Troubleshooting

## Why are you using two different Arduinos?!
There were just two different Arduinos laying around when I started this project. It is theoretically possible to use ESP32/ESP8266 or other WiFi-supporting Arduinos for both the lamp-control and the keypad-control. Notice that the ESP8266 made my WS281x-strip flicker, no matter how hard I tried to fix it so you might be better off with the newer ESP32.

## Espressifs Wifi.h is colliding with Arduinos Wifi.h when compiling?!
It is important to name the ESP32_DIR ```esp32``` and not ```arduino-esp32``` as it will be named automatically. See [this thread](https://github.com/espressif/arduino-esp32/issues/20) for further information.

## My LED-Strip is flickering?!
See [this thread](https://github.com/FastLED/FastLED/issues/306) for problem with flickering LEDs.

## Why is there an additional inner lighted area in the images of your lamp which is never mentioned here?!
This is just a flat lamp from an off-the-shelf lamp which has nothing to do with the LED-strip and thus, with this software.