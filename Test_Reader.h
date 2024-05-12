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
const uint16_t blockList[4] = {0x8080, 0x8081, 0x8082, 0x8083};
const uint16_t serviceCodeList[1] = {0x000B};
uint8_t blockData[1][16];

void Test_Reader_Loop()
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
     delay(100);
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
     delay(100);
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
      delay(100);
     return;
    }
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL)) {
      SerialDevice.println("Unknown key Mifare.");
      SerialDevice.print("UID Value:");
     nfc.PrintHex(uid, uL);
      delay(100);
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
      delay(100);
      return;
    }
    SerialDevice.println("Didn't find card");
    delay(100);
}