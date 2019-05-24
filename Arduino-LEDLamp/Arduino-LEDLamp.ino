#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include <WiFi.h>
#include "PubSubClient.h"
#include "connection_conf.h" // contains WIFI_SSID, WIFI_KEY, MQTT_SRV_IP, MQTT_SRV_PORT and all MQTT-topics

FASTLED_USING_NAMESPACE

#define DATA_PIN        27
#define LED_TYPE        WS2812
#define COLOR_ORDER     GRB

#define LEDS_PER_ROW    20
#define NUM_SIDES       4 // maximum 9
#define NUM_ROWS        3
#define START_OFFSET    11

#define LEDS_PER_ROUND  (LEDS_PER_ROW * NUM_SIDES)
#define TOTAL_LEDS      (LEDS_PER_ROW * NUM_ROWS * NUM_SIDES)

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

CRGB leds[TOTAL_LEDS];

int LedMode = 1;

// 
byte R[NUM_SIDES];
byte G[NUM_SIDES];
byte B[NUM_SIDES];
float Rf[NUM_SIDES];
float Gf[NUM_SIDES];
float Bf[NUM_SIDES];

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
      mqttClient.subscribe(MQTT_MODE_TOPIC);
      mqttClient.subscribe(MQTT_COLOR_TOPIC);
      mqttClient.subscribe(MQTT_BRIGHTNESS_TOPIC);
      
      char subColorTopic[strlen(MQTT_COLOR_TOPIC)+3] = {0};
      strcat(subColorTopic, MQTT_COLOR_TOPIC);
      strcat(subColorTopic, "/+");
      /*sprintf(subColorTopic, "%s%s", MQTT_COLOR_TOPIC, "/+");*/
      mqttClient.subscribe(subColorTopic);
      Serial.println(subColorTopic);
	  
      #ifdef DEBUG 
        Serial.println("");
        Serial.print("MQTT connected with client-name '");
        Serial.print(CLIENT_NAME);
        Serial.print("' and subscribed to topic '");
        Serial.print(MQTT_MODE_TOPIC);
        Serial.print("' and to topic '");
        Serial.print(MQTT_BRIGHTNESS_TOPIC);
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

// -------------------------- MSG HANDLING AND UPDATE --------------------------

// msg format: r1 g1 b1 r2 g2 b2 ... rn gn bn
// has to be this format, otherwise unexpected stuff will happen due to no checks existing
void receivedMsg(char* topic, byte* msg, unsigned int length)
{
	#ifdef DEBUG 
		Serial.print("Message received in topic ");
		Serial.print(topic);
    Serial.print(" : ");
    for (byte i = 0; i < length; i++)
    {
      if (msg[i] == 32) { Serial.print(" "); }
      else { Serial.print(msg[i] - 48); }
    }
    Serial.println("");
	#endif
	
	if (length == 0)
		return;
	    
	// lamp-mode: 0 ... 4
	if (strcmp(topic, MQTT_MODE_TOPIC) == 0)
	{
		updateMode(toNumberDec(msg, 0, 0));
	}
 
	// + or - to incerease or decrease brightness
	else if (strcmp(topic, MQTT_BRIGHTNESS_TOPIC) == 0)
	{
		updateBrightness(msg[0]);
	}
 
	// color-parent-topic: Contains color-values for each side
	else if (strcmp(topic, MQTT_COLOR_TOPIC) == 0)
	{
		byte wordNr = 0;
    byte digitNr = 0;

    for (unsigned int i = 0; i <= length; i++)
    {
      if (i == length || msg[i] == ' ') // space
      {
        if (digitNr == 0) { continue; } // two consecutive spaces or leading space

        if (msg[i - 7] == '#') // #HEX f.e. #ff204e #55f4e4 ...
        { 
          if (wordNr >= NUM_SIDES || digitNr != 7) { return; }
          updateColorRGB(0, wordNr + 1, toNumberHex(msg[i - 6], msg[i - 5]));
          updateColorRGB(1, wordNr + 1, toNumberHex(msg[i - 4], msg[i - 3]));
          updateColorRGB(2, wordNr + 1, toNumberHex(msg[i - 3], msg[i - 1]));
        }
        else // decimals f.e. 255 120 0 241 255 60 ... (rgb-triplets)
        {
          if (wordNr >= NUM_SIDES * 3 || digitNr > 3) { return; }
          updateColorRGB(wordNr % 3, wordNr / 3 + 1, toNumberDec(msg, i - digitNr, i - 1));
        }
        
        digitNr = 0;
        wordNr++;
      }
      else { digitNr++; }
    }
	}
 
	// color-sub-topic: To update a single side or all sides with the same color at once
	else if (strstr(topic, MQTT_COLOR_TOPIC)) 
	{
		byte sideNr = topic[strlen(topic) - 1] - 48;
    if (sideNr > NUM_SIDES)
      return;
      
    byte wordNr = 0;
    byte digitNr = 0;

    for (unsigned int i = 0; i <= length; i++)
    {
      if (i == length || msg[i] == ' ') // space
      {
        if (digitNr == 0) { continue; } // two consecutive spaces

        if (msg[i - 7] == '#') // #HEX f.e. #ff204e (only one hex-Nr)
        { 
          if (wordNr > 0 || digitNr != 7) { return; }
          updateColorRGB(0, sideNr, toNumberHex(msg[i - 6], msg[i - 5]));
          updateColorRGB(1, sideNr, toNumberHex(msg[i - 4], msg[i - 3]));
          updateColorRGB(2, sideNr, toNumberHex(msg[i - 3], msg[i - 1]));
        }
        else // decimals f.e. 255 120 0 (only one rgb-triplet)
        {
          if (wordNr >= 3) { return; }
          updateColorRGB(wordNr, sideNr, toNumberDec(msg, i - digitNr, i - 1));
        }
        
        digitNr = 0;
        wordNr++;
      }
      else { digitNr++; }
    }
	}
}

// rgbNr: 0=red, 1=green, 2=blue
// sideNr: 1...NUM_SIDES   (0 for all sides)
void updateColorRGB(byte rgbNr, byte sideNr, byte value) 
{
	byte from;
	byte to;
	if (sideNr == 0) 
	{
		from = 1;
		to = NUM_SIDES;
	}
	else 
	{
		from = sideNr;
		to = sideNr;
	}
	
	for (byte side = from; side <= to; side++)
	{
		switch(rgbNr)
		{
			case 0: 
        R[side - 1] = value; 
        Rf[side - 1] = value; 
			  break;
			case 1: 
			  G[side - 1] = value; 
        Gf[side - 1] = value; 
        break;
			case 2: 
        B[side - 1] = value; 
        Bf[side - 1] = value; 
			  break; 
			default: break;
		}
	}
}

void updateMode(byte value) 
{
	LedMode = value;
}

void updateBrightness(byte msg) 
{
	int sign; // cast '+' to 1 and '-' to -1
	if (msg == '+') {
		sign = 1;
	} else if (msg == '-'){
		sign = -1;
	} else {
		return;
	}
	
	for (byte sideNr = 0; sideNr < NUM_SIDES; sideNr++)
	{
		for (byte i = 0; i < NUM_SIDES; i++)
		{
      float newR = Rf[i] * ((float)1 + sign * BRIGHTNESS_STEP_PERC));
      float newG = Gf[i] * ((float)1 + sign * BRIGHTNESS_STEP_PERC));
      float newB = Bf[i] * ((float)1 + sign * BRIGHTNESS_STEP_PERC));
      
			// only update brightness if colors do not exceed the bounds
			if ((sign > 0 && newR <= 255 && newG <= 255 && newB <= 255)
				|| (sign < 0 && newR >= 0 && newG >= 0 && newB >= 0))
			{
				Rf[i] = newR;
				Gf[i] = newG;
				Bf[i] = newB;
        R[i] = (byte)round(newR);
        G[i] = (byte)round(newG);
        B[i] = (byte)round(newB);
			}
		}
	}
}

byte toNumberDec(byte* msg, int from_incl, int to_incl)
{
  int count = to_incl - from_incl + 1; // number of digits
  byte sum = 0;
  for (int i = 0; i < count; i++)
  {
    sum += (msg[from_incl + i] - 48) * pow(10, count - 1 - i); // convert "char" to its represented number and sum it up depending on its place
  }
  return sum;
}

byte toNumberHex(byte digitA, byte digitB) // only two digits
{
	byte sum = 0;

	if (digitA >= 48 && digitA <= 57) { // 0 ... 9
	  sum += (digitA - 48) * 16;
	} else if (digitA >= 65 && digitA <= 70) { // A ... F
	  sum += (digitA - 55) * 16;
	} else if (digitA >= 97 && digitA <= 102) { // a ... f
	  sum += (digitA - 87) * 16;
	}

	if (digitB >= 48 && digitB <= 57) { // 0 ... 9
	  sum += (digitB - 48);
	} else if (digitB >= 65 && digitB <= 70) { // A ... F
	  sum += (digitB - 55);
	} else if (digitB >= 97 && digitB <= 102) { // a ... f
	  sum += (digitB - 87);
	}

	return sum;
}

// --------------------------------- MODES ---------------------------------

void modeTurnOff() 
{
  Serial.println("TURNING OFF");
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, 0, 0, 0, 0);
  }
  
  delay(200);
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
 
  delay(25);
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
  leds[(i+j)/2] = CHSV(ms / 29, 200, 255);
  leds[(j+k)/2] = CHSV(ms / 41, 200, 255);
  leds[(k+i)/2] = CHSV(ms / 73, 200, 255);
  leds[(k+i+j)/3] = CHSV(ms / 53, 200, 255);
  delay(20);
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
