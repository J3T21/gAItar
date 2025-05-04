#ifndef UART_H
#define UART_H
#include <Arduino.h>

#define INSTR_RX 22
#define INSTR_TX 23
#define UPLOAD_RX 16
#define UPLOAD_TX 17
#define BAUDRATE 1000000

extern HardwareSerial& instruction_uart;
extern HardwareSerial& upload_uart;


void setupUARTs();
void instructionToSAMD(uint8_t instruction);
void uploadToSAMD(const uint8_t *data, size_t length);

#endif