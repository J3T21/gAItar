#ifndef UART_H
#define UART_H
#include <Arduino.h>

#define INSTR_RX 22
#define INSTR_TX 23
#define UPLOAD_RX 16
#define UPLOAD_TX 17
#define BAUDRATE 115200

extern HardwareSerial& instruction_uart;
extern HardwareSerial& upload_uart;



void setupUARTs();
void instructionToSAMD(const uint8_t* instruction, size_t length);
void uploadToSAMD(bool &sendFile,const String &filePath);
void uploadToSAMD_chunk(bool &sendFile, const String &filePath);
void uploadToSAMD_state(bool &sendFile, const String &filePath);
#endif