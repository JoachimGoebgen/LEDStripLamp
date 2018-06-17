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

int rotationOffset = 0;
int fadingStepMax = 20;
int fadingStepCounter = 0;
int speedUpFactor = 0;

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

  
  FastLED.show();
}

void modeSolidLight(uint8_t R[4], uint8_t G[4], uint8_t B[4]) 
{
  for(int side = 0; side < NUM_SIDES; side++) 
  {
    colorSideSolid(side, 0, R[side], G[side], B[side]);
  }
  
  delay(2000 / speedUpFactor);
}

void modeSolidLightRotating(uint8_t R[4], uint8_t G[4], uint8_t B[4]) 
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

void modeColorGradient(uint8_t R[4], uint8_t G[4], uint8_t B[4])
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
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    fill_gradient_RGB(leds, startPx, fromColor, startPx + LEDS_PER_ROW, toColor);
    startPx += LEDS_PER_ROUND;
  }
}

void colorSideSolid(int side, int offset, uint8_t r, uint8_t g, uint8_t b) 
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

void colorColumn(int px, uint8_t r, uint8_t g, uint8_t b) 
{
  for (int row = 0; row < NUM_ROWS; row++) 
  {
    setColor(px, r, g, b);
    px += LEDS_PER_ROUND;
  }
}

void updateRotation() 
{
  rotationOffset = (rotationOffset + 1) % LEDS_PER_ROUND;
}

void setColor(int i, uint8_t r, uint8_t g, uint8_t b) 
{
  leds[i % total_leds] = CRGB(r, g, b);
}

int next(int side) 
{
  return (side - 1) % NUM_SIDES;
}

