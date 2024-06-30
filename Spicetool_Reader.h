#include "lib/Spicetool/connection.h"
spiceapi::Connection CON(512);
extern uint8_t system_mode;
extern uint8_t switch_flag;

char hex2str(uint8_t hex){
  //注意这里传输的必须是半字节
  if(hex < 0xA){
    return (hex + 48);//ascii 0 = 48
  }
  else{
    return (hex - 0xA + 65);//ascii A = 65
  }
}
void Spice_Mode_Init(){
  SerialDevice.begin(115200);
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
  LED_show(255,0,64);
  delay(1000);
}

void Spice_Mode_Loop(){
  LED_show(0,0,0);
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
    LED_show(0,255,0);
    char hex2str_buffer[2] = {0,0};
    char buffer[90] = "{\"id\":1,\"module\":\"card\",\"function\":\"insert\",\"params\":[0,\"E00401AF87654321\"]}";//应为E00401开头
    if(system_setting[0] & 0b100000){ //开启了传入真实卡号
      for(uint8_t i = 0;i<8;i++){
        buffer[57+2*i] = hex2str(block[8+i] >> 4);//高4位转换为字符
        buffer[58+2*i] = hex2str(block[8+i] & 0xF);//低4位转换为字符
      }
    }else{
      for(uint8_t i = 0;i<4;i++){
        buffer[65+2*i] = hex2str(mifare_uid[i] >> 4);
        buffer[66+2*i] = hex2str(mifare_uid[i] & 0xF);
      }
    }
    if(system_setting[0] & 0b1000000){//开启了2P刷卡
      buffer[54] = 49;
    }
    CON.request(buffer);
    delay(100);
  }else if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, mifare_uid, &id_len) && nfc.mifareclassic_AuthenticateBlock(mifare_uid, id_len, 1, 0, BanaKey)){
    LED_show(0,255,0);
    char hex2str_buffer[2] = {0,0};
    char buffer[90] = "{\"id\":1,\"module\":\"card\",\"function\":\"insert\",\"params\":[0,\"E00401AF87654321\"]}";//应为E00401开头
    for(uint8_t i = 0;i<4;i++){
      buffer[65+2*i] = hex2str(mifare_uid[i] >> 4);
      buffer[66+2*i] = hex2str(mifare_uid[i] & 0xF);
    }
    if(system_setting[0] & 0b1000000){//开启了2P刷卡
      buffer[54] = 49;
    }
    CON.request(buffer);
    delay(100);
  }
  uint8_t IDm[8] = {0};
  uint8_t PMm[8] = {0};
  if (nfc.felica_Polling(0xFFFF, 0x01, IDm, PMm, &SystemCode, 200) == 1) {
    LED_show(0,0,255);
    char buffer[90] = "{\"id\":1,\"module\":\"card\",\"function\":\"insert\",\"params\":[0,\"1234567887654321\"]}";
    char hex2str_buffer[2] = {0,0};
    for(uint8_t i = 0;i<8;i++){
      buffer[57+2*i] = hex2str(IDm[i] >> 4);//高4位转换为字符
      buffer[58+2*i] = hex2str(IDm[i] & 0xF);//低4位转换为字符
    }
    if(system_setting[0] & 0b1000000){//开启了2P刷卡
      buffer[54] = 49;
    }
    CON.request(buffer);
    delay(100);
  }
  // char light_cmd_buffer[58] = "{\"id\":3,\"module\":\"lights\",\"function\":\"read\",\"params\":[]}"
  // CON.request(light_cmd_buffer);
}