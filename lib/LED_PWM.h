void LED_show(uint8_t r, uint8_t g,uint8_t b)
{
  	 //RED_PWM->setPWM(channel_RED, LED_PIN_RED, 50,(uint8_t) r*100/255); 
 	 //GREEN_PWM->setPWM(channel_GREEN, LED_PIN_GREEN, 50, (uint8_t) g*100/255); 
 	 //BLUE_PWM->setPWM(channel_BLUE,LED_PIN_BLUE, 50, (uint8_t) b*100/255);
	analogWrite(LED_PIN_RED,r);
	analogWrite(LED_PIN_GREEN,g);
	analogWrite(LED_PIN_BLUE,b);
	//digitalWrite(LED_PIN_RED,r);
	//analogWrite(LED_PIN_GREEN,g);
	//analogWrite(LED_PIN_BLUE,b);
	return;
}
void LED_Init()
{	
	pinMode(LED_PIN_RED, OUTPUT);
	pinMode(LED_PIN_GREEN, OUTPUT);
	pinMode(LED_PIN_BLUE, OUTPUT);
	LED_show(0, 0, 0);
	return;
}