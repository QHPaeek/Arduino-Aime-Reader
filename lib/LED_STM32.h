// Automatically retrieve TIM instance and channel associated to pin
// This is used to be compatible with all STM32 series automatically.
TIM_TypeDef *LED_RED_TIM = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(LED_PIN_RED), PinMap_PWM);
uint32_t channel_RED = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(LED_PIN_RED), PinMap_PWM));

TIM_TypeDef *LED_GREEN_TIM = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(LED_PIN_GREEN), PinMap_PWM);
uint32_t channel_GREEN = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(LED_PIN_GREEN), PinMap_PWM));

TIM_TypeDef *LED_BLUE_TIM = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(LED_PIN_BLUE), PinMap_PWM);
uint32_t channel_BLUE = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(LED_PIN_BLUE), PinMap_PWM));

// Instantiate HardwareTimer object. Thanks to 'new' instantiation, HardwareTimer is not destructed when setup() function is finished.
HardwareTimer *RED_PWM = new HardwareTimer(LED_RED_TIM);
HardwareTimer *GREEN_PWM = new HardwareTimer(LED_GREEN_TIM);
HardwareTimer *BLUE_PWM = new HardwareTimer(LED_BLUE_TIM);

void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
  	 RED_PWM->setPWM(channel_RED, LED_PIN_RED, 50,(uint8_t) r*100/255); 
 	 GREEN_PWM->setPWM(channel_GREEN, LED_PIN_GREEN, 50, (uint8_t) g*100/255); 
 	 BLUE_PWM->setPWM(channel_BLUE,LED_PIN_BLUE, 50, (uint8_t) b*100/255);
	return;
}
void LED_Init()
{
	LED_show(0, 0, 0);
	return;
}