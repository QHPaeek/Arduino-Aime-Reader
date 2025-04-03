// 作者：Qinh
// 用AI写的
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <stdbool.h>
#include <ctype.h>

#define HIGH_BAUDRATE 115200 // 高波特率
#define LOW_BAUDRATE 38400   // 低波特率
#define DATA_LENGTH 12       // 回复数据的长度
#define CHECKSUM_OFFSET 0xE0 // 校验位的偏移量
#define LED_MIN 2            // LED亮度的最小值
#define LED_MAX 255          // LED亮度的最大值

HANDLE hPort;          // 串口句柄
DCB dcb;               // 串口参数结构体
COMMTIMEOUTS timeouts; // 串口超时结构体

HANDLE hConsole; // 终端
WORD defaultAttributes;

char comPort[13];
uint8_t recv_buffer[DATA_LENGTH];       // 接收数据缓冲区
uint8_t system_setting_buffer[2] = {0}; // 系统设置缓冲区
BOOL high_baudrate_mode = FALSE;        // 高波特率模式标志
uint8_t change_highbaudrate_mode = 0;
BOOL led_enabled = FALSE;     // LED启用标志
uint8_t led_brightness = 0;   // LED亮度
uint8_t firmware_version = 0; // 固件版本号
uint8_t hardware_version = 0; // 硬件版本号

enum
{
    CMD_READ_EEPROM = 0xf6,
    CMD_WRITE_EEPROM = 0xf7,
    CMD_SW_MODE = 0xf8,
    CMD_READ_MODE = 0xf9,
};

enum
{
    SEGA_MODE = 0,
    SPICE_MODE = 1,
    NAMCO_MODE = 2,
    TEST_MODE = 3,
    RAW_MODE = 4,
};

enum
{
    STATUS_OK = 0x00,
    STATUS_CARD_ERROR = 0x01,
    STATUS_NOT_ACCEPT = 0x02,
    STATUS_INVALID_COMMAND = 0x03,
    STATUS_INVALID_DATA = 0x04,
    STATUS_SUM_ERROR = 0x05,
    STATUS_INTERNAL_ERROR = 0x06,
    STATUS_INVALID_FIRM_DATA = 0x07,
    STATUS_FIRM_UPDATE_SUCCESS = 0x08,
    STATUS_COMP_DUMMY_2ND = 0x10,
    STATUS_COMP_DUMMY_3RD = 0x20,
};

typedef union
{
    uint8_t bytes[128];
    struct
    {
        uint8_t frame_len;
        uint8_t addr;
        uint8_t seq_no;
        uint8_t cmd;
        uint8_t payload_len;
        union
        {
            uint8_t mode;
            struct
            {
                uint8_t eeprom_data[2]; // 系统内部设置
                uint8_t mapped_IDm[8];
                uint8_t target_accesscode[10];
            };
        };
    };
} packet_request_t;

typedef union
{
    uint8_t bytes[128];
    struct
    {
        uint8_t frame_len;
        uint8_t addr;
        uint8_t seq_no;
        uint8_t cmd;
        uint8_t status;
        uint8_t payload_len;
        union
        {
            uint8_t mode;
            uint8_t version[1]; // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
            uint8_t block[16];  // CMD_MIFARE_READ
            uint8_t eeprom_data[4];
        };
    };
} packet_response_t;

packet_request_t req;
packet_response_t res;

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
    dcb.ByteSize = 8;             // 设置数据位为8
    dcb.Parity = NOPARITY;        // 设置无奇偶校验
    dcb.StopBits = ONESTOPBIT;    // 设置停止位为1
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
    timeouts.ReadIntervalTimeout = 50;         // 设置读取间隔超时为50毫秒
    timeouts.ReadTotalTimeoutConstant = 1000;  // 设置读取总超时常量为1000毫秒
    timeouts.ReadTotalTimeoutMultiplier = 10;  // 设置读取总超时乘数为10毫秒
    timeouts.WriteTotalTimeoutConstant = 100;  // 设置写入总超时常量为100毫秒
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

void close_port()
{
    CloseHandle(hPort);
}

void add_data(uint8_t *buffer, uint8_t send_byte, uint8_t *len)
{
    buffer[*len] = send_byte;
    memset(len, *len + 1, 1);
}

BOOL recv_data(uint8_t recv_buffer)
{
    DWORD bytes_read;
    return ReadFile(hPort, &recv_buffer, 1, &bytes_read, NULL);
}
uint8_t len, r, checksum;
bool escape = false;

uint8_t packet_read()
{
    uint8_t recv_buffer[128];
    long unsigned int recv_len;
    ReadFile(hPort, recv_buffer, 128, &recv_len, NULL);
    res.frame_len = 0;
    for (uint8_t i = 0; i < recv_len; i++)
    {
        r = recv_buffer[i];
        printf("%x ", r);
        if (r == 0xE0)
        {
            res.frame_len = 0xFF;
            continue;
        }
        if (res.frame_len == 0)
        {
            continue;
        }
        if (res.frame_len == 0xFF)
        {
            res.frame_len = r;
            len = 0;
            checksum = r;
            continue;
        }
        if (r == 0xD0)
        {
            escape = true;
            continue;
        }
        if (escape)
        {
            r++;
            escape = false;
        }
        res.bytes[++len] = r;
        if (len == res.frame_len)
        {
            return checksum == r ? res.cmd : STATUS_SUM_ERROR;
        }
        checksum += r;
    }
    return 0;
}

void packet_write()
{
    uint8_t checksum = 0, len = 0;
    uint8_t send_buffer[128];
    uint8_t send_len;
    if (req.cmd == 0)
    {
        return;
    }
    add_data(send_buffer, 0xE0, &send_len);
    while (len <= req.frame_len)
    {
        uint8_t w;
        if (len == req.frame_len)
        {
            w = checksum;
        }
        else
        {
            w = req.bytes[len];
            checksum += w;
        }
        if (w == 0xE0 || w == 0xD0)
        {
            add_data(send_buffer, 0xD0, &send_len);
            add_data(send_buffer, --w, &send_len);
        }
        else
        {
            add_data(send_buffer, w, &send_len);
        }
        len++;
    }
    // if (!GetCommState(hPort, &dcb))
    // {
    //     printf("A");
    // }
    WriteFile(hPort, send_buffer, send_len, NULL, NULL);
    // if (!GetCommState(hPort, &dcb))
    // {
    //     printf("B");
    // }
    req.cmd = 0;
}

void req_read_eeprom()
{
    req.frame_len = 6 + 0;
    req.addr = 0;
    req.seq_no = 0;
    req.cmd = CMD_READ_EEPROM;
    req.payload_len = 0;
    packet_write();
}

void req_write_eeprom(uint8_t *eeprom_data, uint8_t *mapped_IDm, uint8_t *target_accesscode)
{
    req.frame_len = 6 + 20;
    req.addr = 0;
    req.seq_no = 0;
    req.cmd = CMD_READ_EEPROM;
    req.payload_len = 20;
    memcpy(req.eeprom_data, eeprom_data, 2);
    memcpy(req.mapped_IDm, mapped_IDm, 8);
    memcpy(req.target_accesscode, target_accesscode, 10);
    packet_write();
}

void req_change_mode(uint8_t mode)
{
    req.frame_len = 6 + 1;
    req.addr = 0;
    req.seq_no = 0;
    req.cmd = CMD_SW_MODE;
    req.payload_len = 1;
    req.mode = mode;
    packet_write();
}

// 修改波特率的函数，参数为波特率，返回值为BOOL，表示是否成功
BOOL change_baudrate(int baudrate)
{
    // DWORD errors;
    // COMSTAT comStat;
    // ClearCommError(hPort, &errors, &comStat);
    if (!GetCommState(hPort, &dcb))
    {
        printf("无法获取串口%s的参数！ERROR:%lu\n", comPort, GetLastError());
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
    int number;      // 用户输入的数字
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
bool convert_string_to_hex(const char *str, uint8_t *output)
{

    // 检查字符串内容是否全部为0~9或A~F
    for (size_t i = 0; i < 16; i++)
    {
        if (!isxdigit(str[i]))
        {
            return false;
        }
    }

    // 将字符串转换为16进制并存储到数组中
    for (size_t i = 0; i < 8; i++)
    {
        sscanf(str + 2 * i, "%2hhx", &output[i]);
    }

    return true;
}
bool convert_string_to_decimal(const char *str, uint8_t *output)
{

    // 检查字符串内容是否全部为0~9
    for (size_t i = 0; i < 20; i++)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }

    // 将字符串转换为10进制并存储到数组中
    for (size_t i = 0; i < 20; i++)
    {
        output[i] = str[i] - '0';
    }

    return true;
}
bool process_array(uint8_t *input, uint8_t *output)
{
    for (int i = 0; i < 20; i++)
    {
        if (input[i] < 0 || input[i] > 9)
        {
            return false;
        }
    }

    for (int i = 0; i < 10; i++)
    {
        output[i] = (input[2 * i] << 4) | input[2 * i + 1];
    }

    return true;
}

void setTextColor(WORD color)
{
    SetConsoleTextAttribute(hConsole, color);
}

void resetTextColor()
{
    SetConsoleTextAttribute(hConsole, defaultAttributes);
}

void printColoredStatus(BOOL isEnabled)
{
    if (isEnabled)
    {
        setTextColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY); // 亮绿色
        printf("√ 启用");
        resetTextColor();
    }
    else
    {
        setTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY); // 亮红色
        printf("X 禁用");
        resetTextColor();
    }
}

void printColoredResult(BOOL isSuccess, char *message)
{
    if (isSuccess)
    {
        setTextColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY); // 亮绿色
        printf("√ %s\n", message);
        resetTextColor();
    }
    else
    {
        setTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY); // 亮红色
        printf("X %s\n", message);
        resetTextColor();
    }
}

// 打印超链接的函数，参数为URL和文本
void printHyperlink(const char* url, const char* text) {
    // 获取控制台的当前模式
    DWORD mode;
    GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
    
    // 启用虚拟终端序列处理
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    // 输出带超链接的文本 (使用ESC序列)
    printf("\x1b]8;;%s\x07%s\x1b]8;;\x07", url, text);
    
    // 恢复原来的控制台模式
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), mode);
}


// 主函数
int main()
{
    // 终端初始化
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    defaultAttributes = consoleInfo.wAttributes;

    printf("BaudRateTool V10 By Qinh\n");
    printf("本工具用于修改Kobato (Lite) 读卡器内部设置\n");
    printf("源代码可见");
    printHyperlink("https://github.com/QHPaeek/Arduino-Aime-Reader", "https://github.com/QHPaeek/Arduino-Aime-Reader");
    printf("\n\n");
    printf("读卡器EEPROM寿命有限，请不要频繁修改！\n\n");
    int ports;
    while (1)
    {
        ports = get_user_input_number("请输入读卡器的端口号（例如COM4请输入数字4），端口号可通过设备管理器查看\n输入数字后请按下回车继续：");
        if (ports < 0)
        {
            printf("请输入有效的数字！");
            continue;
        }
        if (ports > 9)
        {
            snprintf(comPort, sizeof(comPort), "\\\\.\\COM%d", ports);
            break;
        }
        snprintf(comPort, sizeof(comPort), "COM%d", ports);
        break;
    }
    // 打开串口
    if (!open_port())
    {
        printColoredResult(FALSE, "打开串口失败！");
        // 打开失败，等待用户按下任意键退出程序
        getch();
        return -1;
    }
    uint8_t mode_rst_cmd[30] = {0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf};
    change_baudrate(LOW_BAUDRATE);
    WriteFile(hPort, mode_rst_cmd, 30, NULL, NULL);
    Sleep(1);
    change_baudrate(HIGH_BAUDRATE);
    WriteFile(hPort, mode_rst_cmd, 30, NULL, NULL);
    Sleep(1000);
    // 以两个不同波特率发30个0xaf，即读卡器模式初始化命令，确保读卡器此时切换回sega模式
    //  向COM4端口以115200波特率输出16进制数据E0 06 00 00 F6 00 00 FC，并且监听回复。
    uint8_t send_buffer1[8] = {0xE0, 0x06, 00, 00, 0xF6, 00, 00, 0xFC};
    WriteFile(hPort, send_buffer1, 8, NULL, NULL);
    // req_read_eeprom();
    //  接收数据
    if (packet_read() != CMD_READ_EEPROM)
    {
        // 接收失败，尝试以38400波特率重新输出上述数据，并且监听回复。
        if (!change_baudrate(LOW_BAUDRATE))
        {
            // 修改波特率失败，关闭串口，等待用户按下任意键退出程序
            close_port();
            getch();
            return -1;
        }
        WriteFile(hPort, send_buffer1, 8, NULL, NULL);
        if (packet_read() != CMD_READ_EEPROM)
        {
            // 接收失败，关闭串口，等待用户按下任意键退出程序
            printColoredResult(FALSE, "读卡器连接错误！请检查连接！\n");
            close_port();
            getch();
            return -1;
        }
    }
    // 处理数据
    high_baudrate_mode = res.eeprom_data[0] & 0x02; // 第8个字节的第2位表示高波特率模式
    led_enabled = res.eeprom_data[0] & 0x04;        // 第8个字节的第3位表示LED启用
    led_brightness = res.eeprom_data[1];            // 第9个字节表示LED亮度
    firmware_version = res.eeprom_data[2];          // 第10个字节表示固件版本号
    hardware_version = res.eeprom_data[3];          // 第11个字节表示硬件版本号

    // 获取硬件版本的字符串描述
    char hardware_version_str[32] = {0};
    switch (hardware_version)
    {
    case 1:
        strcpy(hardware_version_str, "ATmega32U4");
        break;
    case 2:
        strcpy(hardware_version_str, "SAMD21");
        break;
    case 3:
        strcpy(hardware_version_str, "ESP8266");
        break;
    case 4:
        strcpy(hardware_version_str, "ESP32");
        break;
    case 5:
        strcpy(hardware_version_str, "AIR001/PY32F002");
        break;
    case 6:
        strcpy(hardware_version_str, "STM32F1");
        break;
    case 7:
        strcpy(hardware_version_str, "STM32F0");
        break;
    case 8:
        strcpy(hardware_version_str, "RP2040");
        break;
    case 9:
        strcpy(hardware_version_str, "ATmega328P");
        break;
    case 10:
        strcpy(hardware_version_str, "ESP32C3");
        break;
    default:
        strcpy(hardware_version_str, "未知");
        break;
    }

    // 清空屏幕并以表格形式显示信息
    system("cls");
    printf("Kobato读卡器 - 设备信息：\n");
    printf("┌──────────────┬────────────┬──────────┬──────────┬─────────────────┐\n");
    printf("│ 高波特率模式 │  LED启用   │ LED亮度  │ 固件版本 │     硬件版本    │\n");
    printf("├──────────────┼────────────┼──────────┼──────────┼─────────────────┤\n");
    printf("│     %s      │    %s     │   %3d    │   v%-3d   │ %-15s │\n",
           high_baudrate_mode ? "是 " : "否 ",
           led_enabled ? "是 " : "否 ",
           led_brightness,
           firmware_version,
           hardware_version_str);
    printf("└──────────────┴────────────┴──────────┴──────────┴─────────────────┘\n\n");
    char choice;
    uint8_t mode_sw = 0;
    while (1)
    {
        // 使用框线包围的重要提示信息
        printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
        printf("┃                              重要提示                                  ┃\n");
        printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
        printf("┃ * V4版本以下不支持读卡测试模式，V7版本以下不支持卡号映射功能           ┃\n");
        printf("┃ * 进入读卡测试模式后只能通过拔线退出，否则读卡器将无法在正常模式工作   ┃\n");
        printf("┃ * 连接此工具后读卡器已被重置为默认sega模式                             ┃\n");
        printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");

        // 操作菜单
        printf("┌─────────────────────────── 操作选项 ──────────────────────────┐\n");
        printf("│                                                               │\n");
        printf("│  【1】 修改SEGA模式下读卡器设置                               │\n");
        printf("│  【2】 进入读卡测试模式                                       │\n");
        printf("│  【3】 进入Namco模式                                          │\n");
        printf("│  【4】 进入Spice模式                                          │\n");
        printf("│  【5】 进入PN532直通模式                                      │\n");
        printf("│  【n】 退出程序                                               │\n");
        printf("│                                                               │\n");
        printf("└───────────────────────────────────────────────────────────────┘\n\n");

        printf("请选择操作 [1-5/n]: ");
        choice = getch();
        printf("%c\n", choice); // 显示用户的选择

        printf("%c\n", choice); // 显示用户的选择

        // 判断用户输入
        if (choice == '1')
        {
            mode_sw = 1;
            break;
        }
        else if (choice == '2')
        {
            mode_sw = 2;
            break;
        }
        else if (choice == '3')
        {
            req_change_mode(NAMCO_MODE);
            printf("\n已切换到Namco模式，按任意键退出...\n");
            getch();
            return -1;
        }
        else if (choice == '4')
        {
            req_change_mode(SPICE_MODE);
            printf("\n已切换到Spice模式，按任意键退出...\n");
            getch();
            return -1;
        }
        else if (choice == '5')
        {
            req_change_mode(RAW_MODE);
            printf("\n已切换到PN532直通模式，按任意键退出...\n");
            getch();
            return -1;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // 输入n或者N，关闭串口，退出程序
            printf("\n串口已关闭\n");
            close_port();
            return 0;
        }
        else
        {
            // 输入其他内容，提示用户"请重新输入！"
            printf("\n输入无效，请按任意键重新选择...\n");
            getch(); // 等待用户按键后继续
        }
    }
    if (mode_sw == 1)
    {
        // 设置正确的波特率
        if (!change_baudrate(high_baudrate_mode ? HIGH_BAUDRATE : LOW_BAUDRATE))
        {
            printf("\n无法设置波特率！请检查连接状态。\n");
            close_port();
            getch();
            return -1;
        }

        system("cls");
        printf("┌─────────────────────── 读卡器设置向导 ────────────────────────┐\n");
        printf("│                                                               │\n");
        printf("│  现在将开始设置读卡器参数，请根据提示进行操作                 │\n");
        printf("│  * 输入 y 表示启用功能，输入 n 表示不启用                     │\n");
        printf("│                                                               │\n");
        printf("└───────────────────────────────────────────────────────────────┘\n\n");

        // ========== 设置高波特率模式 ==========
        printf("┌──── 高波特率模式 ─────────────────────────────────────────────┐\n");
        printf("│ 说明：高波特率模式下通信速率为115200，低波特率为38400         │\n");
        printf("│ 当前状态：%-10s                                          │\n", high_baudrate_mode ? "高波特率" : "低波特率");
        printf("└───────────────────────────────────────────────────────────────┘\n");

        while (1)
        {
            choice = get_user_input("是否启用高波特率模式？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                system_setting_buffer[0] |= 0x02;
                change_highbaudrate_mode = 1;
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                change_highbaudrate_mode = 0;
                break;
            }
            else
            {
                printColoredResult(FALSE, "输入无效，请重新输入！\n");
            }
        }

        // ========== 设置LED指示灯 ==========
        printf("┌──── LED指示灯 ────────────────────────────────────────────────┐\n");
        printf("│ 说明：启用后读卡时LED灯会闪烁提示                             │\n");
        printf("│ 当前状态：%-10s                                          │\n", led_enabled ? "启用" : "禁用");
        printf("└───────────────────────────────────────────────────────────────┘\n");

        while (1)
        {
            choice = get_user_input("是否启用LED指示灯？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                system_setting_buffer[0] |= 0x04;
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                break;
            }
            else
            {
                printColoredResult(FALSE,"输入无效，请重新输入！\n");
            }
        }

        // ========== 设置LED亮度 ==========
        printf("┌──── LED亮度设置 ──────────────────────────────────────────────┐\n");
        printf("│ 说明：亮度范围为0~255，0为关闭，255为最亮                     │\n");
        printf("│ 当前设置：%-3d                                                 │\n", led_brightness);
        printf("└───────────────────────────────────────────────────────────────┘\n");

        int brightness; // LED亮度
        while (1)
        {
            brightness = get_user_input_number("请输入LED亮度值(0-255): ");

            if (brightness >= 0 && brightness <= 255)
            {
                system_setting_buffer[1] = brightness;
                break;
            }
            else
            {
                printColoredResult(FALSE,"输入无效，请输入0-255之间的数值！\n");
            }
        }

        // ========== 设置扩展读卡 ==========
        printf("┌──── 扩展读卡 ────────────────────────────────────────────────┐\n");
        printf("│ 说明：启用后可支持更多类型的卡片                             │\n");
        printf("│ 当前状态：%-10s                                         │\n",
               (res.eeprom_data[0] & 0x10) ? "启用" : "禁用");
        printf("└──────────────────────────────────────────────────────────────┘\n");

        while (1)
        {
            choice = get_user_input("是否启用扩展读卡功能？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                system_setting_buffer[0] |= 0x10;
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                break;
            }
            else
            {
                printColoredResult(FALSE,"输入无效，请重新输入！\n");
            }
        }

        // ========== 设置SPICE模式2P支持 ==========
        printf("┌──── SPICE模式下的2P刷卡 ─────────────────────────────────────┐\n");
        printf("│ 说明：启用后可用于IIDX等需要2P支持的游戏                     │\n");
        printf("│ 当前状态：%-10s                                         │\n",
               (res.eeprom_data[0] & 0x40) ? "启用" : "禁用");
        printf("└──────────────────────────────────────────────────────────────┘\n");

        while (1)
        {
            choice = get_user_input("是否在SPICE模式下启用2P刷卡？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                system_setting_buffer[0] |= 0x40;
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                break;
            }
            else
            {
                printColoredResult(FALSE,"输入无效，请重新输入！\n");
            }
        }

        // ========== 设置卡号映射 ==========
        uint8_t card_reflect = 0;
        printf("┌──── 卡号映射功能 ──────────────────────────────────────────────┐\n");
        printf("│ 说明：可将一张卡的ID映射为另一个卡号，V7以上固件支持此功能     │\n");
        printf("│ 当前状态：%-10s                                           │\n",
               (res.eeprom_data[0] & 0x08) ? "启用" : "禁用");
        printf("└────────────────────────────────────────────────────────────────┘\n");

        while (1)
        {
            choice = get_user_input("是否启用卡号映射功能？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                system_setting_buffer[0] |= 0x08;
                card_reflect = 1;
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                break;
            }
            else
            {
                printColoredResult(FALSE,"输入无效，请重新输入！\n");
            }
        }

        // 如果启用卡号映射，获取映射信息
        char card_IDm[8] = {0};
        char card_accesscode[10] = {0};

        if (card_reflect)
        {
            system("cls");
            printf("┌────────────────── 卡号映射设置 ──────────────────┐\n");
            printf("│                                                  │\n");
            printf("│  需要设置源卡的IDm和目标卡号                     │\n");
            printf("│  IDm可通过本工具的读卡测试模式获取               │\n");
            printf("│                                                  │\n");
            printf("└──────────────────────────────────────────────────┘\n\n");

            printf("请输入被转换卡的IDm(16进制格式，16位，无前缀)\n");
            bool transform_result_IDm = false;
            while (!transform_result_IDm)
            {
                char char_buffer[256];
                printf("IDm: ");
                fgets(char_buffer, 256, stdin);
                if (convert_string_to_hex(char_buffer, card_IDm))
                {
                    transform_result_IDm = true;
                    printColoredResult(TRUE, "IDm格式正确\n\n");
                }
                else
                {
                    printColoredResult(FALSE, "格式错误！请输入16位十六进制字符\n");
                }
            }

            printf("请输入目标转换卡的卡号(10进制格式，20位，无前缀)\n");
            bool transform_result_accode = false;
            while (!transform_result_accode)
            {
                char char_buffer[256];
                uint8_t accode_buffer[20];
                printf("卡号: ");
                fgets(char_buffer, 256, stdin);
                if (convert_string_to_decimal(char_buffer, accode_buffer))
                {
                    transform_result_accode = true;
                    process_array(accode_buffer, card_accesscode);
                    printColoredResult(TRUE, "卡号格式正确\n\n");
                }
                else
                {
                    printColoredResult(FALSE, "格式错误！请输入20位十进制数字\n");
                }
            }
        }

        // 显示设置摘要并确认
        system("cls");
        printf("┌──────────────────── 设置确认 ────────────────────┐\n");
        printf("│                                                  │\n");
        printf("│        当前值                  修改后            │\n");
        printf("├────────────────────┬─────────────────────────────┤\n");

        // 高波特率模式
        printf("│ 高波特率模式:      │ ");
        printColoredStatus(system_setting_buffer[0] & 0x02);
        printf("                      │\n");

        printf("│ ");
        printColoredStatus(res.eeprom_data[0] & 0x02);
        printf("             │                             │\n");

        printf("├────────────────────┼─────────────────────────────┤\n");

        // LED指示灯
        printf("│ LED指示灯:         │ ");
        printColoredStatus(system_setting_buffer[0] & 0x04);
        printf("                      │\n");

        printf("│ ");
        printColoredStatus(res.eeprom_data[0] & 0x04);
        printf("             │                             │\n");

        printf("├────────────────────┼─────────────────────────────┤\n");

        // LED亮度
        printf("│ LED亮度:           │ %-3d                         │\n", system_setting_buffer[1]);
        printf("│ %-3d                │                             │\n", res.eeprom_data[1]);

        printf("├────────────────────┼─────────────────────────────┤\n");

        // 扩展读卡
        printf("│ 扩展读卡:          │ ");
        printColoredStatus(system_setting_buffer[0] & 0x10);
        printf("                      │\n");

        printf("│ ");
        printColoredStatus(res.eeprom_data[0] & 0x10);
        printf("             │                             │\n");

        printf("├────────────────────┼─────────────────────────────┤\n");

        // SPICE 2P模式
        printf("│ SPICE 2P模式:      │ ");
        printColoredStatus(system_setting_buffer[0] & 0x40);
        printf("                      │\n");

        printf("│ ");
        printColoredStatus(res.eeprom_data[0] & 0x40);
        printf("             │                             │\n");

        printf("├────────────────────┼─────────────────────────────┤\n");

        // 卡号映射
        printf("│ 卡号映射:          │ ");
        printColoredStatus(system_setting_buffer[0] & 0x08);
        printf("                      │\n");

        printf("│ ");
        printColoredStatus(res.eeprom_data[0] & 0x08);
        printf("             │                             │\n");

        printf("└────────────────────┴─────────────────────────────┘\n\n");

        // 如果启用了卡号映射，显示映射信息
        if (system_setting_buffer[0] & 0x08)
        {
            printf("┌───────────────── 卡号映射信息 ───────────────────┐\n");
            printf("│ 源卡IDm: ");
            for (int i = 0; i < 8; i++)
            {
                printf("%02X", card_IDm[i]);
            }
            printf("                  │\n");

            printf("│ 目标卡号: ");
            for (int i = 0; i < 10; i++)
            {
                printf("%02X", card_accesscode[i]);
            }
            printf("                   │\n");
            printf("└──────────────────────────────────────────────────┘\n\n");
        }

        printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
        printf("┃  警告：读卡器EEPROM寿命有限，频繁修改可能会损坏设备！        ┃\n");
        printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");

        while (1)
        {
            choice = get_user_input("确认保存这些设置到读卡器？[y/n]: ");
            printf("%c\n", choice);

            if (choice == 'y' || choice == 'Y')
            {
                printf("\n正在保存设置...\n");
                break;
            }
            else if (choice == 'n' || choice == 'N')
            {
                printColoredResult(FALSE, "\n已取消保存，按任意键退出...\n");
                close_port();
                getch();
                return 0;
            }
            else
            {
                printColoredResult(FALSE, "输入无效，请重新输入！\n");
            }
        }

        // 保存设置
        // 设置正确的波特率进行通信
        if (high_baudrate_mode)
        {
            change_baudrate(HIGH_BAUDRATE);
        }
        else
        {
            change_baudrate(LOW_BAUDRATE);
        }

        uint8_t uart_send_buffer[28] = {0xE0, 0x1A, 0x00, 0x00, 0xF7, 0x14, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uart_send_buffer[6] = system_setting_buffer[0];
        uart_send_buffer[7] = system_setting_buffer[1];

        for (uint8_t i = 0; i < 8; i++)
        {
            uart_send_buffer[i + 8] = card_IDm[i];
        }

        for (uint8_t i = 0; i < 10; i++)
        {
            uart_send_buffer[i + 16] = card_accesscode[i];
        }

        // 计算校验和
        for (uint8_t i = 0; i < 26; i++)
        {
            uart_send_buffer[27] += uart_send_buffer[i + 1];
        }

        DWORD bytes_written;
        WriteFile(hPort, uart_send_buffer, 28, &bytes_written, NULL);
        Sleep(3);

        // 根据用户选择更新波特率
        if (change_highbaudrate_mode)
        {
            change_baudrate(HIGH_BAUDRATE);
        }
        else
        {
            change_baudrate(LOW_BAUDRATE);
        }

        printColoredResult(TRUE, "\n√ 设置已成功保存到读卡器！\n");
        printf("\n按任意键退出程序...\n");
        close_port();
        getch();
        return -1;
    }
    else
    {
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
        uint8_t send_buffer_readtest_cmd[8] = {0xE0, 0x06, 00, 00, 0xF8, 0x01, 0x03, 0x02};
        while ((WriteFile(hPort, send_buffer_readtest_cmd, 8, &bytes_written, NULL) == FALSE))
            ;
        while (1)
        {
            while (ReadFile(hPort, buffer, sizeof(buffer), &bytesRead, NULL) == FALSE)
                ;
            system("cls");
            for (DWORD i = 0; i < bytesRead; i++)
            {
                printf("%c", buffer[i]);
            }
        }
    }
}
