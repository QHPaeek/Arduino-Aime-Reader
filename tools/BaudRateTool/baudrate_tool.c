//作者：Qinh
//用AI写的
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <stdbool.h>
#include <ctype.h>

#define HIGH_BAUDRATE 115200 // 高波特率
#define LOW_BAUDRATE 38400 // 低波特率
#define DATA_LENGTH 12 // 回复数据的长度
#define CHECKSUM_OFFSET 0xE0 // 校验位的偏移量
#define LED_MIN 2 // LED亮度的最小值
#define LED_MAX 255 // LED亮度的最大值

HANDLE hPort; // 串口句柄
DCB dcb; // 串口参数结构体
COMMTIMEOUTS timeouts; // 串口超时结构体
char comPort[10];
uint8_t send_buffer[8] = {0xE0, 0x06, 0x00, 0x00, 0xF6, 0x00, 0x00, 0xFC}; // 发送数据缓冲区
uint8_t recv_buffer[DATA_LENGTH]; // 接收数据缓冲区
uint8_t system_setting_buffer[2] = {0}; // 系统设置缓冲区
BOOL high_baudrate_mode = FALSE; // 高波特率模式标志
uint8_t change_highbaudrate_mode = 0;
BOOL led_enabled = FALSE; // LED启用标志
uint8_t led_brightness = 0; // LED亮度
uint8_t firmware_version = 0; // 固件版本号
uint8_t hardware_version = 0; // 硬件版本号

// 打开串口的函数，返回值为BOOL，表示是否成功
BOOL open_port()
{
    // 打开串口
    hPort = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPort == INVALID_HANDLE_VALUE)
    {
        printf("无法打开串口%s！\n", comPort);
        return FALSE;
    }

    // 获取串口参数
    if (!GetCommState(hPort, &dcb))
    {
        printf("无法获取串口%s的参数！\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // 设置串口参数
    dcb.BaudRate = HIGH_BAUDRATE; // 设置波特率为高波特率
    dcb.ByteSize = 8; // 设置数据位为8
    dcb.Parity = NOPARITY; // 设置无奇偶校验
    dcb.StopBits = ONESTOPBIT; // 设置停止位为1
    if (!SetCommState(hPort, &dcb))
    {
        printf("无法设置串口%s的参数！\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // 获取串口超时
    if (!GetCommTimeouts(hPort, &timeouts))
    {
        printf("无法获取串口%s的超时！\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // 设置串口超时
    timeouts.ReadIntervalTimeout = 50; // 设置读取间隔超时为50毫秒
    timeouts.ReadTotalTimeoutConstant = 1000; // 设置读取总超时常量为1000毫秒
    timeouts.ReadTotalTimeoutMultiplier = 10; // 设置读取总超时乘数为10毫秒
    timeouts.WriteTotalTimeoutConstant = 1000; // 设置写入总超时常量为1000毫秒
    timeouts.WriteTotalTimeoutMultiplier = 10; // 设置写入总超时乘数为10毫秒
    if (!SetCommTimeouts(hPort, &timeouts))
    {
        printf("无法设置串口%s的超时！\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // 返回成功
    return TRUE;
}

// 关闭串口的函数，无返回值
void close_port()
{
    // 关闭串口
    CloseHandle(hPort);
}

// 发送数据的函数，参数为数据长度，返回值为BOOL，表示是否成功
BOOL send_data(int length)
{
    DWORD bytes_written; // 写入的字节数
    // 写入数据
    if (!WriteFile(hPort, send_buffer, length, &bytes_written, NULL))
    {
        printf("无法向串口%s写入数据！\n", comPort);
        return FALSE;
    }
    // 检查写入的字节数是否正确
    if (bytes_written != length)
    {
        printf("向串口%s写入数据不完整！\n", comPort);
        return FALSE;
    }
    // 返回成功
    return TRUE;
}

// 接收数据的函数，参数为数据长度，返回值为BOOL，表示是否成功
BOOL recv_data(int length)
{
    DWORD bytes_read; // 读取的字节数
    // 读取数据
    if (!ReadFile(hPort, recv_buffer, length, &bytes_read, NULL))
    {
        printf("无法从串口%s读取数据！\n", comPort);
        return FALSE;
    }
    // 检查读取的字节数是否正确
    if (bytes_read != length)
    {
        //printf("从串口%s读取数据不完整！\n", comPort);
        return FALSE;
    }
    // 返回成功
    return TRUE;
}

// 处理数据的函数，参数为数据长度，返回值为BOOL，表示是否成功
BOOL process_data(int length)
{
    int i; // 循环变量
    uint8_t checksum = 0; // 校验位
    // 检查第一个字节是否为E0
    if (recv_buffer[0] != 0xE0)
    {
        printf("读卡器连接错误！请检查连接！ERROR01\n");
        return FALSE;
    }
    // 计算校验位
    for (i = 0; i < length - 1; i++)
    {
        checksum += recv_buffer[i];
    }
    checksum -= CHECKSUM_OFFSET;
    // 检查校验位是否正确
    if (checksum != recv_buffer[length - 1])
    {
        printf("读卡器连接错误！请检查连接！REEOR02\n");
        return FALSE;
    }
    // 解析数据
    high_baudrate_mode = recv_buffer[7] & 0x02; // 第8个字节的第2位表示高波特率模式
    led_enabled = recv_buffer[7] & 0x04; // 第8个字节的第3位表示LED启用
    led_brightness = recv_buffer[8]; // 第9个字节表示LED亮度
    firmware_version = recv_buffer[9]; // 第10个字节表示固件版本号
    hardware_version = recv_buffer[10]; // 第11个字节表示硬件版本号
    // 返回成功
    return TRUE;
}

// 打印数据的函数，无参数，无返回值
void print_data()
{
    // 打印数据
    printf("读卡器状态信息如下：\n");
    printf("高波特率模式：%s\n", high_baudrate_mode ? "是" : "否");
    printf("LED启用：%s\n", led_enabled ? "是" : "否");
    printf("LED亮度：%d\n", led_brightness);
    printf("固件版本：v%d\n", firmware_version);
	switch(hardware_version)
	{
		case 1:
			printf("硬件版本：ATmega32U4\n");
			break;
		case 2:
			printf("硬件版本：SAMD_ZERO\n");
			break;
		case 3:
			printf("硬件版本：ESP8266\n");
			break;
		case 4:
			printf("硬件版本：ESP32\n");
			break;
		case 5:
			printf("硬件版本：AIR001/PY32F002\n");
			break;
		case 6:
			printf("硬件版本：STM32F1\n");
			break;
		case 7:
			printf("硬件版本：STM32F0\n");
			break;
		case 8:
			printf("硬件版本：RP2040\n");
			break;
		case 9:
			printf("硬件版本：ATmega328P\n");
			break;
		default:
			printf("硬件版本：未知\n");
			break;
	}	
}

// 修改波特率的函数，参数为波特率，返回值为BOOL，表示是否成功
BOOL change_baudrate(int baudrate)
{
    // 获取串口参数
    if (!GetCommState(hPort, &dcb))
    {
        printf("无法获取串口%s的参数！\n", comPort);
        return FALSE;
    }
    // 设置串口参数
    dcb.BaudRate = baudrate; // 设置波特率
    if (!SetCommState(hPort, &dcb))
    {
        printf("无法设置串口%s的参数！\n", comPort);
        return FALSE;
    }
    // 返回成功
    return TRUE;
}

// 获取用户输入的函数，参数为提示信息，返回值为char，表示用户输入的字符
char get_user_input(char *prompt)
{
    char input; // 用户输入的字符
    // 打印提示信息
    printf("%s", prompt);
    // 获取用户输入
    input = getch();
    // 返回用户输入
    return input;
}

// 获取用户输入的数字的函数，参数为提示信息，返回值为int，表示用户输入的数字
int get_user_input_number(char *prompt)
{
    char input[256]; // 用户输入的字符串
    int number; // 用户输入的数字
    // 打印提示信息
    printf("%s", prompt);
    // 获取用户输入
    fgets(input, 256, stdin);
    // 尝试将输入转换为数字
    if (sscanf(input, "%d", &number) != 1)
    {
        // 转换失败，返回-1
        return -1;
    }
    // 转换成功，返回数字
    return number;
}
bool convert_string_to_hex(const char* str, uint8_t* output) {

    // 检查字符串内容是否全部为0~9或A~F
    for (size_t i = 0; i<16; i++) {
        if (!isxdigit(str[i])) {
            return false;
        }
    }

    // 将字符串转换为16进制并存储到数组中
    for (size_t i = 0; i < 8; i++) {
        sscanf(str + 2 * i, "%2hhx", &output[i]);
    }

    return true;
}
bool convert_string_to_decimal(const char* str, uint8_t* output) {

    // 检查字符串内容是否全部为0~9
    for (size_t i = 0; i < 20; i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }

    // 将字符串转换为10进制并存储到数组中
    for (size_t i = 0; i < 20; i++) {
        output[i] = str[i] - '0';
    }

    return true;
}
bool process_array(uint8_t* input, uint8_t* output) {
    for (int i = 0; i < 20; i++) {
        if (input[i] < 0 || input[i] > 9) {
            return false;
        }
    }

    for (int i = 0; i < 10; i++) {
        output[i] = (input[2*i] << 4) | input[2*i + 1];
    }

    return true;
}
// 主函数
int main()
{
printf("本工具用于修改Aime读卡器内部设置，源代码见https://github.com/QHPaeek/Arduino-Aime-Reader\n");
printf("读卡器EEPROM寿命有限，请不要频繁修改！\n");
int ports;
while(1)
{
	ports = get_user_input_number("请输入读卡器的端口号（例如com4请输入数字4）按下回车继续：");
	if (ports < 0){
		printf("请输入有效的数字！");
		continue;
	}
	if(ports > 9 ){
	snprintf(comPort, sizeof(comPort), "\\\\.\\COM%d", ports);
	break;
	}
	snprintf(comPort, sizeof(comPort), "COM%d", ports);
	break;
}
    // 打开串口
    if (!open_port())
    {
        // 打开失败，等待用户按下任意键退出程序
        getch();
        return -1;
    }
	uint8_t mode_rst_cmd[30]={0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf,0xaf};
	change_baudrate(LOW_BAUDRATE);
	while(!WriteFile(hPort, mode_rst_cmd, 30, NULL, NULL));
	change_baudrate(HIGH_BAUDRATE);
	while(!WriteFile(hPort, mode_rst_cmd, 30, NULL, NULL));
//以两个不同波特率发30个0xff，即读卡器模式初始化命令，确保读卡器此时切换回sega模式
    // 向COM4端口以115200波特率输出16进制数据E0 06 00 00 F6 00 00 FC，并且监听回复。
    if (!send_data(8))
    {
        // 发送失败，关闭串口，等待用户按下任意键退出程序
        close_port();
        getch();
        return -1;
    }
    // 接收数据
    if (!recv_data(DATA_LENGTH))
    {
        // 接收失败，尝试以38400波特率重新输出上述数据，并且监听回复。
        if (!change_baudrate(LOW_BAUDRATE))
        {
            // 修改波特率失败，关闭串口，等待用户按下任意键退出程序
            close_port();
            getch();
            return -1;
        }
        if (!send_data(8))
        {
            // 发送失败，关闭串口，等待用户按下任意键退出程序
            close_port();
            getch();
            return -1;
        }
        if (!recv_data(DATA_LENGTH))
        {
            // 接收失败，关闭串口，等待用户按下任意键退出程序
            printf("读卡器连接错误！请检查连接！\n");
            close_port();
            getch();
            return -1;
        }
    }
    // 处理数据
    if (!process_data(DATA_LENGTH))
    {
        // 处理失败，关闭串口，等待用户按下任意键退出程序
        close_port();
        getch();
        return -1;
    }
    // 打印数据
    print_data();
    char choice; 
uint8_t mode_sw = 0;
    while (1)
    {
	printf("提示：V4版本以下不支持读卡测试模式，V7版本以下不支持卡号映射功能\n");
	printf("提示：进入读卡测试模式后只能通过拔线退出，如果不退出，读卡器将无法在正常模式工作\n");
	printf("提示：连接此工具后读卡器已被重置为默认sega模式\n");
        choice = get_user_input("输入1修改sega模式下读卡器设置，输入2进入读卡测试模式，输入3进入Namco模式，输入4进入Spice模式，输入5进入PN532直通模式，输入n退出\n");
        // 判断用户输入
        if (choice == '1' )
        {
	mode_sw = 1;
            break;
        }
        else if (choice == '2' )
        {
	mode_sw = 2;
            break;
        }
        else if (choice == '3' )
        {
		DWORD bytes_written;
		uint8_t send_buffer_readtest_cmd[8] = {0xE0 ,0x06 ,00,00,0xF8 ,0x01,0x02,0x01};
		while((WriteFile(hPort, send_buffer_readtest_cmd,8, &bytes_written, NULL) == FALSE));
		printf("模式修改完成，按任意键退出\n");
        	getch();
        	return -1;
        }
        else if (choice == '4' )
        {
		DWORD bytes_written;
		uint8_t send_buffer_readtest_cmd[8] = {0xE0 ,0x06 ,00,00,0xF8 ,0x01,0x01,0x00};
		while((WriteFile(hPort, send_buffer_readtest_cmd,8, &bytes_written, NULL) == FALSE));
		printf("模式修改完成，按任意键退出\n");
        	getch();
        	return -1;
        }
        else if (choice == '5' )
        {
		DWORD bytes_written;
		uint8_t send_buffer_readtest_cmd[8] = {0xE0 ,0x06 ,00,00,0xF8 ,0x01,0x04,0x03};
		while((WriteFile(hPort, send_buffer_readtest_cmd,8, &bytes_written, NULL) == FALSE));
		printf("模式修改完成，按任意键退出\n");
        	getch();
        	return -1;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // 输入n或者N，关闭串口，退出程序
            close_port();
            return 0;
        }
        else
        {
            // 输入其他内容，提示用户”请重新输入！“
            printf("请重新输入！\n");
        }
    }
if (mode_sw == 1){
    if (high_baudrate_mode)
    {
        // 高波特率模式，设置波特率为115200
        if (!change_baudrate(HIGH_BAUDRATE))
        {
            // 修改波特率失败，关闭串口，等待用户按下任意键退出程序
            close_port();
            getch();
            return -1;
        }
    }
    else
    {
        // 低波特率模式，设置波特率为38400
        if (!change_baudrate(LOW_BAUDRATE))
        {
            // 修改波特率失败，关闭串口，等待用户按下任意键退出程序
            close_port();
            getch();
            return -1;
        }
    }
    while (1)
    {
        // 获取用户输入
        choice = get_user_input("是否启用高波特率模式？输入y启用，输入n不启用\n");
        // 判断用户输入
        if (choice == 'y' || choice == 'Y')
        {
            // 输入y或者Y，设置system_setting_buffer[0] 的第二位置1，跳出循环
            system_setting_buffer[0] |= 0x02;
		change_highbaudrate_mode = 1;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // 输入n或者N，跳出循环
		change_highbaudrate_mode = 0;
            break;
        }
        else
        {
            // 输入其他内容，提示用户”请重新输入！“
            printf("请重新输入！\n");
        }
    }
    while (1)
    {
        // 获取用户输入
        choice = get_user_input("是否启用LED？输入y启用，输入n不启用\n");
        // 判断用户输入
        if (choice == 'y' || choice == 'Y')
        {
            // 输入y或者Y，设置system_setting_buffer[0] 的第三位置1，跳出循环
            system_setting_buffer[0] |= 0x04;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // 输入n或者N，跳出循环
            break;
        }
        else
        {
            // 输入其他内容，提示用户”请重新输入！“
            printf("请重新输入！\n");
        }
    }
    int brightness; // LED亮度
    while (1)
    {
        // 获取用户输入的数字
        brightness = get_user_input_number("请输入LED亮度范围，范围为0~255，按回车继续\n");
        // 判断用户输入的数字是否合法
        if (brightness >= 0 && brightness <= 255)
        {
            // 合法，将输入的数值赋给system_setting_buffer[1]，跳出循环
            system_setting_buffer[1] = brightness;
            break;
        }
        else
        {
            // 不合法，提示用户”请重新输入!“
            printf("请重新输入!\n");
        }
    }
uint8_t card_reflect = 0;
    while (1)
    {
        // 获取用户输入
        choice = get_user_input("是否启用卡号映射功能？输入y启用，输入n不启用\n");
        // 判断用户输入
        if (choice == 'y' || choice == 'Y')
        {
            // 输入y或者Y，设置system_setting_buffer[0] 的第4位置1，跳出循环
            system_setting_buffer[0] |= 0b1000;
	    card_reflect = 1;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // 输入n或者N，跳出循环
            break;
        }
        else
        {
            // 输入其他内容，提示用户”请重新输入！“
            printf("请重新输入！\n");
        }
    }
char card_IDm[8];
char card_accesscode[10] = {0};
if(card_reflect)
{	
	printf("请输入被转换卡的IDm(16进制格式，`6位，无前缀，中间不需要空格）,按回车结束\n");
	printf("IDm可以通过本工具的读卡测试模式获取\n");
	bool transform_result_IDm = false;
	while(!transform_result_IDm){
		char char_buffer[256];
		fgets(char_buffer, 256, stdin);
		if(convert_string_to_hex(char_buffer,card_IDm)){
			transform_result_IDm = true;
		}
		else{
			printf("输入格式错误！请重新输入\n");
		}
	}
	printf("请输入目标转换卡的卡号(10进制格式，20位，无前缀，中间不需要空格）,按回车结束\n");
	bool transform_result_accode = false;
	while(!transform_result_accode){
		char char_buffer[256];
		uint8_t accode_buffer[20];
		fgets(char_buffer, 256, stdin);
		if(convert_string_to_decimal(char_buffer,accode_buffer)) {
			transform_result_accode = true;
			process_array(accode_buffer, card_accesscode);
		}
		else{
			printf("输入格式错误！请重新输入\n");
		}
	}
 }
if(high_baudrate_mode)
{
	change_baudrate(115200);
}
else
{
	change_baudrate(38400);
}
uint8_t uart_send_buffer[28] = {0xE0,0x1A,0x00,0x00,0xF7,0x14,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uart_send_buffer[6] = system_setting_buffer[0]; 
uart_send_buffer[7] = system_setting_buffer[1]; 
for(uint8_t i =0;i<8;i++){
	uart_send_buffer[i+8] = card_IDm[i];
}
for(uint8_t i =0;i<10;i++){
	uart_send_buffer[i+16] =card_accesscode[i];
}
uint16_t checksum_cmd;
for(uint8_t i =0;i<26;i++){
	uart_send_buffer[27] += uart_send_buffer[i+1];
}
DWORD bytes_written;
//OVERLAPPED overlapped = {0};
//overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//if (!WriteFile(hPort, uart_send_buffer, 28, &bytes_written, &overlapped))
//{
//        GetOverlappedResult(hPort, &overlapped, &bytes_written, TRUE);
//}
//CloseHandle(overlapped.hEvent);
WriteFile(hPort, uart_send_buffer,28, &bytes_written, NULL);
Sleep(3);
 // 等待串口数据发送完成
// 改变串口波特率
    if (change_highbaudrate_mode)
    {
        // 用户选择了高波特率模式，设置波特率为115200
        change_baudrate(HIGH_BAUDRATE);
    }
    else
    {
        // 用户没有选择高波特率模式，设置波特率为38400
	change_baudrate(LOW_BAUDRATE);
    }
printf("修改完成！按任意键退出\n");
close_port();
getch();
return -1;
}
else{
	if (high_baudrate_mode)
    {
        if (!change_baudrate(HIGH_BAUDRATE))
        {  
            close_port();
            getch();
            return -1;
        }
    }
    else
    {
        if (!change_baudrate(LOW_BAUDRATE))
        {
            close_port();
            getch();
            return -1;
        }
    }
DWORD bytes_written;
char buffer[1024];
DWORD bytesRead;
uint8_t send_buffer_readtest_cmd[8] = {0xE0 ,0x06 ,00,00,0xF8 ,0x01,0x03,0x02};
while((WriteFile(hPort, send_buffer_readtest_cmd,8, &bytes_written, NULL) == FALSE));
 while (1) {
        while(ReadFile(hPort, buffer, sizeof(buffer), &bytesRead, NULL) == FALSE);
	system("cls");
        for (DWORD i = 0; i < bytesRead; i++) {
            printf("%c", buffer[i]);
        }
    }
}
}
