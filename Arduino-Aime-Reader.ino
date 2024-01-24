#define high_baudrate
#include "Aime_Reader.h"

void setup() {
//  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
//  FastLED.setBrightness(50);
//  FastLED.showColor(0);
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
//    FastLED.showColor(0xFF0000);
    delay(500);
 //   FastLED.showColor(0);
 //   delay(500);
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));

  SerialDevice.begin(baudrate);
//  FastLED.showColor(BootColor);
}

void loop() {
  //SERIALnum = SerialDevice.available();
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
//      FastLED.showColor(CRGB(req.color_payload[0], req.color_payload[1], req.color_payload[2]));
      break;

    case CMD_EXT_BOARD_INFO:
      sys_get_led_info();
      break;

    case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
      break;

    default:
      res_init();
  }
  packet_write();
}
