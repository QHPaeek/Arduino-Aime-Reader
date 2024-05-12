void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
	analogWrite(LED_PIN_RED,r);
	analogWrite(LED_PIN_GREEN,g);
	analogWrite(LED_PIN_BLUE,b);
	return;
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