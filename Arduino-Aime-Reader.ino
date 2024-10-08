//#define SEGA_MODE 1
//#define SPICE_MODE 1
#define NAMCO_MODE 1
//#define Other_MODE 1

#include "Device.h"
#if defined (SEGA_MODE)
#include "Sega_Aime_Reader.h"

#endif
#if defined (SPICE_MODE)
#include "Spicetool_Reader.h"
#endif
#if defined (NAMCO_MODE)
#include "Namco_Banapass_Reader.h"
#endif
#if defined (Other_MODE)
#include "Test_Reader.h"
#include "PN532_RAW.h"
#endif

uint8_t switch_flag = 0;
uint8_t system_mode = 0;

void (*ReaderMain)();

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
  afio_remap(AFIO_REMAP_TIM2_FULL);
  afio_remap(AFIO_REMAP_USART1); 
  #endif
  EEPROM_get_sysconfig();
  switch(system_mode){
    #if defined (SEGA_MODE)
    case 0:
      Sega_Mode_Init();
      break;
    case 3:
      Sega_Mode_Init();
      break;
    case 4:
      Sega_Mode_Init();
      break;
    #endif
    #if defined (SPICE_MODE)
    case 1:
      Spice_Mode_Init();
      break;
    #endif
    #if defined (NAMCO_MODE)
    case 2:
      Namco_PN532_Setup();
      break;
    #endif
    
    default:
      #if SEGA_MODE
      Sega_Mode_Init();
    #elif SPICE_MODE
      Spice_Mode_Init();
    #elif NAMCO_MODE
      Namco_PN532_Setup();
    #endif
      break;
  }
}

void loop() {
  if(switch_flag){
    switch(system_mode){
      #if defined (SEGA_MODE)
      case 0:
        Sega_Mode_Init();
        switch_flag = 0;
        break;
      #endif
      #if defined (SPICE_MODE)
      case 1:
        Spice_Mode_Init();
        switch_flag = 0;
        break;
      #endif
      #if defined (NAMCO_MODE)
      case 2:
        Namco_PN532_Setup();
        switch_flag = 0;
        break;
      #endif
      default:
        break;
    }
  }
  switch(system_mode){
    #if defined (SEGA_MODE)
    case 0:
      Sega_Mode_Loop();
      break;
    #endif
    #if defined (SPICE_MODE)
    case 1:
      Spice_Mode_Loop();
      break;
    #endif
    #if defined (NAMCO_MODE)
    case 2:
      Namco_Mode_Loop();
      break;
    #endif
    #if defined (Other_MODE)
    case 3:
      Test_Reader_Loop();
      break;
    case 4:
      RAW_Loop();
      break;
    #endif
    default:
    #if SEGA_MODE
      Sega_Mode_Loop();
    #elif SPICE_MODE
      Spicetool_Mode_Loop();
    #elif NAMCO_MODE
      Namco_Mode_Loop();
    #endif
      break;
  }
}