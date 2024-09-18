class message
{
  public:
    int len = -1;
    int all_len = 0;
    uint8_t data[100];
    bool add_ack = 0;
    void initThis()
    {
      this->len = -1;
      this->all_len = 0;
      this->add_ack = 0;
      memset(data, 0, sizeof(data));
    }
    bool isReady()
    {
      if (len == 0 && all_len == 6)
        return 1;
      return len != -1 && (all_len == (len + 7));
    }
    uint8_t getCommandCode() {
      return this->data[6];
    }

};

message recv, _send;
uint8_t cmd_switch;
int pass_s0 = 0;
bool zerohead = 0;
uint8_t proc_status = 0;
extern uint8_t switch_flag;

void Namco_PN532_Setup(){
  cmd_switch = 0;
  SerialDevice.begin(38400);
  SerialNFC.begin(115200);
  LED_Init();
  const uint8_t startup532[] = {
    0x55 , 0x55 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0x03 , 0xFD , 0xD4 , 0x14 , 0x01 , 0x17 , 0x00,
    0x00 , 0x00 , 0xff , 0x05 , 0xfb , 0xd4 , 0x14 , 0x01 , 0x14 , 0x01 , 0x02 , 0x00
  };
  while (SerialNFC.available())SerialNFC.read();
  for (int i = 0 ; i < 24; i++) {
    SerialNFC.write(startup532[i]);
  }
  unsigned long cmdTime = millis();
  int toreadlen = 15;
  bool nfc_exist = 0;
  while (toreadlen && (millis() - cmdTime <= 2000))
  {
    if (SerialNFC.available()) {
      SerialNFC.read();
      toreadlen-- ;
    }
  }
  if (!toreadlen) nfc_exist = 1;
  
  for (int i = 24 ; i < 24 + 12; i++) {
    SerialNFC.write(startup532[i]);
  }

  cmdTime = millis();
  toreadlen = 15;
  while (toreadlen && (millis() - cmdTime <= 50))
  {
    if (SerialNFC.available()) {
      SerialNFC.read();
      toreadlen-- ;
    }
  }
  int tryCnt = 0;
  while (!nfc_exist)
  {
    tryCnt++;
    delay(40);
  }
  delay(250);
  LED_show(255,60,60); // Send the updated pixel colors to the hardware.
  delay(700);
  _send.initThis();
}

/*

   Dirty LED code:
   Namco LED has tons of mode that difficult to determine
   so only coded modes that games use frequently.

  0->off
  1->green to blue
  2->blue breath
  3->RGB breath
  4->red falling
  7->red to blue
  8->green blue green blue~
  10->RYloop
  11->blue keep
*/
uint8_t ledstat = 0;
uint32_t leddelay = 0;
uint32_t ledlasttim = 0; 
uint8_t ledlev = 0; 
uint8_t ledext = 0;

void ledsvc()
{
  if (!ledstat && !ledext)
  {
    LED_show(0,0,0); // Send the updated pixel colors to the hardware.
    ledext = 1;
    return;
  }
  if (ledstat == 11 && !ledext) {
    LED_show(0,0,system_setting[1]); // Send the updated pixel colors to the hardware.
    ledext = 1;
  }
  if (ledstat == 8)
  {
    // 绿蓝闪烁
    // delay bypass,tim means a col 's last time,lev bypass ,ext is color
    if (millis() - ledlasttim >= 150)
    {
      ledext = !ledext;
      ledlasttim = millis();
    }
    if (ledext)
    {
      LED_show(0,0,system_setting[1]); // Send the updated pixel colors to the hardware.
    }
    else
    {
      LED_show(0,system_setting[1],0); // Send the updated pixel colors to the hardware.
    }
  }

  if (ledstat == 10)
  {
    // RY闪烁
    // delay bypass,tim means a col 's last time,lev bypass ,ext is color
    if (millis() - ledlasttim >= 500)
    {
      ledext = !ledext;
      ledlasttim = millis();
    }
    if (ledext)
    {
      LED_show(system_setting[1],0,0); // Send the updated pixel colors to the hardware.
    }
    else
    {
      LED_show(system_setting[1],system_setting[1],0); // Send the updated pixel colors to the hardware.
    }
  }

  if (ledstat == 4)
  {
    if (ledlev <= 0)
    {
      LED_show(0,0,0); // Send the updated pixel colors to the hardware.
      ledstat = 0;
      return;
    }
    if (millis() - ledlasttim >= leddelay)
    {
      LED_show(ledlev,0,0); // Send the updated pixel colors to the hardware.
      ledlev -= 5;
      ledlasttim = millis();
    }
  }
  if (ledstat == 1)
  {
    if (ledlev <= 0)
    {
      return;
    }

    if (ledext == 0)
    {
      LED_show(0,system_setting[1],0);// Send the updated pixel colors to the hardware.
      ledext = 1;
    }
    if (millis() - ledlasttim >= leddelay)
    {
      ledext = 2;
      leddelay = 3;
    }

    if (ledext == 2 && millis() - ledlasttim >= leddelay)
    {
      LED_show(0, ledlev, 255 - ledlev);
      ledlev -= 25;
      ledlasttim = millis();
    }
  }

  if (ledstat == 7)
  {
    if (ledlev <= 0)
    {
      return;
    }

    if (ledext == 0)
    {
      LED_show(system_setting[1],0,0);
      ledext = 1;
    }
    if (millis() - ledlasttim >= leddelay)
    {
      ledext = 2;
      leddelay = 3;
    }

    if (ledext == 2 && millis() - ledlasttim >= leddelay)
    {
      LED_show(ledlev, 0, 255 - ledlev);
      ledlev -= 25;
      ledlasttim = millis();
    }
  }

  if (ledstat == 3)
  {

    if (ledext == 0)
    {
      // red
      if (millis() - ledlasttim >= 1000)
      {
        ledext = 1;
        ledlasttim = millis();
        return;
      }
      LED_show(255,0,0);
      //LED_show((uint8_t)(((float)1 - (float)fabs(((((float)millis() - (float)ledlasttim) / (float)1000) * (float)2) - (float)1)) * (float)255), 0, 0);
    }
    if (ledext == 1)
    {
      // green
      if (millis() - ledlasttim >= 1000)
      {
        ledext = 2;
        ledlasttim = millis();
        return;
      }
      //LED_show(0, (uint8_t)(((float)1 - (float)fabs(((((float)millis() - (float)ledlasttim) / (float)1000) * (float)2) - (float)1)) * (float)255), 0);
      LED_show(0,255,0);
    }
    if (ledext == 2)
    {
      // blue
      if (millis() - ledlasttim >= 1000)
      {
        ledext = 0;
        ledlasttim = millis();
        return;
      }
      LED_show(0,0,255);
      //LED_show(0, 0, (uint8_t)(((float)1 - (float)fabs(((((float)millis() - (float)ledlasttim) / (float)1000) * (float)2) - (float)1)) * (float)255));
    }
  }
  if (ledstat == 2)
  {
    if (millis() - ledlasttim >= 500)
    {
      ledlasttim = millis();
      ledext = !ledext;
    }
    if (!ledext)
    {
      // ___---^^^
      LED_show(0, 0, (float(millis() - ledlasttim) / (float)500) * 255);
      //LED_show(0,0,255);
    }
    else if (ledext)
    {
      // ^^^---___
      LED_show(0, 0, 255 - (float(millis() - ledlasttim) / (float)500) * 255);
      //LED_show(0,0,255);
    }
  }
}

void set_led_blue_breath() {
  ledstat = 2;
  ledlasttim = millis();
  ledext = 0;

}
void set_led_rgb_breath() {
  ledstat = 3;
  ledlasttim = millis();
  ledext = 0;

}
void set_led_red_yellow_loop() {
  ledstat = 10;
  ledlev = 255;
  leddelay = 600;
  ledlasttim = millis();
  ledext = 0;

}

void set_led_off() {
  ledstat = 0;
  ledext = 0;
}
void set_led_blue_keep() {
  LED_show(0,0,system_setting[1]);

}
void set_led_green_blue_loop() {
  ledstat = 8;
  ledlev = 255;
  leddelay = 600;
  ledlasttim = millis();
  ledext = 0;

}

void set_led_green_to_blue_keep() {
  ledstat = 1;
  ledlev = 255;
  leddelay = 600;
  ledlasttim = millis();
  ledext = 0;
}

void set_led_red_to_blue_keep() {
  ledstat = 7;
  ledlev = 255;
  leddelay = 600;
  ledlasttim = millis();
  ledext = 0;
}

void setLEDMode(uint8_t _mode) {
  //bngRwReqLED?
  if (_mode == 0x11)set_led_green_to_blue_keep();
  else if (_mode == 0x16)set_led_red_to_blue_keep();
  else if (_mode == 0x0c)set_led_green_blue_loop();
  else if (_mode == 0x00)set_led_off();
  else if (_mode == 0x05)set_led_blue_breath();
  else if (_mode == 0x0b) set_led_rgb_breath();
  else if (_mode == 0x08) set_led_red_yellow_loop();
  else if (_mode == 0x1b) set_led_blue_keep();

}

void proc()
{
  if (recv.len == 0)
  {
    zerohead = 1;
    recv.initThis();
    return;
  }
  bool to_pn532 = 1 ;
  //do non/dif-pn532 proc:
  //PeripheralDevice
  if (recv.getCommandCode() == 0x0E)
  { //Command Code = 0x0E(Write GPIO) -> BEEP and LED Commands
    if (recv.data[7] == 0x01) // LED Command
      setLEDMode(recv.data[8]);
    //all of them has a same response
    _send.add_ack = 1;
    _send.len = 2;
    _send.all_len = 9;
    uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x02 , 0xFE , 0xD5 , 0x0F , 0x1C , 0x00};
    memcpy(_send.data, tempBuf, sizeof(tempBuf));
    to_pn532 = 0;
  }
  //BngRw Commands
  if (recv.getCommandCode() == 0x18)
  { //Command Code = 0x18(Unknown Command 0x18)
    if (recv.data[7] = 0x01) // WakeUp Command
    {
      _send.add_ack = 1;
      _send.len = 2;
      _send.all_len = 9;
      uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x02 , 0xFE , 0xD5 , 0x19 , 0x12 , 0x00};
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    to_pn532 = 0;
  }
  if (recv.getCommandCode() == 0x32)
  { //Command Code == 0x32(RFConfig)
    if (recv.data[7] == 0x81) {
      //(Syntax err: 0x81 is a invild item)
      _send.add_ack = 1;
      _send.len = 2;
      _send.all_len = 9;
      uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x02 , 0xFE , 0xD5 , 0x33 , 0xF8 , 0x00};
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      to_pn532 = 0;
    }
  }
  if (recv.getCommandCode() == 0x06)
  { //Command Code == 0x06(ReadRegister)
    if (recv.len == 0x12) {
      //BngRwSpecialRegisterLen0x12
      _send.add_ack = 1;
      _send.len = 0x0A;
      _send.all_len = 17;
      uint8_t tempBuf[20] = {0x00, 0x00, 0xFF, 0x0A, 0xF6, 0xD5, 0x07, 0xFF, 0x3F, 0x0E, 0xF1, 0xFF, 0x3F, 0x0E, 0xF1, 0xAA, 0x00};
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      to_pn532 = 0;
    }
    if (recv.len == 0x18) {
      //BngRwSpecialRegisterLen0x18
      _send.add_ack = 1;
      _send.len = 0x0D;
      _send.all_len = 20;
      uint8_t tempBuf[20] = { 0x00, 0x00, 0xFF, 0x0D, 0xF3, 0xD5, 0x07, 0xDC, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0xF1, 0x26, 0x6A, 0x87, 0xC9, 0x00};
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      to_pn532 = 0;
    }

  }
  if (recv.getCommandCode() == 0x0C)
  { //Command Code == 0x0C(ReadGPIO)
    //BngRwSpecialGPIO
    _send.add_ack = 1;
    _send.len = 0x05;
    _send.all_len = 12;
    uint8_t tempBuf[15] = {0x00 , 0x00 , 0xFF , 0x05 , 0xFB , 0xD5 , 0x0D , 0x20 , 0x06 , 0x00 , 0xF8 , 0x00};
    memcpy(_send.data, tempBuf, sizeof(tempBuf));
    to_pn532 = 0;
  }
  if (recv.getCommandCode() == 0x12)
  { //Command Code == 0x12(SetParameters)
    //BngRwSpecialParam
    _send.add_ack = 1;
    _send.len = 0x02;
    _send.all_len = 9;
    uint8_t tempBuf[15] = {0x00 , 0x00 , 0xFF , 0x02 , 0xFE , 0xD5 , 0x13 , 0x18 , 0x00};
    memcpy(_send.data, tempBuf, sizeof(tempBuf));
    to_pn532 = 0;
  }
  if (recv.getCommandCode() == 0x08)
  { //Command Code == 0x08(Write Register)
    if (recv.data[7] == 0xFF) {
      //(Syntax err: Unknown SFR address)
      _send.add_ack = 1;
      _send.len = 3;
      _send.all_len = 10;
      uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x03 , 0xFD , 0xD5 , 0x09 , 0x00 , 0x22 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      to_pn532 = 0;
    }

  }
  if (recv.getCommandCode() == 0x08)
  { //Command Code == 0x08(Write Register)
    if (recv.data[7] == 0xFF) {
      //(Syntax err: Unknown SFR address)
      _send.add_ack = 1;
      _send.len = 3;
      _send.all_len = 10;
      uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x03 , 0xFD , 0xD5 , 0x09 , 0x00 , 0x22 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      to_pn532 = 0;
    }
    if (recv.data[7] == 0x63) {
      //clear cardreading service
      _send.add_ack = 1;
      _send.len = 3;
      _send.all_len = 10;
      uint8_t tempBuf[10] = {0x00 , 0x00 , 0xFF , 0x03 , 0xFD , 0xD5 , 0x09 , 0x00 , 0x22 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
      pass_s0 = 9 + 6;
    }
  }
  if (recv.getCommandCode() == 0x52)
  { //Command Code == 0x52(InRelease)
    for (int i = 0 ; i < recv.all_len; i++) {
      SerialNFC.write(recv.data[i]);
    }
    uint8_t temp_532_data[10 + 6];
    int to_recv_532_len = 10 + 6;
    int recved_532_len = 0;
    while (to_recv_532_len) {
      while (to_recv_532_len && SerialNFC.available()) {
        temp_532_data[recved_532_len++] = SerialNFC.read();
        to_recv_532_len --;
      }
    }
    _send.add_ack = 1;
    _send.len = 4;
    _send.all_len = 11;
    if (temp_532_data[13] == 0x00) {
      uint8_t tempBuf[11] = {0x00 , 0x00 , 0xFF , 0x04 , 0xFC , 0xD5 , 0x53 , 0x01 , 0x00 , 0xD7 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    else if (temp_532_data[13] == 0x27) {
      uint8_t tempBuf[11] = {0x00 , 0x00 , 0xFF , 0x04 , 0xFC , 0xD5 , 0x53 , 0x01 , 0x27 , 0xB0 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    to_pn532 = 0;
  }
  if (recv.getCommandCode() == 0x44)
  { //Command Code == 0x44(InDeselect)
    for (int i = 0 ; i < recv.all_len; i++) {
      SerialNFC.write(recv.data[i]);
    }
    uint8_t temp_532_data[10 + 6];
    int to_recv_532_len = 10 + 6;
    int recved_532_len = 0;
    while (to_recv_532_len) {
      while (to_recv_532_len && SerialNFC.available()) {
        temp_532_data[recved_532_len++] = SerialNFC.read();
        to_recv_532_len --;
      }
    }
    _send.add_ack = 1;
    _send.len = 4;
    _send.all_len = 11;
    if (temp_532_data[13] == 0x00) {
      uint8_t tempBuf[11] = {0x00 , 0x00 , 0xFF , 0x04 , 0xFC , 0xD5 , 0x45 , 0x01 , 0x00 , 0xE5 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    else  if (temp_532_data[13] == 0x27) {
      uint8_t tempBuf[11] = {0x00 , 0x00 , 0xFF , 0x04 , 0xFC , 0xD5 , 0x45 , 0x01 , 0x27 , 0xBF , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    to_pn532 = 0;
  }
  if (recv.getCommandCode() == 0xA0)
  {
    //Command Code == 0xA0 ReadFelicaCardbySonyRCS620s?
    //00 00 ff %Len=15% %lensum=EB% D4 [40] %Tg:01% [Raw Data:[Data len(cmdlen+1)][cmd]] %checksum% 00
    //send to 532
    uint8_t to_532_data[] = {0x00 , 0x00 , 0xFF , 0x15 , 0xEB , 0xD4 , 0x40 , 0x01,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0xCC , 0x00
                            };
    uint8_t sum = 0;
    for (uint8_t i = 0 ; i < 18; i++)to_532_data[8 + i] = recv.data[9 + i]; //copy felica len+cmd raw data
    for (uint8_t i = 0; i < 26; i++) sum += to_532_data[i];// calc checksum step 1
    to_532_data[26] = (uint16_t)0xFF - (uint16_t)sum; // calc checksum step 2
    for (uint8_t i = 0; i < 28; i++) {
      SerialNFC.write(to_532_data[i]); // send to 532}
      //      Serial.write(to_532_data[i]);debug
    }
    uint8_t temp_532_data[0x30 + 7 + 6];
    int to_recv_532_len = 0x30 + 7 + 6;
    bool card_lost = 0 ;
    unsigned long begin_time = millis();
    int recved_532_len = 0;
    while (to_recv_532_len) {
      ledsvc();
      if (millis() - begin_time >= 250) {
        card_lost = 1;
        break;
      }
      while (to_recv_532_len && SerialNFC.available()) {
        temp_532_data[recved_532_len++] = SerialNFC.read();
        if (recved_532_len == 10 && temp_532_data[9] == 0x03) {
          card_lost = 1;
          to_recv_532_len -= 0x30 - 0x03;
        }
        to_recv_532_len --;
      }
    }
    if (card_lost) {
      _send.add_ack = 1;
      _send.len = 3;
      _send.all_len = 10;
      uint8_t tempBuf[11] = {0x00 , 0x00 , 0xFF , 0x03 , 0xFD , 0xD5 , 0xA1 , 0x01 , 0x89 , 0x00 };
      memcpy(_send.data, tempBuf, sizeof(tempBuf));
    }
    else {
      temp_532_data[12] += 0x60;
      temp_532_data[0x030 + 7 + 6 - 2] -= 0x60;
      _send.add_ack = 0;
      _send.len = 0x30 + 6;
      _send.all_len = 0x30 + 7 + 6;

      memcpy(_send.data, temp_532_data, sizeof(temp_532_data));
    }
    to_pn532 = 0;
  }
  if (to_pn532) {
    for (int i = 0 ; i < recv.all_len; i++) {
      SerialNFC.write(recv.data[i]);
    }
  }
  recv.initThis();
}

void doRecv() {
  if (SerialDevice.available())
  {
    proc_status = 10;
    uint8_t recvByte = SerialDevice.read();
    if(recvByte == 0xaf){
      cmd_switch++;
    }else{
      cmd_switch=0;
    }
    //计数连续接到0xaf的数量，当做切换模式的指令
    if (recv.all_len == 0 && recvByte != 0x00)
    {
      recv.initThis();
    }
    else if (recv.all_len < 3)
    { //package head
      recv.data[recv.all_len++] = recvByte;
    }
    else if (recv.all_len == 3)
    { //package length
      recv.data[recv.all_len++] = recvByte;
      recv.len = recv.data[3];
      if (!(recv.data[0] == 0 && recv.data[1] == 0 && recv.data[2] == 0xFF))
      {
        recv.initThis();
      }
    }else{ 
      //normal data
      recv.data[recv.all_len++] = recvByte;
      if (recv.isReady())
      {
        proc();
      }
    }
  }else{
  proc_status --;
  }
}

void doSend() {
  if (_send.isReady()) {
    if (_send.add_ack) {
      uint8_t str[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
      for (int i = 0; i < 6; i++)
        SerialDevice.write((byte)str[i]);
    }
    for (int i = 0 ; i < _send.all_len; i++)
      SerialDevice.write(_send.data[i]);
  }
  _send.initThis();
  //proc_status = 0;
}
void Namco_Mode_Loop(){
  doRecv();
  if(cmd_switch == 30){
    system_mode = 0;
    switch_flag = 1;
    EEPROM.write(23,0);
    #if defined(ESP8266)
    EEPROM.commit();
    #endif
    return;
  }
  doSend();
  if (pass_s0) {
    while (SerialNFC.available() && pass_s0) {
      SerialNFC.read();
      pass_s0--;
    }
  }
  else {
    while (SerialNFC.available())SerialDevice.write(SerialNFC.read());
  }
  if((!proc_status) && (!SerialDevice.available()) && (!SerialNFC.available())){
    ledsvc();
  }
}