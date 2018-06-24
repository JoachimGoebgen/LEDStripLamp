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

#define LEDS_PER_ROUND  (LEDS_PER_ROW * NUM_SIDES)
#define TOTAL_LEDS      (LEDS_PER_ROW * NUM_ROWS * NUM_SIDES)

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

CRGB leds[TOTAL_LEDS];

byte R[NUM_SIDES];
byte G[NUM_SIDES];
byte B[NUM_SIDES];
double SpeedUpFactor = 1;
int LedMode = 0;
  
int rotationOffset = 0;
int fadingStepMax = 30;
int fadingStepCounter = 0;

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
      mqttClient.subscribe(MQTT_COLOR_TOPIC);
      mqttClient.subscribe(MQTT_SETTINGS_TOPIC);
      
      Serial.println("");
      Serial.print("MQTT connected with client-name '");
      Serial.print(CLIENT_NAME);
      Serial.print("' and subscribed to topic '");
      Serial.print(MQTT_COLOR_TOPIC);
      Serial.print("' and '");
      Serial.println(MQTT_SETTINGS_TOPIC);
      Serial.print("'");
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

// msg format: r1 g1 b1 r2 g2 b2 ... rn gn bn
// has to be this format, otherwise unexpected stuff will happen due to no checks existing
void receivedMsg(char* topic, byte* msg, unsigned int length)
{
  Serial.print("Message received in topic ");
  Serial.print(topic);
  Serial.print(" : ");

  byte digitPos = 0;
  byte wordCount = 0;
  byte value;
  // split msg by spaces and convert digits inbetween to numbers
  for (unsigned int i = 0; i <= length; i++)
  {
    if (i < length) { Serial.print((char)msg[i]); }
    
    if (i == length || msg[i] == 32) // space
    {
      if (digitPos == 0) { continue; } // two consecutive spaces
      
      value = toNumber(msg, i - digitPos, i - 1);

      if (strcmp(topic, MQTT_COLOR_TOPIC) == 0)
      {
        receivedColor(wordCount, value);
      }
      else if (strcmp(topic, MQTT_SETTINGS_TOPIC) == 0)
      {
        receivedSettings(wordCount, value);
      }
      
      digitPos = 0;
      wordCount++;
    }
    else 
    {
      digitPos++;
    }
  }
}

void receivedColor(byte wordCount, byte value) 
{
  switch(wordCount % 3)
  {
    case 0: R[wordCount / 3] = value; break;
    case 1: G[wordCount / 3] = value; break;
    case 2: B[wordCount / 3] = value; break; 
  }
}

void receivedSettings(byte wordCount, byte value) 
{
  switch(wordCount)
  {
    case 0: LedMode = value; break;
    case 1: SpeedUpFactor = value / (double)100; break;
  }
}

byte toNumber(byte* msg, int from_incl, int to_incl)
{
  int count = to_incl - from_incl + 1; // number of digits
  byte sum = 0;
  for (int i = 0; i < count; i++)
  {
    sum += (msg[from_incl + i] - 48) * pow(10, count - 1 - i); // convert "char" to its represented number and sum it up depending on its place
  }
  return sum;
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); }
  if (!mqttClient.connected()) { connectMqtt(); }

  mqttClient.loop();

  switch (LedMode) 
  {
    case 0: modeTurnOff(); break;
    case 1: modeSolid(); break;
    case 2: modeSolidRotating(); break;
    case 3: modeGradient(); break;
    case 4: modeParty(); break;
  }
  
  FastLED.show();
}

void modeTurnOff() 
{
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, 0, 0, 0, 0);
  }
}

void modeSolid() 
{
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, 0, R[side], G[side], B[side]);
  }
  
  delay(2000 / SpeedUpFactor);
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
 
  delay(50 / SpeedUpFactor);
}

void modeGradient()
{
  for (int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideGradient(side, 0, CRGB(R[side], G[side], B[side]), CRGB(R[next(side)], G[next(side)], B[next(side)]));
  }
  
  delay(2000 / SpeedUpFactor);
}

void modeParty() 
{
  uint8_t blurAmount = dim8_raw( beatsin8(3, 64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
  blur1d(leds, TOTAL_LEDS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.
  
  uint8_t  i = beatsin8(9, 0, TOTAL_LEDS);
  uint8_t  j = beatsin8(7, 0, TOTAL_LEDS);
  uint8_t  k = beatsin8(5, 0, TOTAL_LEDS);
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();  
  leds[(i+j)/2] = CHSV(ms / 29, 200, 255);
  leds[(j+k)/2] = CHSV(ms / 41, 200, 255);
  leds[(k+i)/2] = CHSV(ms / 73, 200, 255);
  leds[(k+i+j)/3] = CHSV(ms / 53, 200, 255);
  delay(20 / SpeedUpFactor);
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
    leds[px % TOTAL_LEDS] = CRGB(r, g, b);
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
      leds[i % TOTAL_LEDS] = leds[i - LEDS_PER_ROUND];
    }
  }
}

void colorColumn(int px, uint8_t r, uint8_t g, uint8_t b) 
{
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    leds[px % TOTAL_LEDS] = CRGB(r, g, b);
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

