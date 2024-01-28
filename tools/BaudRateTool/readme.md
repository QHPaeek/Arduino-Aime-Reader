本工具用于非CDC的读卡器设备通过自定义协议修改波特率，以适应不同游戏。

stm32等通过USB  CDC的设备由于CDC无视波特率的特性不需要使用本工具进行调整。

用法：

在此目录下打开CMD

执行 `gcc .\baudrate_tool.c -o baudrate_tool.exe`以编译程序。(mingw32)

运行编译出来的EXE
