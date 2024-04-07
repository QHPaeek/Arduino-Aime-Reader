#include "HardwareTimer.h"

#define LED_PIN_RED PB1
#define LED_PIN_GREEN PB11
#define LED_PIN_BLUE PB10

#if defined(STM32F0)
  	HardwareTimer *timerRED = new HardwareTimer(TIM14);
	uint8_t RED_channel = 1;
#elif defined(STM32F1)
  	HardwareTimer *timerRED = new HardwareTimer(TIM3);
	uint8_t RED_channel = 3;
const PinMap PinMap_TIM[] = {
  {PB_1_ALT2,  TIM3, STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, AFIO_TIM3_PARTIAL, 4, 0)}, // TIM3_CH4
  {PB_10,      TIM2, STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, AFIO_TIM2_PARTIAL_2, 3, 0)}, // TIM2_CH3
  {PB_11,      TIM2, STM_PIN_DATA_EXT(STM_MODE_AF_PP, GPIO_PULLUP, AFIO_TIM2_PARTIAL_2, 4, 0)}, // TIM2_CH4
  {NC,         NP,   0}
};
#endif
HardwareTimer *timer2 = new HardwareTimer(TIM2);

void LED_Init()
{	
  	timerRED->setOverflow(500, HERTZ_FORMAT);
  	timerRED->setCaptureCompare(RED_channel, 0, PERCENT_COMPARE_FORMAT);
 	timerRED->setMode(RED_channel, TIMER_OUTPUT_COMPARE_PWM1, LED_PIN_RED);
  	timerRED->resume();
  	timer2->setOverflow(500, HERTZ_FORMAT);
  	timer2->setCaptureCompare(3, 0, PERCENT_COMPARE_FORMAT);
  	timer2->setCaptureCompare(4, 0, PERCENT_COMPARE_FORMAT);
 	timer2->setMode(3, TIMER_OUTPUT_COMPARE_PWM1, LED_PIN_BLUE);
 	timer2->setMode(4, TIMER_OUTPUT_COMPARE_PWM1,LED_PIN_GREEN);
  	timer2->resume();
	return;
}
void LED_show(uint8_t r, uint8_t g,uint8_t b)
{	
	timerRED->setCaptureCompare(RED_channel, r, PERCENT_COMPARE_FORMAT);
  	timer2->setCaptureCompare(3, b, PERCENT_COMPARE_FORMAT);
  	timer2->setCaptureCompare(4, g, PERCENT_COMPARE_FORMAT);
  	timerRED->resume();
  	timer2->resume();
	return;
}