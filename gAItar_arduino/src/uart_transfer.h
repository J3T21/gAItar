#ifndef UART_TRANSFER_H
#define UART_TRANSFER_H
#include <Arduino.h>

void fileReceiver(Uart &fileUart); // Function to receive files over UART

void instructionReceiver(Uart &instrUart); // Function to receive instructions over UART
void fileReceiver_chunk(Uart &fileUart); // Function to receive files in chunks over UART
void fileReceiver_state(Uart &fileUart); // Function to receive files with state management over Uart
bool createDirectory(String fullPath); // Function to create directories on SD card
void listFilesOnSD(); // Function to list files on SD card
const char* findFile(const String& title, const String& artist, const String& genre); // Function to find a file on SD card
void instructionReceiverJson(Uart &instrUart); // Function to receive instructions in JSON format over UART
#endif

