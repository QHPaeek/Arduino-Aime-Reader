//#if defined(__AVR_ATmega32U4__)
//#pragma message "当前的开发板是 ATmega32U4"
//#define SerialDevice SerialUSB
//#define NUM_LEDS 11
//#define LED_PIN A3
//#define BOARD_VISION 1
//#include "lib/WS2812_FastLed.h"
//#define PN532_SPI_SS 10

//#elif defined(ARDUINO_SAMD_ZERO)
//#pragma message "当前的开发板是SAMD_ZERO"
//#define SerialDevice SerialUSB
//#define NUM_LEDS 11
//#define LED_PIN A3
//#define BOARD_VISION 2
//#include "lib/WS2812_FastLed.h"
//#define PN532_SPI_SS 10

#if defined(ESP8266)
#pragma message "当前的开发板是 ESP8266"
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
#define NUM_LEDS 11
//#define LED_PIN D5 //NodeMCU 1.0(ESP12E-Mod)
#define LED_PIN 14 //Generic ESP8266 Module
#define BOARD_VISION 3
#include "lib/LED/WS2812_FastLed.h"
#include "SoftwareSerial.h"
SoftwareSerial SoftSerialNFC(4, 5); // RX, TX

#elif defined(ESP32)
#pragma message "当前的开发板是 ESP32"
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
#define NUM_LEDS 11
#define LED_PIN 1
#define BOARD_VISION 4
#include "lib/LED/WS2812_FastLed.h"
SoftwareSerial SoftSerialNFC(1, 2); // RX, TX

#elif defined(AIR001xx)
#pragma message "当前的开发板是 AIR001"
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
//LED灯的个数
#define NUM_LEDS  11
//LED引脚为PA7，不支持更改，不需要定义
#define BOARD_VISION 5
#include "lib/LED/WS2812_Air001.h"
HardwareSerial SerialPN532(PF0, PF1);
#define SerialNFC SerialPN532

#elif defined(_BOARD_GENERIC_STM32F103C_H_)
#pragma message "当前的开发板是 STM32F103C6"
#define EEPROM_PAGE_SIZE        (uint16)0x400  /* Page size = 1KByte */
#define FLASH_SIZE 32
#define EEPROM_START_ADDRESS    ((uint32)(0x8000000 + FLASH_SIZE * 1024 - 2 * EEPROM_PAGE_SIZE))
#define EEPROM_PAGE0_BASE               ((uint32)(EEPROM_START_ADDRESS + 0x000))
#define EEPROM_PAGE1_BASE               ((uint32)(EEPROM_START_ADDRESS + EEPROM_PAGE_SIZE))
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
#define BOARD_VISION 6
#define LED_PIN_RED PB11
#define LED_PIN_GREEN PB10
#define LED_PIN_BLUE PB1
#include "lib/LED/LED_analogwrite.h"
#define SerialNFC Serial1

#elif defined(ARDUINO_GENERIC_F072C8TX)
#pragma message "当前的开发板是 STM32F072C8"
//Generic STM32F1 series
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
#define BOARD_VISION 7
#define LED_PIN_RED PB_11
#define LED_PIN_GREEN PB_10
#define LED_PIN_BLUE PB_1
#include "lib/LED/LED_analogwrite.h"
#define SerialNFC Serial1
HardwareSerial Serial1(PB_7, PB_6);

#elif defined(ARDUINO_ARCH_RP2040)
#pragma message "当前的开发板是 RP2040"
#define SerialDevice Serial
#define SPICEAPI_INTERFACE Serial
#define BOARD_VISION 8
//#define LED_PIN D5
#define SerialNFC Serial1

//#elif defined(__AVR_ATmega328P__)
//#pragma message "当前的开发板是 ATmega328P"
//#define SerialDevice Serial
//#define NUM_LEDS 11
//#define LED_PIN 13
//#define BOARD_VISION 9
//#include "lib/WS2812_FastLed.h"

#else
#error "未经测试的开发板，请检查串口和针脚定义"

#endif

#if defined(SerialNFC)
#pragma message "使用 UART 连接 PN532"
#include <PN532_HSU.h>
PN532_HSU pn532(SerialNFC);
#else
#pragma message "使用 SWUART 连接 PN532"
#include <PN532_SWHSU.h>
PN532_SWHSU pn532(SoftSerialNFC);
#define SerialNFC SoftSerialNFC
#endif

#include "PN532.h"
PN532 nfc(pn532);

#include <EEPROM.h>
//EEPROM
//1：系统设置，第0位保留，第1位是否开启高波特率，第二位是否开启LED，第三位是否启用AIC卡号映射，5、6、7三位代表使用的系统模式
//系统模式（取上述三位拼出一个整数）：0为SEGA模式，1为SpiceTool模式，2为Namco模式，3为Test模式，4为RAW直通模式
//2：LED亮度
//3：固件版本号
//4-11：被映射AIC卡号IDM
//12-22：目标卡号
uint8_t system_setting[3] = {0};
uint8_t mapped_card_IDm[8] = {0};
const uint8_t default_system_setting[3] = {0b00000110,128,7};
//LED灯颜色缓冲区，每次循环根据缓冲区颜色刷一次灯。
uint8_t LED_buffer[3] = {0};
uint8_t system_mode;

void EEPROM_get_sysconfig(){
  #if defined(ESP8266)
  EEPROM.begin(32);
  #elif defined(_BOARD_GENERIC_STM32F103C_H_)
  EEPROM.init(EEPROM_PAGE0_BASE,EEPROM_PAGE1_BASE,EEPROM_PAGE_SIZE);
  #endif
  for(uint8_t i = 0;i<3;i++){
    system_setting[i] = EEPROM.read(i);
  }
  if(system_setting[2] != default_system_setting[2]){
    for(uint8_t i = 0;i<3;i++)  {
    EEPROM.write(i, default_system_setting[i]);
    system_setting[i] = default_system_setting[i];
    }
  }
  system_mode = (system_setting[0] & (0b11100000) >> 5);
  #if defined(ESP8266)
  EEPROM.commit();
  #endif
}