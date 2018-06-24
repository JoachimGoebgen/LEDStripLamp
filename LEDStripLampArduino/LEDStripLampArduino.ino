#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "connection_conf.h" // contains WIFI_SSID, WIFI_KEY, MQTT_SRV_IP, MQTT_SRV_PORT, MQTT_TOPIC

FASTLED_USING_NAMESPACE

#define DATA_PIN        27
#define LED_TYPE        WS2812
#define COLOR_ORDER     GRB

#define LEDS_PER_ROW    20
#define NUM_SIDES       4
#define NUM_ROWS        3
#define START_OFFSET    11

#define LEDS_PER_ROUND  LEDS_PER_ROW * NUM_SIDES
#define TOTAL_LEDS      LEDS_PER_ROW * NUM_ROWS * NUM_SIDES

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int total_leds = LEDS_PER_ROW * NUM_ROWS * NUM_SIDES;
CRGB leds[TOTAL_LEDS];

uint8_t R[4] = {200, 0, 0, 200};
uint8_t G[4] = {0, 200, 0, 200};
uint8_t B[4] = {0, 0, 200, 200};
  
int rotationOffset = 0;
int fadingStepMax = 30;
int fadingStepCounter = 0;
int speedUpFactor = 1;

void setup() 
{
  delay(3000);
  Serial.begin(9600);

  connectWifi();
  
  mqttClient.setServer(MQTT_SRV_IP, (uint16_t)MQTT_SRV_PORT);
  mqttClient.setCallback(receivedMsg);
  connectMqtt();
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, TOTAL_LEDS).setCorrection(TypicalLEDStrip);
}

void connectWifi() 
{
  Serial.println("");
  Serial.print("Connecting to WiFi named ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_KEY);
  WiFi.setHostname(CLIENT_NAME);

  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected with IP address ");
  Serial.println(WiFi.localIP());
}

void connectMqtt() 
{
  while (!mqttClient.connected()) 
  {
    Serial.println("");
    Serial.print("Connecting to MQTT Server on ");
    Serial.print(MQTT_SRV_IP);
    
    if (mqttClient.connect(CLIENT_NAME)) 
    {
      mqttClient.subscribe(MQTT_TOPIC);
      
      Serial.println("");
      Serial.print("MQTT connected with client-name ");
      Serial.print(CLIENT_NAME);
      Serial.print(" and subscribed to topic ");
      Serial.println(MQTT_TOPIC);
    } 
    else 
    {
      Serial.println("");
      Serial.print("MQTT connection FAILED with status-code ");
      Serial.print(mqttClient.state());
      Serial.print(". Trying again in ");
      
      for (int i = 5; i > 0; i--)
      {
        Serial.print(i);
        Serial.print(" ");
        delay(1000);
      }
    }
  }
}

void receivedMsg(char* topic, byte* msg, unsigned int length)
{
  Serial.print("Message received: ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)msg[i]);
  }
  Serial.print("");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); }
  if (!mqttClient.connected()) { connectMqtt(); }

  mqttClient.loop();

  modeSolid();
  
  delay(20);
  FastLED.show();
}

void modeParty() 
{
  uint8_t blurAmount = dim8_raw( beatsin8(3, 64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
  blur1d(leds, TOTAL_LEDS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.
  
  uint8_t  i = beatsin8(  9, 0, TOTAL_LEDS);
  uint8_t  j = beatsin8( 7, 0, TOTAL_LEDS);
  uint8_t  k = beatsin8(  5, 0, TOTAL_LEDS);
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();  
  leds[(i+j)/2] = CHSV( ms / 29, 200, 255);
  leds[(j+k)/2] = CHSV( ms / 41, 200, 255);
  leds[(k+i)/2] = CHSV( ms / 73, 200, 255);
  leds[(k+i+j)/3] = CHSV( ms / 53, 200, 255);
  delay(10);
}

void modeSolid() 
{
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, 0, R[side], G[side], B[side]);
  }
  
  delay(2000 / speedUpFactor);
}

void modeSolidRotating() 
{
  // color sides
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, rotationOffset, R[side], G[side], B[side]);
  }

  // color transition-LEDs
  float fadePercentage = (float)fadingStepCounter / fadingStepMax;
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    int dR = (int)round((R[next(side)] - R[side]) * fadePercentage);
    int dG = (int)round((G[next(side)] - G[side]) * fadePercentage);
    int dB = (int)round((B[next(side)] - B[side]) * fadePercentage);
    colorColumn(START_OFFSET + rotationOffset + side * LEDS_PER_ROW, R[side] + dR, G[side] + dG, B[side] + dB);
  }

  // update fading
  fadingStepCounter++;
  if (fadingStepCounter >= fadingStepMax) 
  {
    fadingStepCounter = 0;
    updateRotation();
  }
 
  delay(50 / speedUpFactor);
}

void modeGradient()
{
  for (int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideGradient(side, 0, CRGB(R[side], G[side], B[side]), CRGB(R[next(side)], G[next(side)], B[next(side)]));
  }
  
  delay(2000 / speedUpFactor);
}

void colorSideGradient(int side, int offset, CRGB fromColor, CRGB toColor)
{
  int startPx = START_OFFSET + side * LEDS_PER_ROW + offset;
  fill_gradient_RGB(leds, startPx, fromColor, startPx + LEDS_PER_ROW, toColor);
  fillRemainingRows();
}

void colorSideSolid(int side, int offset, uint8_t r, uint8_t g, uint8_t b) 
{
  int startPx = offset + START_OFFSET + side * LEDS_PER_ROW;
  int endPx = startPx + LEDS_PER_ROW;
  
  for (int px = startPx; px < endPx; px++) 
  {
    leds[px % total_leds] = CRGB(r, g, b);
  }
  
  fillRemainingRows();
}

void fillRemainingRows() 
{
  int startPx = START_OFFSET;
  int endPx = startPx + LEDS_PER_ROUND;
  for (int row = 1; row < NUM_ROWS; row++) 
  {
    startPx = endPx;
    endPx += LEDS_PER_ROUND;
    for (int i = startPx; i < endPx; i++) 
    {
      leds[i % total_leds] = leds[i - LEDS_PER_ROUND];
    }
  }
}

void colorColumn(int px, uint8_t r, uint8_t g, uint8_t b) 
{
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    leds[px % total_leds] = CRGB(r, g, b);
    px += LEDS_PER_ROUND;
  }
}

void updateRotation() 
{
  if (rotationOffset < LEDS_PER_ROUND) {
    rotationOffset++;
  }
  else {
    rotationOffset = 0;
  }
}

int next(int side) 
{
  return (side - 1) % NUM_SIDES;
}

