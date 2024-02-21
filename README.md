# Arduino-Aime-Reader

使用 Arduino + PN532 制作的 Aime 兼容读卡器。

本仓库为Fork仓库，支持更多的MCU以及开发板。 

- 支持卡片类型： [FeliCa](https://zh.wikipedia.org/wiki/FeliCa)（Amusement IC、Suica、八达通等）和 [MIFARE](https://zh.wikipedia.org/wiki/MIFARE)（Aime，Banapassport）
- 逻辑实现是通过对官方读卡器串口数据进行分析猜测出来的，并非逆向，不保证正确实现
- 通信数据格式参考了 [Segatools](https://github.com/djhackersdev/segatools) 和官方读卡器抓包数据，可在 [Example.txt](doc/Example.txt) 和 [nfc.txt](https://github.com/djhackersdev/segatools/blob/master/doc/nfc.txt) 查看
- PCB与外壳均开源，请查阅`/PCB`目录
- 可以通过baudrate_tool轻易的修改内置波特率和LED参数而无需重新刷写固件

### 使用方法：

1. 按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库
2. 按照`/PCB` 目录内对应硬件的使用方式，短接读卡器的mode-sw跳线进入下载模式，同时将读卡器与电脑中间接好线，并调整 PN532 上的拨码开关
3. 上传 [ReaderTest](tools/ReaderTest/ReaderTest.ino) 测试硬件是否工作正常
4. 若读卡正常，重新上传[Arduino-Aime-Reader](Arduino-Aime-Reader.ino)，按照支持列表打开设备管理器设置 COM 端口号并设置好实际需要使用的波特率
5. 按照游戏的波特率更改好游戏配置文件的`high_baudrate`选项，`115200`是`true`，`38400`是`false`
6. 读卡器默认情况下为`115200`波特率，如果你的游戏仅支持`38400`波特率（SDHD 60FPS)，需要运行[baudrate_tool](tools/BaudRateTool/baudrate_tool.exe)以更改读卡器上的设置
7. 如果有使用 [Segatools](https://github.com/djhackersdev/segatools)，参考 [segatools.ini 设置教程](https://github.com/djhackersdev/segatools/blob/master/doc/config/common.md#enable) 关闭 Aime 模拟读卡器
8. 上传程序打开游戏测试

如果需要自定义 Aime 卡，安装 [MifareClassicTool](https://github.com/ikarus23/MifareClassicTool)，修改 [Aime 卡示例](doc/aime示例.mct) 后写入空白 MIFARE UID/CUID 卡，即可刷卡使用。    


波特率的概念：正确运行读卡器需要游戏，计算机本地端口，以及读卡器三个地方的设置全部一致才可以。游戏内的波特率一般是不可更改的，计算机本地端口的波特率可以在设置管理器里面修改，读卡器的波特率请通过[baudrate_tool](tools/BaudRateTool/baudrate_tool.exe)进行修改。只有三个设置全部一致才可以使用。



使用CDC虚拟串口连接计算机的读卡器（STM32），或具有自适应波特率功能的读卡器不需要修改读卡器波特率，因为CDC具有无视波特率的特性。

### 支持游戏列表：

| 代号        | 默认 COM 号 | 支持的卡          | 默认波特率               |
| --------- | -------- | ------------- | ------------------- |
| SDDT/SDEZ | COM1     | FeliCa,MIFARE | 115200              |
| SDEY      | COM2     | MIFARE        | 38400               |
| SDHD      | COM4     | FeliCa,MIFARE | cvt=38400,sp=115200 |
| SBZV/SDDF | COM10    | FeliCa,MIFARE | 38400               |
| SDBT      | COM12    | FeliCa,MIFARE | 38400               |

- 如果读卡器没有正常工作，可以尝试运行[baudrate_tool](tools/BaudRateTool/baudrate_tool.exe)以更改读卡器上的波特率
- 参考 config_common.json 内 aime > unit > port 确认端口号
- 如果 `"high_baudrate" : true` 则波特率是`115200`，否则就是`38400`
- 在 `"high_baudrate" : true` 的情况下，本读卡器程序支持 emoney 功能，端末认证和刷卡支付均正常（需要游戏和服务器支持）

### 开发板适配情况：

| 开发板名                         | 主控                       | 备注                  |
| ---------------------------- | ------------------------ | ------------------- |
| SparkFun Pro Micro           | ATmega32U4               | 需要发送 DTR/RTS，未完全测试  |
| SparkFun SAMD21 Dev Breakout | ATSAMD21G18              | 未完全测试               |
| NodeMCU 1.0                  | ESP-12E + CP2102 & CH340 | CH340通讯，可能需要修改内部波特率 |
| NodeMCU-32S                  | ESP32-S + CH340          | 主要适配环境              |
| Arduino Uno                  | ATmega328P + CH340       | 未实际测试，据反馈不可用        |
| Air001 Dev Chip              | Air001 + CH340           | CH340通讯，可能需要修改内部波特率 |
| STM32  BulePill              | STM32F103C6T6/C8T6/CBT6  | 使用CDC链接,不需要修改波特率    |
| STM32  F072                  | STM32F072C8T6            | 使用CDC链接,不需要修改波特率    |
| Raspberry Pi Pico            | Raspberry RP2040         | 使用CDC链接,不需要修改波特率    |

### 已知问题：

- 在 NDA_08 命令的写入 Felica 操作没有实现，因为未确认是否会影响卡片后续使用
- 未确定`res.status`错误码的定义，因此`res.status`的值可能是错误的
- 因为 PN532 库不支持同时读取多张卡片，所以未实现`mifare_select_tag`，只会读到最先识别的卡片

### 引用库：

- 驱动 WS2812B：[FastLED](https://github.com/FastLED/FastLED)
- 驱动 PN532：[PN532](https://github.com/elechouse/PN532) 或 [Aime_Reader_PN532](https://github.com/Sucareto/Aime_Reader_PN532)
- 读取 FeliCa 参考：[PN532を使ってArduinoでFeliCa学生証を読む方法](https://qiita.com/gpioblink/items/91597a5275862f7ffb3c)
- 读取 FeliCa 数据的程序：[NFC TagInfo](https://play.google.com/store/apps/details?id=at.mroland.android.apps.nfctaginfo)，[NFC TagInfo by NXP](https://play.google.com/store/apps/details?id=com.nxp.taginfolite)
