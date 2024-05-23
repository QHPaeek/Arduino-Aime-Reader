刷写命令：.\AirISP.exe -c air001 -p 端口号 -b 115200 write_flash -e 0x08000000 .\固件.bin
例如Air001在COM1端口：
.\AirISP.exe -c air001 -p COM1 -b 115200 write_flash -e 0x08000000 .\Arduino-Aime-Reader-Air001-V801-SSRR.bin
刷写前请参照Kobato-Lite使用说明书中固件升级部分，使读卡器进入下载模式