#include "card_read.h"
uint8_t KeyA[6], KeyB[6];
extern uint8_t switch_flag;
extern uint8_t system_mode;
extern uint8_t AimeKey[6];
extern uint8_t BanaKey[6];

#define RETRY 10

enum {
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING = 0x41,
  CMD_CARD_DETECT = 0x42,
  CMD_CARD_SELECT = 0x43,
  CMD_CARD_HALT = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ = 0x52,
  CMD_MIFARE_WRITE = 0x53,
  CMD_MIFARE_KEY_SET_B = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  CMD_TO_UPDATER_MODE = 0x60,
  CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  CMD_SEND_BINDATA_INIT = 0x63,
  CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  CMD_FELICA_THROUGH = 0x71,
  // LED board
  CMD_EXT_BOARD_LED = 0x80,
  CMD_EXT_BOARD_LED_RGB = 0x81,
  CMD_EXT_BOARD_LED_RGB_UNKNOWN = 0x82,  // 未知
  CMD_EXT_BOARD_INFO = 0xf0,
  CMD_EXT_FIRM_SUM = 0xf2,
  CMD_EXT_SEND_HEX_DATA = 0xf3,
  CMD_EXT_TO_BOOT_MODE = 0xf4,
  CMD_EXT_TO_NORMAL_MODE = 0xf5,
  //读卡器上位机功能
  CMD_READ_EEPROM = 0xf6,
  CMD_WRITE_EEPROM = 0xf7,
  CMD_SW_MODE = 0xf8,
  CMD_READ_MODE = 0xf9,
};

enum {
  FelicaPolling = 0x00,
  FelicaReqResponce = 0x04,
  FelicaReadWithoutEncryptData = 0x06,
  FelicaWriteWithoutEncryptData = 0x08,
  FelicaReqSysCode = 0x0C,
  FelicaActive2 = 0xA4,
};

enum {
  STATUS_OK = 0x00,
  STATUS_CARD_ERROR = 0x01,
  STATUS_NOT_ACCEPT = 0x02,
  STATUS_INVALID_COMMAND = 0x03,
  STATUS_INVALID_DATA = 0x04,
  STATUS_SUM_ERROR = 0x05,
  STATUS_INTERNAL_ERROR = 0x06,
  STATUS_INVALID_FIRM_DATA = 0x07,
  STATUS_FIRM_UPDATE_SUCCESS = 0x08,
  STATUS_COMP_DUMMY_2ND = 0x10,
  STATUS_COMP_DUMMY_3RD = 0x20,
};

extern struct mifare_card aime_card_buffer;
extern struct mifare_card bana_card_buffer;
extern struct mifare_card card_reflect;
extern struct nesica_card nesica_card_buffer;

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t mode;
      uint8_t key[6];            // CMD_MIFARE_KEY_SET
      uint8_t color_payload[3];  // CMD_EXT_BOARD_LED_RGB
      struct {
        uint8_t eeprom_data[2];     //系统内部设置
        uint8_t mapped_IDm[8];
        uint8_t target_accesscode[10];
      };
      struct {                   // CMD_CARD_SELECT,AUTHORIZE,READ
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {  // CMD_FELICA_THROUGH_POLL
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct {  // CMD_FELICA_THROUGH_READ,WRITE,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            uint8_t blockList[1][2];  // CMD_FELICA_THROUGH_READ
            uint8_t blockData[16];    // CMD_FELICA_THROUGH_WRITE
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_request_t;

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      uint8_t mode;
      uint8_t version[1];  // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
      uint8_t block[16];   // CMD_MIFARE_READ
      struct{
        uint8_t eeprom_data[3];
        uint8_t board_vision;
      };
      struct {             // CMD_CARD_DETECT
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t mifare_uid[4];
          struct {
            uint8_t IDm[8];
            uint8_t PMm[8];
          };
        };
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {  // FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];
            uint8_t numBlock;
            uint8_t blockData[1][1][16];
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

packet_request_t req;
packet_response_t res;

uint8_t len, r, checksum;
bool escape = false;

uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len) {
      return checksum == r ? req.cmd : STATUS_SUM_ERROR;
    }
    checksum += r;
  }
  return 0;
}

void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void res_init(uint8_t payload_len = 0) {
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = STATUS_OK;
  res.payload_len = payload_len;
}

void sys_to_normal_mode() {
  res_init();
  if (nfc.getFirmwareVersion()) {
    res.status = STATUS_INVALID_COMMAND;
  } else {
    res.status = STATUS_INTERNAL_ERROR;
    LED_show(255,0,0);
  }
}

void sys_get_fw_version() {
  if(system_setting[0] & 0b10){
    char fw_version[2] = "\x94";
    res_init(sizeof(fw_version) - 1);
    memcpy(res.version, fw_version, res.payload_len);
  }
  else{
    char fw_version[24] = "TN32MSEC003S F/W Ver1.2";
    res_init(sizeof(fw_version) - 1);
    memcpy(res.version, fw_version, res.payload_len);
  }
}

void sys_get_hw_version() {
  if(system_setting[0] & 0b10){
    char hw_version[10] = "837-15396";
    //char hw_version[10] = "837-15286";
    res_init(sizeof(hw_version) - 1);
    memcpy(res.version, hw_version, res.payload_len);
  }
  else{
    char hw_version[24] = "TN32MSEC003S H/W Ver3.0";
    res_init(sizeof(hw_version) - 1);
    memcpy(res.version, hw_version, res.payload_len);
  }

}

void sys_get_led_info() {
  if(system_setting[0] & 0b10){
    char led_info[13] = "000-00000\xFF\x11\x40";
    res_init(sizeof(led_info) - 1);
    memcpy(res.version, led_info, res.payload_len);
  }
  else{
    char led_info[10] = "15084\xFF\x10\x00\x12";
    res_init(sizeof(led_info) - 1);
    memcpy(res.version, led_info, res.payload_len);
  }
}


void nfc_start_polling() {
  res_init();
  nfc.setRFField(0x00, 0x01);
}

void nfc_stop_polling() {
  res_init();
  nfc.setRFField(0x00, 0x00);
}

void nfc_card_detect() {
  card_reflect.enable = false;
  aime_card_buffer.enable = false;
  bana_card_buffer.enable = false;
  nesica_card_buffer.enable = false;
  uint8_t bufferLength;
  uint16_t SystemCode;
  if(Nesica_Read(&nesica_card_buffer)){
    nesica_card_buffer.enable = true;
    memcpy(res.mifare_uid,nesica_card_buffer.uid,4);
    //取UID前4字节
    res.id_len = 4;
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
    return;
  }else if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len) && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid,res.id_len, 1, 0, BanaKey) && nfc.mifareclassic_ReadDataBlock(0, bana_card_buffer.block0) && nfc.mifareclassic_ReadDataBlock(1, bana_card_buffer.block1) && nfc.mifareclassic_ReadDataBlock(2, bana_card_buffer.block2) && nfc.mifareclassic_ReadDataBlock(3, bana_card_buffer.block3)){
    bana_card_buffer.enable = true;
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
    return;
  } else if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len) && nfc.mifareclassic_AuthenticateBlock(res.mifare_uid,res.id_len, 1, 1, AimeKey) && nfc.mifareclassic_ReadDataBlock(2, aime_card_buffer.block2)){
    aime_card_buffer.enable = true;
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
    return;
  }else if(nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {
    if(system_setting[0] & 0b1000)
    { 
      bool card_judge = true;
      for(uint8_t i = 0;i<8;i++)
      {
        if(res.IDm[i] != mapped_card_IDm[i])
        {
          card_judge = false;
          break;
        }
      }
      if(card_judge)
      {
        card_reflect.enable = true;
        res_init(0x07);
        res.count = 1;
        res.type = 0x10;
        return;
      }
      else{
        res_init(0x13);
        res.count = 1;
        res.type = 0x20;
        res.id_len = 0x10;
        return;
      }
    }
    else{
      res_init(0x13);
      res.count = 1;
      res.type = 0x20;
      res.id_len = 0x10;
    }
  } else {
    res_init(1);
    res.count = 0;
  }
}

void nfc_mifare_authorize_a() {
  res_init();
  if ((card_reflect.enable == false) && (aime_card_buffer.enable == false) && (bana_card_buffer.enable == false) && (nesica_card_buffer.enable == false)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_authorize_b() {
  res_init();
  if ((card_reflect.enable == false) && (aime_card_buffer.enable == false) && (bana_card_buffer.enable == false) && (nesica_card_buffer.enable == false)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_read() {
  res_init(0x10);
  if(card_reflect.enable){
    switch(req.block_no){
      case 0:
        memcpy(res.block,card_reflect.block0,16);
        break;
      case 1:
        memcpy(res.block,card_reflect.block1,16);
        break;
      case 2:
        memcpy(res.block,card_reflect.block2,16);
        break;
      case 3:
        memcpy(res.block,card_reflect.block3,16);
        break;
      default:
        res_init();
        res.status = STATUS_CARD_ERROR;
        break;
    }
    return;
  }else if (aime_card_buffer.enable){
    switch(req.block_no){
    case 0:
      memcpy(res.block,aime_card_buffer.block0,16);
      break;
    case 1:
      memcpy(res.block,aime_card_buffer.block1,16);
      break;
    case 2:
      memcpy(res.block,aime_card_buffer.block2,16);
      break;
    case 3:
      memcpy(res.block,aime_card_buffer.block3,16);
      break;
    default:
      res_init();
      res.status = STATUS_CARD_ERROR;
      break;
    }
    return;
  }else if (bana_card_buffer.enable){
    switch(req.block_no){
      case 0:
        memcpy(res.block,bana_card_buffer.block0,16);
        break;
      case 1:
        memcpy(res.block,bana_card_buffer.block1,16);
        break;
      case 2:
        memcpy(res.block,bana_card_buffer.block2,16);
        break;
      case 3:
        memcpy(res.block,bana_card_buffer.block3,16);
        break;
      default:
        res_init();
        res.status = STATUS_CARD_ERROR;
        break;
    }
    return;
  }else if(nesica_card_buffer.enable){  
    switch(req.block_no){
      case 0:
        memcpy(res.block,aime_card_buffer.block0,16);
        //直接把aime的拿过来用
        break;
      case 1:
        memset(res.block,0,16);
        break;
      case 2:
        memset(res.block,0,16);
        for(uint8_t i = 0;i<8;i++){
          res.block[i+8] = (nesica_card_buffer.card_serial[2*i] - 0x30)<<4 | (nesica_card_buffer.card_serial[2*i+1] - 0x30);
        }
        break;
      case 3:
        memcpy(res.block,aime_card_buffer.block3,16);
        break;
      default:
        res_init();
        res.status = STATUS_CARD_ERROR;
        break;
    }
    return;
  }else if(!nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)){
    res_init();
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_felica_through() {
  uint16_t SystemCode;
  if (nfc.felica_Polling(0xFFFF, 0x01, res.encap_IDm, res.poll_PMm, &SystemCode, 200) == 1) {
    SystemCode = SystemCode >> 8 | SystemCode << 8;
  } else {
    res_init();
    res.status = STATUS_CARD_ERROR;
    return;
  }
  uint8_t code = req.encap_code;
  res.encap_code = code + 1;
  switch (code) {
    case FelicaPolling:
      {
        res_init(0x14);
        res.poll_systemCode[0] = SystemCode;
        res.poll_systemCode[1] = SystemCode >> 8;
      }
      break;
    case FelicaReqSysCode:
      {
        res_init(0x0D);
        res.felica_payload[0] = 0x01;
        res.felica_payload[1] = SystemCode;
        res.felica_payload[2] = SystemCode >> 8;
      }
      break;
    case FelicaActive2:
      {
        res_init(0x0B);
        res.felica_payload[0] = 0x00;
      }
      break;
    case FelicaReadWithoutEncryptData:
      {
        uint16_t serviceCodeList[1] = {0x000B};
        for (uint8_t i = 0; i < req.numBlock; i++) {
          uint16_t blockList[1] = { (uint16_t)(req.blockList[i][0] << 8 | req.blockList[i][1]) };
          uint8_t check = 0;
          while(nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, res.blockData[i]) != 1) {
            check++;
            if(check == RETRY){
              memset(res.blockData[i], 0, 16);
              break;
            }
          }
        }
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
        res.numBlock = req.numBlock;
        res_init(0x0D + req.numBlock * 16);
      }
      break;
    case FelicaWriteWithoutEncryptData:
      {
        res_init(0x0C);  // WriteWithoutEncryption,ignore
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
      }
      break;
    default:
      res_init();
      res.status = STATUS_INVALID_COMMAND;
  }
  res.encap_len = res.payload_len;
}

void Sega_Mode_Loop(){
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
        LED_show(req.color_payload[0] , req.color_payload[1] , req.color_payload[2]);
        break;

      case CMD_EXT_BOARD_INFO:
        sys_get_led_info();
        break;

      case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
        break;

      case CMD_CARD_SELECT:

      case CMD_CARD_HALT:

      case CMD_EXT_TO_NORMAL_MODE:
        res_init();
        break;

      case STATUS_SUM_ERROR:
        res_init();
        res.status = STATUS_SUM_ERROR;
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
        for(uint8_t i = 0;i<8;i++)
        {
          mapped_card_IDm[i] = req.mapped_IDm[i];
        }
        for(uint8_t i = 0;i<10;i++)
        {
          card_reflect.block2[i+6] = req.target_accesscode[i];
        }
        EEPROM.write(0, system_setting[0]);
        EEPROM.write(1, system_setting[1]);
        for(uint8_t j = 0;j<8;j++)
        {
          EEPROM.write(j+4, req.mapped_IDm[j]);
        }
        for(uint8_t k = 0;k<10;k++)
        {
          EEPROM.write(k+12, req.target_accesscode[k]);
        }
        #if defined(ESP8266)
        EEPROM.commit();
        #endif
        SerialDevice.begin((system_setting[0] & 0b10)? 115200 : 38400);
        if (system_setting[0] & 0b10){
         LED_show(0,0,255);
         }
        else{
          LED_show(0,255,0);
        }
        delay(1);
        res_init();
        break;

      case CMD_SW_MODE:
        system_mode = req.mode;
        if(req.mode != 3){
          EEPROM.write(23,system_mode);
          #if defined(ESP8266)
          EEPROM.commit();
          #endif
        }
        switch_flag = 1;
        break;  
      
      case CMD_READ_MODE :
        res_init();
        res.mode = EEPROM.read(24);
        break;
        
      default:
        res_init();
        res.status = STATUS_INVALID_COMMAND;
        break;
  }
  packet_write();
}
void Sega_Mode_Init(){
  if ((system_setting[0] & 0b10)){
    SerialDevice.begin(115200);
  }
  else{
    SerialDevice.begin(38400);
  }
  if(system_setting[0] & 0b1000)
  {
    for(uint8_t i = 0;i<8;i++)
    {
      mapped_card_IDm[i] = EEPROM.read(i+4);
    }
    for(uint8_t i = 0;i<10;i++)
    {
      card_reflect.block2[i+6] = EEPROM.read(i+12);
    }
  }
  LED_Init();
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    delay(500);
    SerialDevice.println("Didn't find PN53x board");
    LED_show(255,0,0);

  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));
  if (system_setting[0] & 0b10){
    LED_show(0,0,255);
  }
  else{
    LED_show(0,255,0);
  }
}