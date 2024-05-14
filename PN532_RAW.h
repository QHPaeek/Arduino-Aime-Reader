void RAW_Loop(){
  uint8_t cmd_switch = 0;
  LED_show(0,128,128);
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
    SerialNFC.write(c);
  }
}