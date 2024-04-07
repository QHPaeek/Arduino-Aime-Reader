#include "Aime_Reader.h"
#include "ReadTestMode.h"

void setup() {
  #if defined(ESP8266)
  EEPROM.begin(32);
  #endif
  for(uint8_t i = 0;i<3;i++){
    system_setting[i] = EEPROM.read(i);
  }
  if((system_setting[2] == 0) || (system_setting[2] == 0xff)){
    for(uint8_t i = 0;i<3;i++)  {
    EEPROM.write(i, default_system_setting[i]);
    system_setting[i] = default_system_setting[i];
    }
  }
  if(system_setting[0] & 0b1000)
  {
    for(uint8_t i = 0;i<8;i++)
    {
      mapped_card_IDm[i] = EEPROM.read(i+4);
    }
    for(uint8_t i = 0;i<10;i++)
    {
      card_reflect.block2[i+7] = EEPROM.read(i+12);
    }
  }
  #if defined(ESP8266)
  EEPROM.commit();
  #elif defined(ARDUINO_ARCH_RP2040)
  Serial.ignoreFlowControl();
  Wire.setSDA(12);
  Wire.setSCL(13);
  #elif defined(STM32F1)
  Serial.dtr(false); 
  #elif defined(STM32F0)
  Serial.dtr(false); 
  #elif defined(ESP32)
  //EEPROM.commit();
  Wire.setPins(1,2);
  #endif
  SerialDevice.begin((system_setting[0] & 0b10)? 115200 : 38400);
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
  LED_refresh();
  if(Reader_Test_Mode == true)
  {
    ReadTest();
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
         LED_buffer[0] = 0;
         LED_buffer[1] = 0;
         LED_buffer[2] = system_setting[1];
         }
        else{
          LED_buffer[0] = 0;
          LED_buffer[1] = system_setting[1];
          LED_buffer[2] = 0;
        }
        if(!(system_setting[0] & 0b100)){
         LED_buffer[0] = 0;
         LED_buffer[1] = 0;
         LED_buffer[2] = 0;
        }
        delay(1);
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