#include "lib/Spicetool/connection.h"
spiceapi::Connection CON(1024);
extern uint8_t system_mode;
extern uint8_t switch_flag;

void Spice_Mode_Init(){
  SerialDevice.begin(115200);
  //spiceapi::Connection CON(512);
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
  LED_buffer[0] = 0;
  LED_buffer[1] = 0;
  LED_buffer[2] = system_setting[1];

}

void Spicetool_Mode_Loop(){
  LED_show(255,0,64);
  uint16_t SystemCode;
  char card_id[17];
  uint8_t cmd_switch = 0;
  while(SerialDevice.available()){
    uint8_t c = SerialDevice.read();
    if(c == 0xaf){
      cmd_switch++;
    }else{
      cmd_switch=0;
    }
    if(cmd_switch == 30){
      system_mode = 0;
      switch_flag = 1;
      EEPROM.write(23,0);
      #if defined(ESP8266)
      EEPROM.commit();
      #endif
      return;
    }
  }
  uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
  uint8_t mifare_uid[4] = {0};
  uint8_t id_len = 0;
  uint8_t block[16];
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, mifare_uid, &id_len)
      && nfc.mifareclassic_AuthenticateBlock(mifare_uid, id_len, 1, 1, AimeKey)
      && nfc.mifareclassic_ReadDataBlock(2, block)) {
    // sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
    //         block[8], block[9], block[10], block[11],
    //         block[12], block[13], block[14], block[15]);
    // spiceapi::card_insert(CON, 0, card_id);
    char buffer[90];
    sprintf(buffer, "{\"id\":1,\"module\":\"card\",\"function\":\"insert\",\"params\":[0,\"%02X%02X%02X%02X%02X%02X%02X%02X\"]}",
      block[8], block[9], block[10], block[11],
      block[12], block[13], block[14], block[15]
    );
    CON.request(buffer);
    // SerialDevice.print(buffer);
  }
  uint8_t IDm[8] = {0};
  uint8_t PMm[8] = {0};
  if (nfc.felica_Polling(0xFFFF, 0x01, IDm, PMm, &SystemCode, 200)) {
    // sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
    //         IDm[0], IDm[1], IDm[2], IDm[3],
    //         IDm[4], IDm[5], IDm[6], IDm[7]);
    // spiceapi::card_insert(CON, 0, card_id);
    char buffer[90];
    sprintf(buffer, "{\"id\":1,\"module\":\"card\",\"function\":\"insert\",\"params\":[0,\"%02X%02X%02X%02X%02X%02X%02X%02X\"]}",
      IDm[0], IDm[1], IDm[2], IDm[3],
      IDm[4], IDm[5], IDm[6], IDm[7]
    );
    CON.request(buffer);
    // SerialDevice.print(buffer);
  }
  // spiceapi::InfoAvs avs_info{};
  // if (spiceapi::info_avs(CON, avs_info)) {
  //spiceapi::card_insert(CON, 0, "1111111111111111");
  //   LED_show(system_setting[1],0,0);
  // }
}