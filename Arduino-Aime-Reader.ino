#define high_baudrate
#include "Aime_Reader.h"

void setup() {
  SerialDevice.begin(baudrate);
  //Serial.dtr(false); 
  //delay(5000);
  //Serial.print("1");
  LED_Init();
  //Serial.print("2");
  nfc.begin();
  //Serial.print("3");
  while (!nfc.getFirmwareVersion()) {
//    FastLED.showColor(0xFF0000);
      LED_show(0xff,0x00,0x00);
      delay(500);
 //   FastLED.showColor(0);
      LED_show(0x00,0x00,0x00);
      delay(500);
  }
  //Serial.print("4");
  nfc.setPassiveActivationRetries(0x10);
  //Serial.print("5");
  nfc.SAMConfig();
 // Serial.print("6");
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));

//  FastLED.showColor(BootColor);
  LED_show(0x00,0xff,0x00);
}

void loop() {
  //SERIALnum = SerialDevice.available();
  switch (packet_read()) {
    case 0:
      //SerialDevice.write(0xE0);
      break;

    case CMD_TO_NORMAL_MODE:
    //SerialDevice.write(0xE0);
      sys_to_normal_mode();
      break;
    case CMD_GET_FW_VERSION:
    //SerialDevice.write(0xE0);
      sys_get_fw_version();
      break;
    case CMD_GET_HW_VERSION:
    //SerialDevice.write(0xE0);
      sys_get_hw_version();
      break;

    // Card read
    case CMD_START_POLLING:
    //SerialDevice.write(0xE0);
      nfc_start_polling();
      break;
    case CMD_STOP_POLLING:
    //SerialDevice.write(0xE0);
      nfc_stop_polling();
      break;
    case CMD_CARD_DETECT:
    //SerialDevice.write(0xE0);
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
        LED_show(req.color_payload[0], req.color_payload[1],req.color_payload[2]);
      break;

    case CMD_EXT_BOARD_INFO:
      sys_get_led_info();
      break;

    case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
      break;

    case CMD_BAUDRATE_TO_LOW:
      Serial.write(baudrate_change_status);
      Serial.begin(38400);

    case CMD_BAUDRATE_TO_HIGH:
      Serial.write(baudrate_change_status);
      Serial.begin(115200);

    default:
      res_init();
  }
  packet_write();
}
