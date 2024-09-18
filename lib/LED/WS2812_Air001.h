#include "SPI.h"
extern uint8_t system_setting[3];

#define LED_DIFF_COUNT 10//LED的最小变化数值
uint8_t LED_buffer[3];

bool LED_filtering(uint8_t R, uint8_t G, uint8_t B){
	if((LED_buffer[0] > R) &&((LED_buffer[0] - R) < LED_DIFF_COUNT)){
		return 0;
	}else if((LED_buffer[0] < R) &&((R - LED_buffer[0]) < LED_DIFF_COUNT)){
		return 0;
	}else if((LED_buffer[1] > G) &&((LED_buffer[1] - G) < LED_DIFF_COUNT)){
		return 0;
	}else if((LED_buffer[1] < G) &&((G - LED_buffer[1]) < LED_DIFF_COUNT)){
		return 0;
	}else if((LED_buffer[2] > B) &&((LED_buffer[2] - B) < LED_DIFF_COUNT)){
		return 0;
	}else if((LED_buffer[2] < B) &&((B - LED_buffer[2]) < LED_DIFF_COUNT)){
		return 0;
	}
	return 1;
}

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
	if(!LED_filtering(r,g,b)){
		return;//颜色未变化，跳过发送
	}
	else if(system_setting[0] & 0b100){
  		for(uint8_t i = 0; i < NUM_LEDS ; i++){
    			WS2812_send((uint8_t)r*system_setting[1]/255, (uint8_t)g*system_setting[1]/255, (uint8_t)b*system_setting[1]/255);
  		}
  	delayMicroseconds(60);
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
  //刷新一下所有灯的状态
  LED_show(0,0,0);
}
