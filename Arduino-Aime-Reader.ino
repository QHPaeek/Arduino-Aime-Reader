#include "Device.h"
#include "Sega_Aime_Reader.h"
#include "Spicetool_Reader.h"
#include "Namco_Banapass_Reader.h"
#include "Test_Reader.h"

void setup() {

  #if defined(ARDUINO_ARCH_RP2040)
  Serial.ignoreFlowControl();
  Serial1.setRX(12);
  Serial1.setTX(13);
  Serial1.begin(115200);
  #elif defined(STM32F0)
  Serial.dtr(false); 
  #elif defined(ESP32)
  //EEPROM.commit();
  #elif defined(_BOARD_GENERIC_STM32F103C_H_)
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
  afio_remap(AFIO_REMAP_TIM2_FULL);
  afio_remap(AFIO_REMAP_USART1); 
  #endif
  EEPROM_get_sysconfig();
  system_mode = 0;
  switch(system_mode){
    case 0:
      Sega_Mode_Init();
  //  case 2:
  //    Namco_PN532_Setup();
  //  default:
  //    break;
  }
  //LED_show(0,0,255);
}

void loop() {
  //system_mode = 2;
  switch(system_mode){
    case 0:
      Sega_Mode_Loop();
      break;
  //  case 1:
  //    Spicetool_Mode_Loop();
   case 2:
     Namco_Mode_Loop();
    case 3:
      Test_Reader_Loop();
      break;
  //  case 4:
  //    break;
    default:
      break;
  }
    
}