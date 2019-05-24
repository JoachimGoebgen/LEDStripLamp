#include "WiFi.h"
#include "PubSubClient.h"
#include "connection_conf.h" // contains WIFI_SSID, WIFI_KEY, MQTT_SRV_IP, MQTT_SRV_PORT, MQTT_TOPIC
#include <Keypad.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const byte rows = 4; // nunmber of rows
const byte cols = 4; // number of columns
byte rowPins[rows] = {16, 17, 21, 22}; //connect to the row pinouts of the keypad
byte colPins[cols] = {5, 23, 19, 18}; //connect to the column pinouts of the keypad
char keys[cols][rows] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

void setup() {
  delay(1000);

  #ifdef DEBUG 
    Serial.begin(9600); 
  #endif

  connectWifi();
  
  mqttClient.setServer(MQTT_SRV_IP, (uint16_t)MQTT_SRV_PORT);
  connectMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); }
  if (!mqttClient.connected()) { connectMqtt(); }

  mqttClient.loop();

  char key = keypad.getKey();

  if (key != NO_KEY && keypad.getState() == PRESSED) {
    #ifdef DEBUG
      Serial.println(key);
    #endif
    if (key == '0') {                                   // --- 0:
      mqttClient.publish(MQTT_MODE_TOPIC, &key);    // [turn off] 0 is not used to publish preset but to turn off LED
    } else if (isDigit(key)) {                          // --- 1/2/3...:
      mqttClient.publish(MQTT_LOADPRESET_TOPIC, &key);    // [change preset] just publish digit (1, 2, ...) 
    } else if (isAlpha(key)) {                          // --- A/B/C/D:
      key -= 16;                                        // A/B/C/D to 1/2/3/4
      mqttClient.publish(MQTT_MODE_TOPIC, &key);    // [change mode] 1=solid, 2=rotating, 3=gradient, 4=party 
    } else {
      if (key == '*') {
        key = '-';
        mqttClient.publish(MQTT_BRIGHTNESS_TOPIC, &key); // [dec brightness] send '-'
      } else { // key == '#'
        key = '+';
        mqttClient.publish(MQTT_BRIGHTNESS_TOPIC, &key); // [inc brightness] send '+'
      }
    }
  }

  delay(50);
}

// --------------------------------- NETWORK ---------------------------------

void connectWifi() 
{
  #ifdef DEBUG
    Serial.println("");
    Serial.print("Connecting to WiFi named ");
    Serial.println(WIFI_SSID);
  #endif
  
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  WiFi.setHostname(CLIENT_NAME);
  
  byte c = 10;
  while (WiFi.status() != WL_CONNECTED) 
  {
      if (c == 0) // try reconnect after some time if it didn't work
      {
        #ifdef DEBUG
          Serial.println("");
          Serial.println("Reconnecting");
        #endif
        
        WiFi.disconnect(true);
        delay(1000);
        WiFi.mode(WIFI_STA);
        delay(1000);
        WiFi.begin(WIFI_SSID, WIFI_KEY);
        WiFi.setHostname(CLIENT_NAME);
        c = 10;
      }
      else // wait for connection ...
      {
        #ifdef DEBUG 
          Serial.print("."); 
        #endif
  
        delay(500);
        c--;
      }
  }

  #ifdef DEBUG 
    Serial.println("");
    Serial.print("WiFi connected with IP address ");
    Serial.println(WiFi.localIP());
  #endif
}

void connectMqtt() 
{
  while (!mqttClient.connected()) 
  {
    #ifdef DEBUG 
      Serial.println("");
      Serial.print("Connecting to MQTT Server on ");
      Serial.print(MQTT_SRV_IP);
    #endif
    
    if (mqttClient.connect(CLIENT_NAME)) 
    {
      #ifdef DEBUG 
        Serial.println("");
        Serial.print("MQTT connected with client-name '");
        Serial.print(CLIENT_NAME);
        Serial.print("' and will be publishing to topic '");
        Serial.print(MQTT_LOADPRESET_TOPIC);
        Serial.print("' and '");
        Serial.print(MQTT_MODE_TOPIC);
        Serial.println("'");
      #endif
    } 
    else 
    {
      #ifdef DEBUG 
        Serial.println("");
        Serial.print("MQTT connection FAILED with status-code ");
        Serial.print(mqttClient.state());
        Serial.print(". Trying again in ");
      #endif
      
      for (int i = 5; i > 0; i--)
      {
        #ifdef DEBUG 
          Serial.print(i);
          Serial.print(" ");
        #endif
        
        delay(1000);
      }
    }
  }
}
