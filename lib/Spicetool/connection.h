#ifndef SPICEAPI_CONNECTION_H
#define SPICEAPI_CONNECTION_H

#include <Arduino.h>
#include <stdint.h>

#ifndef SPICEAPI_INTERFACE
#define SPICEAPI_INTERFACE Serial  // 默认使用Serial
#endif

#define BUFFER_SIZE 64

void reset() {
  // drop all input
  while (SPICEAPI_INTERFACE.available()) {
    SPICEAPI_INTERFACE.read();
  }
}

const char* spice_request(const char* json, size_t timeout,uint8_t* receive_buffer) {
    // 清空缓冲区
    memset(receive_buffer, 0, BUFFER_SIZE);

    // 发送请求（不带终止符）
    size_t sent = SPICEAPI_INTERFACE.write((uint8_t*)json, strlen(json));
    SPICEAPI_INTERFACE.write((uint8_t)0);
    SPICEAPI_INTERFACE.flush();
    if (sent != strlen(json)) return "";

    // 接收数据
    size_t received = 0;
    uint32_t t_start = millis();
    
    while (millis() - t_start < timeout) {
        while (SPICEAPI_INTERFACE.available()) {
            int b = SPICEAPI_INTERFACE.read();
            if (b == -1) break;

            // 处理缓冲区溢出
            if (received >= BUFFER_SIZE - 1) {
                receive_buffer[BUFFER_SIZE - 1] = '\0';
                reset();
                return (const char*)receive_buffer;
            }

            receive_buffer[received++] = b;

            // 检测终止符（根据协议调整）
            if (b == '\n' || b == '\0') {
                receive_buffer[received] = '\0';
                return (const char*)receive_buffer;
            }
        }
        
        // 短暂延时防止忙等待
        delay(1);
    }

    // 添加终止符后返回
    receive_buffer[received] = '\0';
    return (const char*)receive_buffer;
}

#endif  // SPICEAPI_CONNECTION_H