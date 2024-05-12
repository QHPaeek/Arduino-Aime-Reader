#include "lib/Spicetool/wrappers.h"
spiceapi::Connection CON(512);
bool FWSW;

void Spicetool_Mode_Loop(){
  uint16_t SystemCode;
  char card_id[17];
  uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)
      && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid, res.id_len, 1, 0, AimeKey)
      && nfc.mifareclassic_ReadDataBlock(1, res.block)) {
    sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
            res.block[0], res.block[1], res.block[2], res.block[3],
            res.block[4], res.block[5], res.block[6], res.block[7]);

  } else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {
    sprintf(card_id, "%02X%02X%02X%02X%02X%02X%02X%02X",
            res.IDm[0], res.IDm[1], res.IDm[2], res.IDm[3],
            res.IDm[4], res.IDm[5], res.IDm[6], res.IDm[7]);
  } else {
    return;
  }
  spiceapi::InfoAvs avs_info{};
  if (spiceapi::info_avs(CON, avs_info)) {
    //FWSW = !digitalRead(SW4_FW);
    FWSW = 1;
    spiceapi::card_insert(CON, FWSW, card_id);

    LED_show(system_setting[1],0,0);
  }
}