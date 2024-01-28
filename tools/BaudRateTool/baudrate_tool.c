#include <stdio.h>  
#include <windows.h>  
#include <conio.h>  // Ϊ��ʹ��getch()����  
  
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
        printf("�밴1ѡ��115200�����ʣ���2ѡ��38400�����ʣ�\n");  
        _getch();  // �ȴ��û�������������ʾ������̨  
        baudRateChoice = _getch() - '0';  // ��ȡ�û���������ֲ�ת��Ϊ����  
    } while (baudRateChoice != 1 && baudRateChoice != 2);  
  
    if (baudRateChoice == 1) {  
        BAUD_RATE = BAUD_RATE_HIGH;  // ���ò�����Ϊ115200  
    } else {  
        BAUD_RATE = BAUD_RATE_LOW;  // ���ò�����Ϊ38400  
    }
    // �򿪴��ж˿�  
    hSerial = CreateFile(SERIAL_PORT_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  
    if (hSerial == INVALID_HANDLE_VALUE) {  
        printf("ɨ�費������������ȷ�϶����������Ӳ��ҷ��䵽�˿�COM4\n");  
	printf("��������˳�......");
        getch();  // �ȴ��û������˳�  
        return 1;  
    }  
  
    // ���ô��ж˿ڲ���  
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);  
    if (!GetCommState(hSerial, &dcbSerialParams)) {  
        printf("�޷���ȡ���ж˿ڵ�ǰ״̬\n");  
	printf("��������˳�......");
        getch();  // �ȴ��û������˳�  
        CloseHandle(hSerial);  
        return 1;  
    }  
    dcbSerialParams.BaudRate = BAUD_RATE;  
    dcbSerialParams.ByteSize = 8;  
    dcbSerialParams.StopBits = ONESTOPBIT;  
    dcbSerialParams.Parity   = NOPARITY;  
    if (!SetCommState(hSerial, &dcbSerialParams)) {  
        printf("�޷����ô��ж˿�״̬\n");  
	printf("��������˳�......");
        getch();  // �ȴ��û������˳�  
        CloseHandle(hSerial);  
        return 1;  
    }  
  
    // ���ô��ж˿ڳ�ʱ����  
    timeouts.ReadIntervalTimeout = MAXDWORD;  
    timeouts.ReadTotalTimeoutConstant = 0;  
    timeouts.ReadTotalTimeoutMultiplier = 0;  
    timeouts.WriteTotalTimeoutConstant = 0;  
    timeouts.WriteTotalTimeoutMultiplier = TIMEOUT;  
    if (!SetCommTimeouts(hSerial, &timeouts)) {  
        printf("�޷����ô��ж˿ڳ�ʱ����\n");  
	printf("��������˳�......");
        getch();  // �ȴ��û������˳�  
        CloseHandle(hSerial);  
        return 1;  
    }  
  
    // ���ж˿�д�����ݲ���鷵��ֵ  
    success = WriteFile(hSerial, writeBuffer, WRITE_BUFFER_SIZE, &bytesRead, NULL);  
    if (success == 0) {  
        printf("����ָ��ʧ��\n");  
	printf("��������˳�......");
        getch();  // �ȴ��û������˳�  
        CloseHandle(hSerial);  
        return 1;  
    } else {  
        printf("����ָ��ɹ�\n");  
    }  
  
    // �Ӵ��ж˿ڶ�ȡ���ݲ���鷵��ֵ����������  
    success = ReadFile(hSerial, readBuffer, READ_BUFFER_SIZE, &bytesRead, NULL);  
    if (success == 0) {  
        printf("��ȡ����ʧ��\n");  
        getch();  // �ȴ��û������˳�  
        CloseHandle(hSerial);  
        return 1;  
    } else {  
        printf("��ȡ���ݳɹ�\n");  
        if (bytesRead == 2 && readBuffer[0] == 0xff && readBuffer[1] == 0x99 && readBuffer[2] == 0xaa) {  
            printf("���ص�������0xff99aa�������ɹ�\n");  
        } else {  
            printf("���ص����ݲ���0xff99aa������ʧ��\n");  
        }  
	getch();  // �ȴ��û������˳���Ȼ��رմ��ж˿�
}}