#include "FastLED.h"

CRGB leds[NUM_LEDS];

void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
  FastLED.showColor(CRGB(r, g, b));
}
void LED_Init(){
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.showColor(0);
}