#include <WS2812B.h>
#include <SPI.h>
extern uint8_t system_setting[3];

WS2812B strip = WS2812B(NUM_LEDS);
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

void LED_show(uint8_t R, uint8_t G, uint8_t B) {
	if(!LED_filtering(R,G,B)){
		return;//颜色未变化，跳过发送
	}
	else if(system_setting[0] & 0b100){
		for(uint8_t i=0; i<NUM_LEDS; i++) {
      			strip.setPixelColor(i,(uint8_t)R*system_setting[1]/255, (uint8_t)G*system_setting[1]/255, (uint8_t)B*system_setting[1]/255);
  		}
		strip.show();
	}
}

void LED_Init(){
	strip.begin();// Sets up the SPI
  	strip.show();// Clears the strip, as by default the strip data is set to all LED's off.
	afio_remap(AFIO_REMAP_USART1); 
	SPI.setClockDivider(SPI_CLOCK_DIV32);
  	SPI.begin();
}