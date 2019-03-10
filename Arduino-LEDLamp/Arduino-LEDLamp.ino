#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "connection_conf.h" // contains WIFI_SSID, WIFI_KEY, MQTT_SRV_IP, MQTT_SRV_PORT and all MQTT-topics

FASTLED_USING_NAMESPACE

#define DATA_PIN        27
#define LED_TYPE        WS2812
#define COLOR_ORDER     GRB

#define LEDS_PER_ROW    20
#define NUM_SIDES       4 // <= 9
#define NUM_ROWS        3
#define START_OFFSET    11

#define LEDS_PER_ROUND  (LEDS_PER_ROW * NUM_SIDES)
#define TOTAL_LEDS      (LEDS_PER_ROW * NUM_ROWS * NUM_SIDES)

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

CRGB leds[TOTAL_LEDS];

// 
byte R[NUM_SIDES];
byte G[NUM_SIDES];
byte B[NUM_SIDES];
double SpeedUpFactor = 1;
int LedMode = 2;

// 
int rotationOffset = 0;
int fadingStepMax = 30;
int fadingStepCounter = 0;
bool doUpdate = true;

void setup() 
{
  delay(1000);

  #ifdef DEBUG 
    Serial.begin(9600); 
  #endif

  connectWifi();
  
  mqttClient.setServer(MQTT_SRV_IP, (uint16_t)MQTT_SRV_PORT);
  mqttClient.setCallback(receivedMsg);
  connectMqtt();
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, TOTAL_LEDS).setCorrection(TypicalLEDStrip);
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

  while (WiFi.status() != WL_CONNECTED) 
  {
      #ifdef DEBUG 
        Serial.print("."); 
      #endif
      
      delay(500);
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
      mqttClient.subscribe(MQTT_MODE_TOPIC);
      mqttClient.subscribe(MQTT_COLOR_TOPIC + "/+");
	  
      #ifdef DEBUG 
        Serial.println("");
        Serial.print("MQTT connected with client-name '");
        Serial.print(CLIENT_NAME);
        Serial.print("' and subscribed to topic '");
        Serial.print(MQTT_MODE_TOPIC);
        Serial.print("' and to all sub-topics of '");
        Serial.print(MQTT_COLOR_TOPIC);
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

// msg format: r1 g1 b1 r2 g2 b2 ... rn gn bn
// has to be this format, otherwise unexpected stuff will happen due to no checks existing
void receivedMsg(char* topic, byte* msg, unsigned int length)
{
  #ifdef DEBUG 
    Serial.print("Message received in topic ");
    Serial.print(topic);
    Serial.print(" : ");
  #endif
  
  byte digitPos = 0;
  byte wordCount = 0;
  byte value;
  // split msg by spaces and convert digits inbetween to numbers
  for (unsigned int i = 0; i <= length; i++)
  {
    #ifdef DEBUG 
      if (i < length) { Serial.print((char)msg[i]); }
    #endif
    
    if (i == length || msg[i] == 32) // space
    {
      if (digitPos == 0) { continue; } // two consecutive spaces
      
      value = toNumber(msg, i - digitPos, i - 1);
      
  	  if (strstr(topic, MQTT_COLOR_TOPIC))
  	  {
    		byte sideNr = topic[strlen(topic) - 1] - 48;
    		if (sideNr == 0)
    		{
    			receivedColor(wordCount, value); // all sides
    		}
    		else
    		{
    			receivedColor(wordCount + 3 * (sideNr - 1), value); // only one side
    		}
      }
      else if (strcmp(topic, MQTT_MODE_TOPIC) == 0)
      {
        receivedMode(wordCount, value);
      }
      
      digitPos = 0;
      wordCount++;
    }
    else 
    {
      digitPos++;
    }
  }
  
  #ifdef DEBUG 
    Serial.println("");
  #endif
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

void receivedMode(byte wordCount, byte value) 
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

// --------------------------------- MODES ---------------------------------

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
  
  delay(100);
}

void modeSolidRotating() 
{
  // color sides
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, rotationOffset, R[side], G[side], B[side]);
  }

  // color transition-LEDs
  float fadePercentage = 1 - (float)fadingStepCounter / fadingStepMax;
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    int dR = (int)round((R[next(side)] - R[side]) * fadePercentage);
    int dG = (int)round((G[next(side)] - G[side]) * fadePercentage);
    int dB = (int)round((B[next(side)] - B[side]) * fadePercentage);

    colorColumn(rotationOffset + LEDS_PER_ROW + side * LEDS_PER_ROW, R[side] + dR, G[side] + dG, B[side] + dB);
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
  
  delay(100);
}

void modeParty() 
{
  uint8_t blurAmount = dim8_raw( beatsin8(3, 64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
  blur1d(leds, TOTAL_LEDS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.
  
  int i = beatsin8(9, 0, TOTAL_LEDS - 1);
  int j = beatsin8(7, 0, TOTAL_LEDS - 1);
  int k = beatsin8(5, 0, TOTAL_LEDS - 1);
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();  
  Serial.println("1");
  leds[(i+j)/2] = CHSV(ms / 29, 200, 255);
  Serial.println("2");
  leds[(j+k)/2] = CHSV(ms / 41, 200, 255);
  Serial.println("3");
  leds[(k+i)/2] = CHSV(ms / 73, 200, 255);
  Serial.println("4");
  leds[(k+i+j)/3] = CHSV(ms / 53, 200, 255);
  Serial.println("5");
  delay(20 / SpeedUpFactor);
}

// --------------------------------- COLORING ---------------------------------

void colorSideGradient(int side, int offset, CRGB fromColor, CRGB toColor)
{
  int startPx = START_OFFSET + side * LEDS_PER_ROW + offset;
  fill_gradient_RGB(leds, startPx, fromColor, startPx + LEDS_PER_ROW, toColor);
  fillRemainingRows();
}

void colorSideSolid(int side, int offset, uint8_t r, uint8_t g, uint8_t b) 
{
  int startPx = offset + side * LEDS_PER_ROW;
  int endPx = startPx + LEDS_PER_ROW;
  
  for (int px = startPx; px < endPx; px++) 
  {
    leds[px % LEDS_PER_ROUND + START_OFFSET] = CRGB(r, g, b);
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
    leds[(px + START_OFFSET) % TOTAL_LEDS] = CRGB(r, g, b);
    px += LEDS_PER_ROUND;
  }
}

void updateRotation() 
{
  if (rotationOffset < LEDS_PER_ROUND) {
    rotationOffset++;
  }
  else {
    rotationOffset = 1;
  }
}

int next(int side) 
{
  return (side + 1) % NUM_SIDES;
}
