# LEDStripLamp
Controlling a multi-sided lamp built with a WS281x-strip and a NodeMCU ESP32 Arduino via MQTT with FastLED (including rudimentary web-interface).

## Using FastLED in Arduino IDE
	- Clone ```https://github.com/FastLED/FastLED``` into ```[ARDUINO_SKETCHBOOK_DIR]/libraries/FsatLED``` and (re-)start Arduino IDE

## Using NodeMCU ESP32 Board in Arduino IDE
For better reading the ```ESP32_DIR``` is meant as ```[ARDUINO_SKETCHBOOK_DIR]/hardware/espressif/esp32```
	- Clone ```https://github.com/espressif/arduino-esp32``` into ```ESP32_DIR``` 
	- Run ```git submodule update --init --recursive``` in ```ESP32_DIR```
	- Double-click get.exe (windows) or run ```python2 get.py``` (linux) in ```ESP32_DIR/tools```
	- (Re-)start Arduino IDE
	- Select ```NodeMCU-32S``` under Tools -> Boards


## connection_conf.h	
This header-file is used as config which is listed on the .gitignore to prevent you from accidently committing sensible information like your WIFI-password.
Just create it in the ```LEDStripLampArduino```-folder and add the following lines:
```
#define WIFI_SSID 		"your wifi name"
#define WIFI_KEY 		"your wifi key"
#define MQTT_SRV_IP 	"the IP your MQTT-server is running on"
#define MQTT_SRV_PORT	"the port your MQTT-server uses (1883 by default)"
#define MQTT_TOPIC		"some topic like /SmartHome/LEDStripLamp"
```


# Troubleshooting

## How do I install the NodeMCU ESP32 Board?!
If the short guide further up is not enough, see (espressif repo)[https://github.com/espressif/arduino-esp32] for more information on the installation process.

## Espressifs Wifi.h is colliding with Arduinos Wifi.h when compiling?!
It is important to name the ESP32_DIR ```esp32``` and not ```arduino-esp32``` as it will be named automatically. See (this thread)[https://github.com/espressif/arduino-esp32/issues/20] for further information.

## My LED-Strip is flickering?!
See (this thread)[https://github.com/FastLED/FastLED/issues/306] for problem with flickering LEDs.
