#include <stdio.h>  
#include <windows.h>  
#include <conio.h>  // 为了使用getch()函数  
  
#define SERIAL_PORT_NAME "COM4"  
#define BAUD_RATE_HIGH 115200
#define BAUD_RATE_LOW 38400  
#define TIMEOUT 5000  
#define WRITE_BUFFER_SIZE 1  
#define READ_BUFFER_SIZE 1024  

int main() {  
    HANDLE hSerial;  
    DCB dcbSerialParams = {0};  
    COMMTIMEOUTS timeouts = {0};  
    char writeBuffer[WRITE_BUFFER_SIZE] = {0xfff6};  
    char readBuffer[READ_BUFFER_SIZE];  
    DWORD bytesRead;  
    int success;  
    int baudRateChoice;
    int BAUD_RATE;
    
   do {  
        printf("请按1选择115200波特率，按2选择38400波特率：\n");  
        _getch();  // 等待用户按键，但不显示到控制台  
        baudRateChoice = _getch() - '0';  // 获取用户输入的数字并转换为整数  
    } while (baudRateChoice != 1 && baudRateChoice != 2);  
  
    if (baudRateChoice == 1) {  
        BAUD_RATE = BAUD_RATE_HIGH;  // 设置波特率为115200  
    } else {  
        BAUD_RATE = BAUD_RATE_LOW;  // 设置波特率为38400  
    }
    // 打开串行端口  
    hSerial = CreateFile(SERIAL_PORT_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  
    if (hSerial == INVALID_HANDLE_VALUE) {  
        printf("扫描不到读卡器，请确认读卡器已连接并且分配到端口COM4\n");  
	printf("按任意键退出......");
        getch();  // 等待用户按键退出  
        return 1;  
    }  
  
    // 设置串行端口参数  
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);  
    if (!GetCommState(hSerial, &dcbSerialParams)) {  
        printf("无法获取串行端口当前状态\n");  
	printf("按任意键退出......");
        getch();  // 等待用户按键退出  
        CloseHandle(hSerial);  
        return 1;  
    }  
    dcbSerialParams.BaudRate = BAUD_RATE;  
    dcbSerialParams.ByteSize = 8;  
    dcbSerialParams.StopBits = ONESTOPBIT;  
    dcbSerialParams.Parity   = NOPARITY;  
    if (!SetCommState(hSerial, &dcbSerialParams)) {  
        printf("无法设置串行端口状态\n");  
	printf("按任意键退出......");
        getch();  // 等待用户按键退出  
        CloseHandle(hSerial);  
        return 1;  
    }  
  
    // 设置串行端口超时参数  
    timeouts.ReadIntervalTimeout = MAXDWORD;  
    timeouts.ReadTotalTimeoutConstant = 0;  
    timeouts.ReadTotalTimeoutMultiplier = 0;  
    timeouts.WriteTotalTimeoutConstant = 0;  
    timeouts.WriteTotalTimeoutMultiplier = TIMEOUT;  
    if (!SetCommTimeouts(hSerial, &timeouts)) {  
        printf("无法设置串行端口超时参数\n");  
	printf("按任意键退出......");
        getch();  // 等待用户按键退出  
        CloseHandle(hSerial);  
        return 1;  
    }  
  
    // 向串行端口写入数据并检查返回值  
    success = WriteFile(hSerial, writeBuffer, WRITE_BUFFER_SIZE, &bytesRead, NULL);  
    if (success == 0) {  
        printf("发送指令失败\n");  
	printf("按任意键退出......");
        getch();  // 等待用户按键退出  
        CloseHandle(hSerial);  
        return 1;  
    } else {  
        printf("发送指令成功\n");  
    }  
  
    // 从串行端口读取数据并检查返回值和数据内容  
    success = ReadFile(hSerial, readBuffer, READ_BUFFER_SIZE, &bytesRead, NULL);  
    if (success == 0) {  
        printf("读取数据失败\n");  
        getch();  // 等待用户按键退出  
        CloseHandle(hSerial);  
        return 1;  
    } else {  
        printf("读取数据成功\n");  
        if (bytesRead == 2 && readBuffer[0] == 0xff && readBuffer[1] == 0x99 && readBuffer[2] == 0xaa) {  
            printf("返回的数据是0xff99aa，操作成功\n");  
        } else {  
            printf("返回的数据不是0xff99aa，操作失败\n");  
        }  
	getch();  // 等待用户按键退出，然后关闭串行端口
}}