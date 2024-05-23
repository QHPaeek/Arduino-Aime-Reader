#include <Adafruit_NeoPixel.h>

#define NUMPIXELS 11

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void LED_show(uint8_t R, uint8_t G, uint8_t B) {
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(R,G,B));
  }
  pixels.show();
}

void LED_Init(){
  pixels.begin();
  LED_show(0,0,0);
}
