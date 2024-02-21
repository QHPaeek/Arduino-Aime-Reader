#include "SPI.h"

void WS2812_send(uint8_t r, uint8_t g, uint8_t b) {
  unsigned char bits = 24;
  unsigned long value = 0x00000000;
  value = (((unsigned long)g << 16) | ((unsigned long)r << 8) | ((unsigned long)b));
  while (bits > 0) {
    if ((value & 0x800000) != LOW) {
      SPI.transfer(0xF8);//1
      asm("nop");
      asm("nop");
    } else {
      SPI.transfer(0xC0);//0
    }
    value <<= 1;
    bits--;
  }
}

//void WS2812_refresh() {
//  unsigned int n = 0;
//  for (n = 0; n < LED_N; n++) {
//    WS2812_send(LED_T[n][0], LED_T[n][1], LED_T[n][2]);
//  }
//  delayMicroseconds(60);
//}

void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
  for(uint8_t i = 0; i < NUM_LEDS ; i++)
  {
    WS2812_send(r, g, b);
  }
}

void LED_Init(){
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
  // 这里需要让SPI处于8MHz
  // 所以芯片要设置16M，SPI配置为2分频：
  // 16/2=8MHz
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  delay(10);
  //刷新一下所有灯的状态，函数见文档接下来的内容
  LED_show(0,0,0);
}
