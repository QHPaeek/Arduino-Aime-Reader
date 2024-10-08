#include <Adafruit_NeoPixel.h>
extern uint8_t system_setting[3];

#define NUMPIXELS 11
#define LED_DIFF_COUNT 10//LED的最小变化数值
uint8_t LED_buffer[3];

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


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
//namco模式下对读卡器反应时间要求非常高，呼吸灯会消耗大量时间用于刷灯，需要进行一定程度的过滤来减缓时间压力，否则游戏会认为读卡器已经断开连接

void LED_show(uint8_t R, uint8_t G, uint8_t B) {
	if(!LED_filtering(R,G,B)){
		return;//颜色未变化，跳过发送
	}
	else if(system_setting[0] & 0b100){
 		for(int i=0; i<NUMPIXELS; i++) {
    			pixels.setPixelColor(i, R*system_setting[1]/255, G*system_setting[1]/255, B*system_setting[1]/255);
 		}
		pixels.show();
		LED_buffer[0] = R;
		LED_buffer[1] = G;
		LED_buffer[2] = B;
	}
}

void LED_Init(){
  pixels.begin();
  memset(LED_buffer,0xff,3);
  LED_show(0,0,0);
  memset(LED_buffer,0,3);
}
