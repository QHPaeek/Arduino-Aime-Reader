extern uint8_t system_setting[3];
void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
	if(system_setting[0] & 0b100){
		analogWrite(LED_PIN_RED,(uint8_t)r*system_setting[1]/255);
		analogWrite(LED_PIN_GREEN,(uint8_t)g*system_setting[1]/255);
		analogWrite(LED_PIN_BLUE,(uint8_t)b*system_setting[1]/255);
		return;
	}
}
void LED_Init()
{	
	#if defined(STM32F0)
	analogWriteFrequency(100);
	#endif
	pinMode(LED_PIN_RED, OUTPUT);
	pinMode(LED_PIN_GREEN, OUTPUT);
	pinMode(LED_PIN_BLUE, OUTPUT);
	//pinMode(LED_PIN_RED, PWM);
	//pinMode(LED_PIN_GREEN, PWM);
	//pinMode(LED_PIN_BLUE,PWM);
	LED_show(0, 0, 0);
	return;
}