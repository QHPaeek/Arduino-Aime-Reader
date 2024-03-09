#include "Aime_Reader.h"

typedef union {
  uint8_t block[18];
  struct {
    uint8_t IDm[8];
    uint8_t PMm[8];
    union {
      uint16_t SystemCode;
      uint8_t System_Code[2];
    };
  };
} Card;
Card card;

uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
uint8_t BanaKey[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define M2F_B 1
uint16_t blockList[4] = {0x8080, 0x8081, 0x8082, 0x8083};
uint16_t serviceCodeList[1] = {0x000B};
uint8_t blockData[1][16];
bool Reader_Test_Mode = false;

void setup() {
  for(uint8_t i = 0;i<3;i++){
    system_setting[i] = EEPROM.read(i);
  }
  if(!(system_setting[2]) || (system_setting[2] == 0xff)){
    for(uint8_t i = 0;i<3;i++)  {
    EEPROM.write(i, default_system_setting[i]);
    system_setting[i] = default_system_setting[i];
    }
  }
  #if defined(ARDUINO_ARCH_RP2040)
  Serial.ignoreFlowControl();
  Wire.setSDA(12);
  Wire.setSCL(13);
  #elif defined(STM32F1)
  Serial.dtr(false); 
  #elif defined(STM32F0)
  Serial.dtr(false); 
  #elif defined(ESP32)
  Wire.setPins(1,2);
  #endif
  LED_Init();
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    delay(500);
    SerialDevice.println("Didn't find PN53x board");
    if((system_setting[0] & 0b100)){
      LED_show(system_setting[1],0x00,0x00);
    }
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));
  SerialDevice.begin((system_setting[0] & 0b10)? 115200 : 38400);
  if(!(system_setting[0] & 0b100)){
    LED_buffer[0] = 0;
    LED_buffer[1] = 0;
    LED_buffer[2] = 0;
  }
  else if (system_setting[0] & 0b10){
    //LED_show(0x00,0x00,(uint8_t)system_setting[1]);
    LED_buffer[0] = 0;
    LED_buffer[1] = 0;
    LED_buffer[2] = system_setting[1];

  }
  else{
    LED_buffer[0] = 0;
    LED_buffer[1] = system_setting[1];
    LED_buffer[2] = 0;
  }
}

void loop() {
  if (!(system_setting[0] & 0b100)){
    LED_show(0,0,0);
  }
  else{
    LED_show((uint8_t)LED_buffer[0],(uint8_t)LED_buffer[1],(uint8_t)LED_buffer[2]);
  }
  if(Reader_Test_Mode == true)
  {
    uint8_t uid[4], uL;
   if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 1, AimeKey)) {
      SerialDevice.println("Aime card!");
     SerialDevice.print("UID Value:");
      nfc.PrintHex(uid, uL);
     SerialDevice.print("Block 2 Data:");
      if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
        nfc.PrintHex(card.block, 16);
      }
     delay(2000);
      return;
   }
   if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, BanaKey)) {
     SerialDevice.println("Banapassport card!");
     SerialDevice.print("UID Value:");
     nfc.PrintHex(uid, uL);
     SerialDevice.print("Block 2 Data:");
     if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
       nfc.PrintHex(card.block, 16);
      }
     delay(2000);
     return;
    }
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, M2F_B, 0, MifareKey)) {
      SerialDevice.println("Default Key Mifare!");
      if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
       SerialDevice.print("Fake IDm:");
       nfc.PrintHex(card.IDm, 8);
       SerialDevice.print("Fake PMm:");
        nfc.PrintHex(card.PMm, 8);
      }
      delay(2000);
     return;
    }
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL)) {
      SerialDevice.println("Unknown key Mifare.");
      SerialDevice.print("UID Value:");
     nfc.PrintHex(uid, uL);
      delay(2000);
      return;
   }
   if (nfc.felica_Polling(0xFFFF, 0x01, card.IDm, card.PMm, &card.SystemCode, 200)) {
     SerialDevice.println("FeliCa card!");
     SerialDevice.print("IDm:");
      nfc.PrintHex(card.IDm, 8);
     SerialDevice.print("PMm:");
     nfc.PrintHex(card.PMm, 8);
     SerialDevice.print("SystemCode:");
     card.SystemCode = card.SystemCode >> 8 | card.SystemCode << 8;
     nfc.PrintHex(card.System_Code, 2);
     Serial.println("FeliCa Block:");
      for (uint8_t i = 0; i < 4; i++) {
        if (nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, &blockList[i], blockData) == 1) {
          Serial.println(blockList[i], HEX);
         nfc.PrintHex(blockData[0], 16);
        } else {
          Serial.println("error");
        }
      }
      delay(2000);
      return;
    }
    SerialDevice.println("Didn't find card");
    delay(500);
  }
  else{
    switch (packet_read()) {
      case 0:
        break;
      case CMD_TO_NORMAL_MODE:
        sys_to_normal_mode();
        break;
      case CMD_GET_FW_VERSION:
        sys_get_fw_version();
        break;
      case CMD_GET_HW_VERSION:
        sys_get_hw_version();
        break;

    // Card read
      case CMD_START_POLLING:
        nfc_start_polling();
        break;
      case CMD_STOP_POLLING:
        nfc_stop_polling();
        break;
      case CMD_CARD_DETECT:
        nfc_card_detect();
        break;

    // MIFARE
      case CMD_MIFARE_KEY_SET_A:
        memcpy(KeyA, req.key, 6);
        res_init();
        break;

      case CMD_MIFARE_KEY_SET_B:
        res_init();
        memcpy(KeyB, req.key, 6);
        break;

      case CMD_MIFARE_AUTHORIZE_A:
        nfc_mifare_authorize_a();
        break;

      case CMD_MIFARE_AUTHORIZE_B:
        nfc_mifare_authorize_b();
        break;

      case CMD_MIFARE_READ:
        nfc_mifare_read();
        break;

    // FeliCa
      case CMD_FELICA_THROUGH:
        nfc_felica_through();
        break;

    // LED
      case CMD_EXT_BOARD_LED_RGB:
        LED_buffer[0] = req.color_payload[0]?system_setting[1]:0;
        LED_buffer[1] = req.color_payload[1]?system_setting[1]:0;
        LED_buffer[2] = req.color_payload[2]?system_setting[1]:0;
        break;

      case CMD_EXT_BOARD_INFO:
        sys_get_led_info();
        break;

      case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
       break;

      case CMD_READ_EEPROM:
        res_init();
        res.payload_len = 4;
        res.frame_len = 10;
       for(uint8_t i = 0;i<3;i++){
          res.eeprom_data[i] = system_setting[i];
        }
        res.board_vision = BOARD_VISION;
        break;
    
     case CMD_WRITE_EEPROM:
        system_setting[0] = req.eeprom_data[0];
        system_setting[1] = req.eeprom_data[1];
        EEPROM.write(0, system_setting[0]);
        EEPROM.write(1, system_setting[1]);
        SerialDevice.begin((system_setting[0] & 0b10)? 115200 : 38400);
        if(!(system_setting[0] & 0b100)){
         LED_buffer[0] = 0;
         LED_buffer[1] = 0;
          LED_buffer[2] = 0;
        }
       else if (system_setting[0] & 0b10){
         LED_buffer[0] = 0;
         LED_buffer[1] = 0;
         LED_buffer[2] = system_setting[1];
         }
        else{
          LED_buffer[0] = 0;
          LED_buffer[1] = system_setting[1];
         LED_buffer[2] = 0;
        }
        res_init();
        break;
      case CMD_SW_READTEST_MODE:
        Reader_Test_Mode = true;
        break;  
      default:
        res_init();
        break;
    }
    packet_write();
  }
}