#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

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
#define FRAMES_PER_SECOND  120

int total_leds = LEDS_PER_ROW * NUM_ROWS * NUM_SIDES;
CRGB leds[TOTAL_LEDS];

void setup() 
{
  delay(3000); // 3 second delay for recovery
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, TOTAL_LEDS).setCorrection(TypicalLEDStrip);
}

void loop()
{
  uint8_t R[4] = {200, 0, 0, 200};
  uint8_t G[4] = {0, 200, 0, 200};
  uint8_t B[4] = {0, 0, 200, 200};
}

int rotationOffset = 0;
int fadingStepMax = 20;
int fadingStepCounter = 0;

void modePermanentLight(uint8_t R[4], uint8_t G[4], uint8_t B[4]) 
{
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSide(side, 0, R[side], G[side], B[side]);
  }
  
  FastLED.show();
  delay(2000);
}

void colorColumn(int px, uint8_t r, uint8_t g, uint8_t b) 
{
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    setColor(px, r, g, b);
    px += LEDS_PER_ROUND;
  }
}

void colorSide(int side, int offset, uint8_t r, uint8_t g, uint8_t b) 
{
  int startPx = offset + START_OFFSET + side * LEDS_PER_ROW;
  int endPx = startPx + LEDS_PER_ROW;
  
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    for (int j = startPx; j < endPx; j++) 
    {
      setColor(j, r, g, b);
    }
    
    startPx += LEDS_PER_ROUND;
    endPx += LEDS_PER_ROUND;
  }
}

void setColor(int i, uint8_t r, uint8_t g, uint8_t b) 
{
  leds[i % total_leds] = CRGB(r, g, b);
}

