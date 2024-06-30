#ifndef CARD_READ_H
#define CARD_READ_H

extern uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
extern uint8_t BanaKey[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
extern uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct mifare_card {
  bool enable;
  uint8_t block0[16]= {0xED,0x88,0xA1,0x5F,0x9B,0x88,0x04,0x00,0xC8,0x50,0x00,0x20,0x00,0x00,0x00,0x16};
  uint8_t block1[16]= {0};
  uint8_t block2[16]= {0};
  uint8_t block3[16]= {0x57,0x43,0x43,0x46,0x76,0x32,0x70,0xF8,0x78,0x11,0x57,0x43,0x43,0x46,0x76,0x32};
};
struct mifare_card aime_card_buffer;
struct mifare_card bana_card_buffer;
struct mifare_card card_reflect;

struct Tunion_card{
  bool enable = false;
  uint8_t uid[4];
  uint8_t uid_len;
};
struct Tunion_card Tunion_card_buffer;

struct nesica_card{
  bool enable = false;
  uint8_t uid[7];
  uint8_t uid_len;
  uint8_t card_serial[16];
};
struct nesica_card nesica_card_buffer;

uint8_t Nesica_Read(nesica_card* nesica_card_buffer){
  if(!(system_setting[0] & 0b10000)){
    //未开启读卡宽容
    //return 0;
  }
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nesica_card_buffer->uid, &nesica_card_buffer->uid_len)) {
    return 0;
  }
  if (nesica_card_buffer->uid_len != 7) {
    return 0;
  }
  for(uint8_t i = 0;i<4;i++){
    uint8_t buffer[4];
    if (!nfc.mifareultralight_ReadPage(i+5,buffer)) {
      //nesica的卡号在page5~page8，以ascii格式明文存储，每个page长度4字节
      return 0;
    }
    for(uint8_t j =0;j<4;j++){
        nesica_card_buffer->card_serial[i*4+j] = buffer[j];
    }
  }
  for(uint8_t i = 0;i<16;i++){
    if(!((nesica_card_buffer->card_serial[i] > 0x2f) && (nesica_card_buffer->card_serial[i] < 0x3a ))){
      //判断是否为ascii码，0的ascii为0x30
      //如果是空白mifare_ultralight卡，这几个扇区应该是空白的
      return 0;
    }
  }
  return 1;
}

// uint8_t Tunion_Read(Tunion_card* Tunion_card_buffer){

// }
#else

#endif