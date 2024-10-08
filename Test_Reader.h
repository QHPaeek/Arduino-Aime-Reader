uint8_t IDm[8];
uint8_t PMm[8];
uint16_t SystemCode;

// const uint16_t blockList[4] = {0x8082,0x8086,0x8090,0x8091};
// const uint16_t serviceCodeList[1] = {0x000b};
uint8_t block[16];
extern uint8_t switch_flag;

void Test_Reader_Loop()
{
  LED_show(255,255,0);
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
      return;
    }
  }
   uint8_t uid[4], uL;
  if(Nesica_Read(&nesica_card_buffer)){
    SerialDevice.println("Nesica card!");
    SerialDevice.print("Card Serial:");
    SerialDevice.print((char*)nesica_card_buffer.card_serial);
    delay(1000);
    return;
  }else if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 1, AimeKey)) {
    SerialDevice.println("Aime card!");
    SerialDevice.print("UID Value:");
    nfc.PrintHex(uid, uL);
    SerialDevice.print("Block 2 Data:");
    if (nfc.mifareclassic_ReadDataBlock(2, block)) {
      nfc.PrintHex(block, 16);
    }
    delay(1000);
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, BanaKey)) {
    SerialDevice.println("Banapassport card!");
    SerialDevice.print("UID Value:");
    nfc.PrintHex(uid, uL);
    SerialDevice.print("Block 0 Data:");
    // if (nfc.mifareclassic_ReadDataBlock(0, card.block)) {
    //   nfc.PrintHex(card.block, 16);
    // }
    // SerialDevice.print("Block 1 Data:");
    // if (nfc.mifareclassic_ReadDataBlock(1, card.block)) {
    //   nfc.PrintHex(card.block, 16);
    // }
    // SerialDevice.print("Block 2 Data:");
    // if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
    //   nfc.PrintHex(card.block, 16);
    // }
    // SerialDevice.print("Block 3 Data:");
    // if (nfc.mifareclassic_ReadDataBlock(3, card.block)) {
    //   nfc.PrintHex(card.block, 16);
    // }
     delay(1000);
     return;
    }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, MifareKey)) {
    SerialDevice.println("Default Key Mifare!");
    SerialDevice.print("UID Value:");
    nfc.PrintHex(uid, uL);
    // SerialDevice.print("Block 2 Data:");
    // if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
    //   nfc.PrintHex(card.block, 16);
    // }
    delay(1000);
    return;
  }
  //   if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL)) {
  //     SerialDevice.println("Unknown key Mifare.");
  //     SerialDevice.print("UID Value:");
  //     nfc.PrintHex(uid, uL);
  //     delay(1000);
  //     return;
  //  }
   if (nfc.felica_Polling(0xFFFF, 0x01, IDm, PMm, &SystemCode, 200) == 1) {
      SerialDevice.println("FeliCa card!");
      SerialDevice.print("IDm:");
      nfc.PrintHex(IDm, 8);
      SerialDevice.print("PMm:");
      nfc.PrintHex(PMm, 8);
      // SerialDevice.print("SystemCode:");
      // card.SystemCode = card.SystemCode >> 8 | card.SystemCode << 8;
      // nfc.PrintHex(card.System_Code, 2);
      // uint8_t write_data[16];
      // for(uint8_t i=0;i<16;i++){
      //   write_data[i] = random(0,256);
      // }
      // Serial.println("RC Write to FeliCa Block 8080:");
      // nfc.PrintHex(write_data, 16);
      // uint16_t serviceCodeList1 = 0x0009;
      // uint16_t blocklist1 = 0x8080;
      // nfc.felica_WriteWithoutEncryption(1, &serviceCodeList1, 1, &blocklist1, &write_data);
      // uint16_t blockList_sp[4] = {0x8082,0x8086,0x8090,0x8091};
      // uint8_t blockdata_sp[4][16];
      // int8_t result = nfc.felica_ReadWithoutEncryption(1, serviceCodeList,4 , blockList_sp, blockData);
      // Serial.println("FeliCa Block:");
      // //int8_t result = nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 4, blockList, blockData);
      // for (uint8_t i = 0; i < sizeof(blockList)/2; i++) {
      //   if (result == 1) {
      //     Serial.println(blockList[i], HEX);
      //     nfc.PrintHex(blockData[i], 16);
      //   } else{
      //     Serial.println(blockList[i], HEX);
      //     Serial.print("error!");
      //     Serial.println(result, HEX);
      //   }
      // }
      delay(1000);
      return;
    }
    SerialDevice.println("Didn't find card");
    delay(1000);
}