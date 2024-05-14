//#include "lib/Spicetool/wrappers.h"
//#include "Sega_Aime_Reader.h"
//extern spiceapi::Connection CON(512);
bool FWSW;
extern uint8_t system_mode;
extern uint8_t switch_flag;
extern packet_request_t req;
extern packet_response_t res;

void Spice_Mode_Init(){
  SerialDevice.begin(115200);
  //spiceapi::Connection CON(512);
  // if(system_setting[0] & 0b1000)
  // {
  //   for(uint8_t i = 0;i<8;i++)
  //   {
  //     mapped_card_IDm[i] = EEPROM.read(i+4);
  //   }
  //   for(uint8_t i = 0;i<10;i++)
  //   {
  //     card_reflect.block2[i+7] = EEPROM.read(i+12);
  //   }
  // }
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
    }
  }
  uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)
      && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid, res.id_len, 1, 1, AimeKey)
      && nfc.mifareclassic_ReadDataBlock(2, res.block)) {
    // sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
    //         res.block[0], res.block[1], res.block[2], res.block[3],
    //         res.block[4], res.block[5], res.block[6], res.block[7]);
    char buffer[90];
    sprintf(buffer, "{\"id\": 1,\"module\": \"card\",\"function\": \"insert\",\"params\": [0, \"%02X%02X%02X%02X%02X%02X%02X%02X\"]}",
      res.block[8], res.block[9], res.block[10], res.block[11],
      res.block[12], res.block[13], res.block[14], res.block[15]
    );
    SerialDevice.print(buffer);
  }
  if (nfc.felica_Polling(0xFFFF, 0x01, res.IDm, res.PMm, &SystemCode, 200)) {
    // sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
    //         res.IDm[0], res.IDm[1], res.IDm[2], res.IDm[3],
    //         res.IDm[4], res.IDm[5], res.IDm[6], res.IDm[7]);
    char buffer[90];
    sprintf(buffer, "{\"id\": 1,\"module\": \"card\",\"function\": \"insert\",\"params\": [0, \"%02X%02X%02X%02X%02X%02X%02X%02X\"]}",
      res.IDm[0], res.IDm[1], res.IDm[2], res.IDm[3],
      res.IDm[4], res.IDm[5], res.IDm[6], res.IDm[7]
    );
    SerialDevice.print(buffer);
  }
  // spiceapi::InfoAvs avs_info{};
  // if (spiceapi::info_avs(CON, avs_info)) {
  //   FWSW = 1;
  //   spiceapi::card_insert(CON, FWSW, card_id);

  //   LED_show(system_setting[1],0,0);
  // }
}