#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define TCP_PORT 1337
#define COM_PORT "\\\\.\\COM100"
#define BUFFER_SIZE 256

int main() {
    // 打开COM端口
    HANDLE hCom = CreateFile(COM_PORT, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE) {
        printf("COM端口打开失败\n按任意键退出...");
        _getch();
        return 1;
    }

    // 配置串口参数
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hCom, &dcb)) {
        CloseHandle(hCom);
        printf("获取COM状态失败\n按任意键退出...");
        _getch();
        return 1;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    
    if (!SetCommState(hCom, &dcb)) {
        CloseHandle(hCom);
        printf("设置COM参数失败\n按任意键退出...");
        _getch();
        return 1;
    }

    // 设置串口超时
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    SetCommTimeouts(hCom, &timeouts);

    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        CloseHandle(hCom);
        printf("WSAStartup失败\n按任意键退出...");
        _getch();
        return 1;
    }

    // 创建TCP套接字
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        CloseHandle(hCom);
        WSACleanup();
        printf("创建套接字失败\n按任意键退出...");
        _getch();
        return 1;
    }

    // 设置TCP连接参数
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 使用 inet_addr 替代 inet_pton

    // 连接TCP服务器
    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        CloseHandle(hCom);
        WSACleanup();
        printf("连接TCP服务器失败\n按任意键退出...");
        _getch();
        return 1;
    }

    printf("COM端口和TCP连接已建立，开始数据转发...\n");

    // 数据转发循环
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;
    int bytesSent;

    while (1) {
        if (ReadFile(hCom, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                bytesSent = send(sock, buffer, bytesRead, 0);
                if (bytesSent == SOCKET_ERROR) {
                    printf("TCP发送错误\n");
                    break;
                }
            }
        } else {
            printf("COM端口读取错误\n");
            break;
        }
    }

    // 清理资源
    closesocket(sock);
    CloseHandle(hCom);
    WSACleanup();
    
    printf("连接已断开\n按任意键退出...");
    _getch();
    return 0;
}