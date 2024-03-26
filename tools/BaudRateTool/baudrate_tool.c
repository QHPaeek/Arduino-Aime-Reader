//���ߣ�Qinh
//��AIд��
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <conio.h>

#define HIGH_BAUDRATE 115200 // �߲�����
#define LOW_BAUDRATE 38400 // �Ͳ�����
#define DATA_LENGTH 12 // �ظ����ݵĳ���
#define CHECKSUM_OFFSET 0xE0 // У��λ��ƫ����
#define LED_MIN 2 // LED���ȵ���Сֵ
#define LED_MAX 255 // LED���ȵ����ֵ

HANDLE hPort; // ���ھ��
DCB dcb; // ���ڲ����ṹ��
COMMTIMEOUTS timeouts; // ���ڳ�ʱ�ṹ��
char comPort[10];
uint8_t send_buffer[8] = {0xE0, 0x06, 0x00, 0x00, 0xF6, 0x00, 0x00, 0xFC}; // �������ݻ�����
uint8_t recv_buffer[DATA_LENGTH]; // �������ݻ�����
uint8_t system_setting_buffer[2] = {0}; // ϵͳ���û�����
BOOL high_baudrate_mode = FALSE; // �߲�����ģʽ��־
uint8_t change_highbaudrate_mode = 0;
BOOL led_enabled = FALSE; // LED���ñ�־
uint8_t led_brightness = 0; // LED����
uint8_t firmware_version = 0; // �̼��汾��
uint8_t hardware_version = 0; // Ӳ���汾��

// �򿪴��ڵĺ���������ֵΪBOOL����ʾ�Ƿ�ɹ�
BOOL open_port()
{
    // �򿪴���
    hPort = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPort == INVALID_HANDLE_VALUE)
    {
        printf("�޷��򿪴���%s��\n", comPort);
        return FALSE;
    }

    // ��ȡ���ڲ���
    if (!GetCommState(hPort, &dcb))
    {
        printf("�޷���ȡ����%s�Ĳ�����\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // ���ô��ڲ���
    dcb.BaudRate = HIGH_BAUDRATE; // ���ò�����Ϊ�߲�����
    dcb.ByteSize = 8; // ��������λΪ8
    dcb.Parity = NOPARITY; // ��������żУ��
    dcb.StopBits = ONESTOPBIT; // ����ֹͣλΪ1
    if (!SetCommState(hPort, &dcb))
    {
        printf("�޷����ô���%s�Ĳ�����\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // ��ȡ���ڳ�ʱ
    if (!GetCommTimeouts(hPort, &timeouts))
    {
        printf("�޷���ȡ����%s�ĳ�ʱ��\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // ���ô��ڳ�ʱ
    timeouts.ReadIntervalTimeout = 50; // ���ö�ȡ�����ʱΪ50����
    timeouts.ReadTotalTimeoutConstant = 1000; // ���ö�ȡ�ܳ�ʱ����Ϊ1000����
    timeouts.ReadTotalTimeoutMultiplier = 10; // ���ö�ȡ�ܳ�ʱ����Ϊ10����
    timeouts.WriteTotalTimeoutConstant = 1000; // ����д���ܳ�ʱ����Ϊ1000����
    timeouts.WriteTotalTimeoutMultiplier = 10; // ����д���ܳ�ʱ����Ϊ10����
    if (!SetCommTimeouts(hPort, &timeouts))
    {
        printf("�޷����ô���%s�ĳ�ʱ��\n", comPort);
        CloseHandle(hPort);
        return FALSE;
    }

    // ���سɹ�
    return TRUE;
}

// �رմ��ڵĺ������޷���ֵ
void close_port()
{
    // �رմ���
    CloseHandle(hPort);
}

// �������ݵĺ���������Ϊ���ݳ��ȣ�����ֵΪBOOL����ʾ�Ƿ�ɹ�
BOOL send_data(int length)
{
    DWORD bytes_written; // д����ֽ���
    // д������
    if (!WriteFile(hPort, send_buffer, length, &bytes_written, NULL))
    {
        printf("�޷��򴮿�%sд�����ݣ�\n", comPort);
        return FALSE;
    }
    // ���д����ֽ����Ƿ���ȷ
    if (bytes_written != length)
    {
        printf("�򴮿�%sд�����ݲ�������\n", comPort);
        return FALSE;
    }
    // ���سɹ�
    return TRUE;
}

// �������ݵĺ���������Ϊ���ݳ��ȣ�����ֵΪBOOL����ʾ�Ƿ�ɹ�
BOOL recv_data(int length)
{
    DWORD bytes_read; // ��ȡ���ֽ���
    // ��ȡ����
    if (!ReadFile(hPort, recv_buffer, length, &bytes_read, NULL))
    {
        printf("�޷��Ӵ���%s��ȡ���ݣ�\n", comPort);
        return FALSE;
    }
    // ����ȡ���ֽ����Ƿ���ȷ
    if (bytes_read != length)
    {
        //printf("�Ӵ���%s��ȡ���ݲ�������\n", comPort);
        return FALSE;
    }
    // ���سɹ�
    return TRUE;
}

// �������ݵĺ���������Ϊ���ݳ��ȣ�����ֵΪBOOL����ʾ�Ƿ�ɹ�
BOOL process_data(int length)
{
    int i; // ѭ������
    uint8_t checksum = 0; // У��λ
    // ����һ���ֽ��Ƿ�ΪE0
    if (recv_buffer[0] != 0xE0)
    {
        printf("���������Ӵ����������ӣ�\n");
        return FALSE;
    }
    // ����У��λ
    for (i = 0; i < length - 1; i++)
    {
        checksum += recv_buffer[i];
    }
    checksum -= CHECKSUM_OFFSET;
    // ���У��λ�Ƿ���ȷ
    if (checksum != recv_buffer[length - 1])
    {
        printf("���������Ӵ����������ӣ�\n");
        return FALSE;
    }
    // ��������
    high_baudrate_mode = recv_buffer[7] & 0x02; // ��8���ֽڵĵ�2λ��ʾ�߲�����ģʽ
    led_enabled = recv_buffer[7] & 0x04; // ��8���ֽڵĵ�3λ��ʾLED����
    led_brightness = recv_buffer[8]; // ��9���ֽڱ�ʾLED����
    firmware_version = recv_buffer[9]; // ��10���ֽڱ�ʾ�̼��汾��
    hardware_version = recv_buffer[10]; // ��11���ֽڱ�ʾӲ���汾��
    // ���سɹ�
    return TRUE;
}

// ��ӡ���ݵĺ������޲������޷���ֵ
void print_data()
{
    // ��ӡ����
    printf("������״̬��Ϣ���£�\n");
    printf("�߲�����ģʽ��%s\n", high_baudrate_mode ? "��" : "��");
    printf("LED���ã�%s\n", led_enabled ? "��" : "��");
    printf("LED���ȣ�%d\n", led_brightness);
    printf("�̼��汾��v%d\n", firmware_version);
	switch(hardware_version)
	{
		case 1:
			printf("Ӳ���汾��ATmega32U4\n");
			break;
		case 2:
			printf("Ӳ���汾��SAMD_ZERO\n");
			break;
		case 3:
			printf("Ӳ���汾��ESP8266\n");
			break;
		case 4:
			printf("Ӳ���汾��ESP32\n");
			break;
		case 5:
			printf("Ӳ���汾��AIR001/PY32F002\n");
			break;
		case 6:
			printf("Ӳ���汾��STM32F1\n");
			break;
		case 7:
			printf("Ӳ���汾��STM32F0\n");
			break;
		case 8:
			printf("Ӳ���汾��RP2040\n");
			break;
		case 9:
			printf("Ӳ���汾��ATmega328P\n");
			break;
		default:
			printf("Ӳ���汾��δ֪\n");
			break;
	}	
}

// �޸Ĳ����ʵĺ���������Ϊ�����ʣ�����ֵΪBOOL����ʾ�Ƿ�ɹ�
BOOL change_baudrate(int baudrate)
{
    // ��ȡ���ڲ���
    if (!GetCommState(hPort, &dcb))
    {
        printf("�޷���ȡ����%s�Ĳ�����\n", comPort);
        return FALSE;
    }
    // ���ô��ڲ���
    dcb.BaudRate = baudrate; // ���ò�����
    if (!SetCommState(hPort, &dcb))
    {
        printf("�޷����ô���%s�Ĳ�����\n", comPort);
        return FALSE;
    }
    // ���سɹ�
    return TRUE;
}

// ��ȡ�û�����ĺ���������Ϊ��ʾ��Ϣ������ֵΪchar����ʾ�û�������ַ�
char get_user_input(char *prompt)
{
    char input; // �û�������ַ�
    // ��ӡ��ʾ��Ϣ
    printf("%s", prompt);
    // ��ȡ�û�����
    input = getch();
    // �����û�����
    return input;
}

// ��ȡ�û���������ֵĺ���������Ϊ��ʾ��Ϣ������ֵΪint����ʾ�û����������
int get_user_input_number(char *prompt)
{
    char input[256]; // �û�������ַ���
    int number; // �û����������
    // ��ӡ��ʾ��Ϣ
    printf("%s", prompt);
    // ��ȡ�û�����
    fgets(input, 256, stdin);
    // ���Խ�����ת��Ϊ����
    if (sscanf(input, "%d", &number) != 1)
    {
        // ת��ʧ�ܣ�����-1
        return -1;
    }
    // ת���ɹ�����������
    return number;
}

// ������
int main()
{
printf("�����������޸�Aime�������ڲ����ã�Դ�����https://github.com/QHPaeek/Arduino-Aime-Reader\n");
printf("������EEPROM�������ޣ��벻ҪƵ���޸ģ�\n");
int ports;
while(1)
{
	ports = get_user_input_number("������������Ķ˿ںţ�����com4����������4�����»س�������");
	if (ports < 0){
		printf("��������Ч�����֣�");
		continue;
	}
	if(ports > 9 ){
	snprintf(comPort, sizeof(comPort), "\\\\.\\COM%d", ports);
	break;
	}
	snprintf(comPort, sizeof(comPort), "COM%d", ports);
	break;
}
    // �򿪴���
    if (!open_port())
    {
        // ��ʧ�ܣ��ȴ��û�����������˳�����
        getch();
        return -1;
    }
    // ��COM4�˿���115200���������16��������E0 06 00 00 F6 00 00 FC�����Ҽ����ظ���
    if (!send_data(8))
    {
        // ����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
        close_port();
        getch();
        return -1;
    }
    // ��������
    if (!recv_data(DATA_LENGTH))
    {
        // ����ʧ�ܣ�������38400��������������������ݣ����Ҽ����ظ���
        if (!change_baudrate(LOW_BAUDRATE))
        {
            // �޸Ĳ�����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
            close_port();
            getch();
            return -1;
        }
        if (!send_data(8))
        {
            // ����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
            close_port();
            getch();
            return -1;
        }
        if (!recv_data(DATA_LENGTH))
        {
            // ����ʧ�ܣ���ӡ��䡱���������Ӵ����������ӣ������رմ��ڣ��ȴ��û�����������˳�����
            printf("���������Ӵ����������ӣ�\n");
            close_port();
            getch();
            return -1;
        }
    }
    // ��������
    if (!process_data(DATA_LENGTH))
    {
        // ����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
        close_port();
        getch();
        return -1;
    }
    // ��ӡ����
    print_data();
    char choice; 
uint8_t mode_sw = 0;
    while (1)
    {
        // ��ȡ�û�����
        choice = get_user_input("����1�޸Ķ��������ã�����2�����������ģʽ���̼�V5�汾���²�֧�֣��������ϵ���˳���������n�˳�\n");
        // �ж��û�����
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
        else if (choice == 'n' || choice == 'N')
        {
            // ����n����N���رմ��ڣ��˳�����
            close_port();
            return 0;
        }
        else
        {
            // �����������ݣ���ʾ�û������������룡��
            printf("���������룡\n");
        }
    }
    // ����û�ѡ����������ȸ���ǰ������������С��������Ƿ����ڸ߲�����ģʽ��ѡ����Զ˿ڵķ��Ͳ����ʣ�����Ϊ115200������Ϊ38400��
if (mode_sw == 1){
    if (high_baudrate_mode)
    {
        // �߲�����ģʽ�����ò�����Ϊ115200
        if (!change_baudrate(HIGH_BAUDRATE))
        {
            // �޸Ĳ�����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
            close_port();
            getch();
            return -1;
        }
    }
    else
    {
        // �Ͳ�����ģʽ�����ò�����Ϊ38400
        if (!change_baudrate(LOW_BAUDRATE))
        {
            // �޸Ĳ�����ʧ�ܣ��رմ��ڣ��ȴ��û�����������˳�����
            close_port();
            getch();
            return -1;
        }
    }
    // ����һ��uint8_t system_setting_buffer[2] = {0}����ӡ��䡱�Ƿ����ø߲�����ģʽ������y���ã�����n�����á����û�����y��ִ��system_setting_buffer[0] �ĵڶ�λ��1���û�����������������ʾ�����������룡��
    while (1)
    {
        // ��ȡ�û�����
        choice = get_user_input("�Ƿ����ø߲�����ģʽ������y���ã�����n������\n");
        // �ж��û�����
        if (choice == 'y' || choice == 'Y')
        {
            // ����y����Y������system_setting_buffer[0] �ĵڶ�λ��1������ѭ��
            system_setting_buffer[0] |= 0x02;
		change_highbaudrate_mode = 1;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // ����n����N������ѭ��
		change_highbaudrate_mode = 0;
            break;
        }
        else
        {
            // �����������ݣ���ʾ�û������������룡��
            printf("���������룡\n");
        }
    }
    // ��ӡ��䡱�Ƿ�����LED������y���ã�����n�����á����û�����y��ִ��system_setting_buffer[0] �ĵ���λ��1���û�����������������ʾ�����������룡��
    while (1)
    {
        // ��ȡ�û�����
        choice = get_user_input("�Ƿ�����LED������y���ã�����n������\n");
        // �ж��û�����
        if (choice == 'y' || choice == 'Y')
        {
            // ����y����Y������system_setting_buffer[0] �ĵ���λ��1������ѭ��
            system_setting_buffer[0] |= 0x04;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            // ����n����N������ѭ��
            break;
        }
        else
        {
            // �����������ݣ���ʾ�û������������룡��
            printf("���������룡\n");
        }
    }
    // ��ӡ��䡱������LED���ȷ�Χ����ΧΪ0~255�����س����������ȴ��û��������ֲ����»س�������û�����Ĳ������ֻ��߷�Χ������0~255������ʾ�û�������������!�����������Ҫ�����������ֵ��system_setting_buffer[1]
    int brightness; // LED����
    while (1)
    {
        // ��ȡ�û����������
        brightness = get_user_input_number("������LED���ȷ�Χ����ΧΪ0~255�����س�����\n");
        // �ж��û�����������Ƿ�Ϸ�
        if (brightness >= 0 && brightness <= 255)
        {
            // �Ϸ������������ֵ����system_setting_buffer[1]������ѭ��
            system_setting_buffer[1] = brightness;
            break;
        }
        else
        {
            // ���Ϸ�����ʾ�û�������������!��
            printf("����������!\n");
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
    // ��COM4�˿ڣ��Ե�7���о����Ĳ����ʣ��ȷ���16��������{E0 08 00 00 F7 02}���ٷ���system_setting_buffer[2]���ٷ���16��������{0}��������8+0xf7+2+system_setting_buffer[0]+system_setting_buffer[1]�����ҽ�������ͳ�ȥ��
    send_buffer[0] = 0xE0; // ��һ���ֽ�ΪE0
    send_buffer[1] = 0x08; // �ڶ����ֽ�Ϊ8
    send_buffer[2] = 0x00; // �������ֽ�Ϊ0
    send_buffer[3] = 0x00; // ���ĸ��ֽ�Ϊ0
    send_buffer[4] = 0xF7; // ������ֽ�ΪF7
    send_buffer[5] = 0x02; // �������ֽ�Ϊ2
    send_buffer[6] = system_setting_buffer[0]; // ���߸��ֽ�Ϊsystem_setting_buffer[0]
    send_buffer[7] = system_setting_buffer[1]; // �ڰ˸��ֽ�Ϊsystem_setting_buffer[1]
    send_buffer[8] = 0x00; // �ھŸ��ֽ�Ϊ0
uint16_t checksum_cmd = 0x08 + 0xF7 + 0x02 + system_setting_buffer[0] + system_setting_buffer[1];
    send_buffer[9] = 0; // ��ʮ���ֽ�ΪУ��λ
send_buffer[9] = checksum_cmd & 0b11111111;
DWORD bytes_written;
WriteFile(hPort, send_buffer,10, &bytes_written, NULL);
Sleep(3);
 // �ȴ��������ݷ������
// �ı䴮�ڲ����ʣ������8�����û�ѡ����y����ô�����ڵĲ����ʸı�Ϊ115200������ı�Ϊ38400
    if (change_highbaudrate_mode)
    {
        // �û�ѡ���˸߲�����ģʽ�����ò�����Ϊ115200
        change_baudrate(HIGH_BAUDRATE);
    }
    else
    {
        // �û�û��ѡ��߲�����ģʽ�����ò�����Ϊ38400
	change_baudrate(LOW_BAUDRATE);
    }
    // ����COM4�˿ڣ�����յ��ظ�{E0 06 00 00 F7 00 00 FD}������ʾ�û����޸ĳɹ�����������˳������ȴ��û�����������˳��������û���յ��ظ������߻ظ��ĵ�һ���ֽڲ���E0������ʾ���޸�ʧ�ܣ���������˳������ȴ��û�����������˳�����
    if (!recv_data(8))
    {
        printf("�޸�ʧ�ܣ�δ���յ����ݣ���������˳�\n");
        close_port();
        getch();
        return -1;
    }
    // ���ظ��Ƿ���ȷ
    if (recv_buffer[0] == 0xE0 && recv_buffer[1] == 0x06 && recv_buffer[2] == 0x00 && recv_buffer[3] == 0x00 && recv_buffer[4] == 0xF7 && recv_buffer[5] == 0x00 && recv_buffer[6] == 0x00 && recv_buffer[7] == 0xFD)
    {
        printf("�޸ĳɹ�����������˳�\n");
        close_port();
        getch();
        return 0;
    }
    else
    {
        printf("�޸�ʧ�ܣ��������ݴ��󣬰�������˳�\n");
        close_port();
        getch();
        return -1;
    }
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
uint8_t send_buffer_readtest_cmd[8] = {0xe0,0x06,0x00 ,0x00,0xF8,0x00,0x00,0xFE};
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
