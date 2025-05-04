#include "uart.h"
#include "SPIFFS.h"
#include "FS.h"

HardwareSerial& instruction_uart = Serial1; 
HardwareSerial& upload_uart = Serial2; 


void setupUARTs() {
    instruction_uart.begin(BAUDRATE, SERIAL_8N1, INSTR_RX, INSTR_TX);
    upload_uart.begin(BAUDRATE, SERIAL_8N1, UPLOAD_RX, UPLOAD_TX);
}

void instructionToSAMD(uint8_t instruction) {
    instruction_uart.write(instruction);
}

void uploadToSAMD(bool &sendFile, const String &filePath) {
  if (sendFile) {
    File file = SPIFFS.open(filePath, FILE_READ);
    
    if (!file) {
      Serial.println("Failed to open file for reading");
      sendFile = false;
      return;
    }
    size_t fileSize = file.size();

    //header for file transfer
    String header = "START:" + filePath + "SIZE:" + String(fileSize) + "\n";
    upload_uart.print(header); // Send header to Grand Central
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (file.available()) {
      size_t len = file.read(buffer, bufferSize);
      size_t written = 0;
      while (written < len) {
        if (upload_uart.availableForWrite()) {
          written += upload_uart.write(buffer + written, len - written);
        }
      }
    }

    file.close();
    Serial.println("File sent to Grand Central");
    sendFile = false; // Reset flag after successful send
  }
}


