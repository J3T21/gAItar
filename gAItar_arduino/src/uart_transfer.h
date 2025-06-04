#ifndef UART_TRANSFER_H
#define UART_TRANSFER_H
#include <Arduino.h>

void listFilesOnSD(); // Function to list files on SD card
void listFilesOnSDUart(Uart& uart);
void instructionReceiverRTOS(Uart &instrUart); // Function to receive instructions in JSON format over UART
const char* findFileSimple(const char* title, const char* artist, const char* genre);
bool createDirectoriesRTOS_static(const char* fullPath);
void fileReceiverRTOS_char(Uart &fileUart);

#endif

