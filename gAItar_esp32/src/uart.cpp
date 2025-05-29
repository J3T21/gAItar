#include "uart.h"
#include "SPIFFS.h"
#include "FS.h"

HardwareSerial& instruction_uart = Serial1; 
HardwareSerial& upload_uart = Serial2; 


void setupUARTs() {
    instruction_uart.begin(BAUDRATE, SERIAL_8N1, INSTR_RX, INSTR_TX);
    upload_uart.begin(BAUDRATE, SERIAL_8N1, UPLOAD_RX, UPLOAD_TX);
}

void instructionToSAMD(const uint8_t* data, size_t length) {
  instruction_uart.write(0xAA); // Start byte
  instruction_uart.write(length);
  instruction_uart.write(data, length);
}


void uploadToSAMD_chunk(bool &sendFile, const String &filePath) {
  if (!sendFile) return;

  File file = SPIFFS.open(filePath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    sendFile = false;
    return;
  }

  size_t fileSize = file.size();
  String header = "START:" + filePath + "SIZE:" + String(fileSize) + "\n";
  upload_uart.print(header);  // Send header

  const size_t chunkSize = 64;
  uint8_t buffer[chunkSize];
  uint16_t chunkId = 0;

  while (file.available()) {
    size_t len = file.read(buffer, chunkSize);

    // Send CHUNK metadata
    upload_uart.printf("CHUNK:%u:SIZE:%u\n", chunkId, len);
    upload_uart.write(buffer, len);

    // Wait for ACK with timeout
    unsigned long startTime = millis();
    bool ackReceived = false;
    while (millis() - startTime < 1000) {
      if (upload_uart.available()) {
        String ack = upload_uart.readStringUntil('\n');
        if (ack == ("ACK:" + String(chunkId))) {
          ackReceived = true;
          break;
        }
      }
    }

    if (!ackReceived) {
      Serial.printf("Timeout or missing ACK for chunk %u\n", chunkId);
      file.close();
      sendFile = false;
      return;
    }

    chunkId++;
  }

  // Notify end of file
  upload_uart.print("DONE\n");

  file.close();
  Serial.println("File sent to Grand Central");

  if (SPIFFS.remove(filePath)) {
    Serial.println("File deleted from ESP32 SPIFFS.");
  } else {
    Serial.println("Failed to delete the file from ESP32 SPIFFS.");
  }

  sendFile = false;
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
    if (SPIFFS.remove(filePath)) {
      Serial.println("File deleted from ESP32 SPIFFS.");
    } else {
      Serial.println("Failed to delete the file from ESP32 SPIFFS.");
    }
    sendFile = false; // Reset flag after successful send
  }
}


enum UploadState{
  IDLE,
  OPEN_FILE,
  SEND_HEADER,
  WAIT_HEADER_ACK,
  SEND_CHUNK,
  WAIT_CHUNK_ACK,
  CLEANUP
};

void uploadToSAMD_state(bool &sendFile, const String &filePath) {
  static UploadState state = IDLE;
  static File file;
  static size_t fileSize = 0;
  static const size_t chunkSize = 64;
  static uint8_t buffer[chunkSize];
  static uint16_t chunkId = 0;
  static unsigned long ackStartTime = 0;
  static size_t lastChunkSize = 0;
  static const int MAX_RETRIES = 10;
  static const int TIMEOUT = 2000; // 1 second timeout for ACK
  static int retryCount = 0;
  static const String tempPath = "/temp";

  switch (state){
    case IDLE:
      if (sendFile){
        state = OPEN_FILE;
      }
      break;
    
    case OPEN_FILE:
      while (upload_uart.available()) {
        upload_uart.read();
      }
      file = SPIFFS.open(tempPath, FILE_READ);
      if (!file){
        Serial.println("Failed to open file: " + tempPath);
        sendFile = false;
        state = IDLE;
        return;
      }
      fileSize = file.size();
      chunkId = 0;
      retryCount = 0;
      state = SEND_HEADER;
      break;

    case SEND_HEADER:{
      String header = "START:" + filePath + ":SIZE:"
      + String(fileSize) + "\n";
      Serial.println("Sending header: " + header);
      upload_uart.print(header); // Send header to Grand Central
      ackStartTime = millis();
      retryCount = 0;
      state = WAIT_HEADER_ACK;
      break;
    }

    case WAIT_HEADER_ACK:
      if (upload_uart.available()){
        String ack = upload_uart.readStringUntil('\n');
        //Serial.println("Received Ack: " + ack);
        ack.trim(); // Remove any trailing whitespace
        if (ack.indexOf("ACK:START:SIZE:") != -1){
          size_t recvdSize = ack.substring(String("ACK:START:SIZE:").length()).toInt();
          if (recvdSize == fileSize){
            state = SEND_CHUNK;
          }else{
            Serial.printf("Header ACK size mismatch: expected %u, got %u\n", fileSize, recvdSize);
            file.close();
            sendFile = false;
            state = IDLE;
          }
        }else{
          Serial.printf("Unexpected header ACK: %s\n", ack.c_str());
        }
      }else if (millis() - ackStartTime > TIMEOUT){
        if (++retryCount <= MAX_RETRIES){
          Serial.println("Header ACK timeout, retrying...");
          String header = "START:" + filePath + ":SIZE:" + String(fileSize) + "\n";
          upload_uart.print(header); // Resend header to Grand Central
          ackStartTime = millis();
        }else{
          Serial.println("Retries exceeded aborting ...");
          file.close();
          sendFile = false;
          state = IDLE;
      }
    } 
    break;

    case SEND_CHUNK:
      while(upload_uart.available()) upload_uart.read(); // Clear any available data
      if (file.available()){
        lastChunkSize = file.read(buffer, chunkSize);
        upload_uart.printf("CHUNK:%u:SIZE:%u\n", chunkId, lastChunkSize);
        // Serial.printf("Sending chunk %u of size %u\n", chunkId, lastChunkSize);
        // Serial.printf("CHUNK:%u:SIZE:%u\n", chunkId, lastChunkSize);
        upload_uart.write(buffer, lastChunkSize);
        ackStartTime = millis();
        retryCount = 0;
        state = WAIT_CHUNK_ACK;
      }else{
        Serial.println("All chunks sent, waiting for final ACK...");
        state = CLEANUP;
      }
      break;

    case WAIT_CHUNK_ACK:
      while(upload_uart.available()){
        String ack = upload_uart.readStringUntil('\n');
        // Serial.println("Received chunk ACK: " + ack);
        ack.trim(); // Remove any trailing whitespace
        if (ack == ("ACK:CHUNK:" + String(chunkId))){
          chunkId++;
          retryCount = 0;
          state = SEND_CHUNK;
        }
      }if (millis() - ackStartTime > TIMEOUT){
        if (++retryCount <= MAX_RETRIES){
          Serial.printf("Chunk ACK timeout for chunk %u, retrying...\n", chunkId);
          upload_uart.printf("CHUNK:%u:SIZE:%u\n", chunkId, lastChunkSize);
          upload_uart.write(buffer, lastChunkSize);
          ackStartTime = millis();
        }else{
        Serial.printf("Retries exceeded for chunk %u, aborting...\n", chunkId);
        file.close();
        sendFile = false;
        state = IDLE;
      }
    } break;

    case CLEANUP:
      file.close();
      Serial.println("File sent to Grand Central");
      if (SPIFFS.remove(tempPath)){
        Serial.println("File deleted from ESP32 SPIFFS.");
      }else{
        Serial.println("Failed to delete the file from ESP32 SPIFFS.");
      }
      sendFile = false; // Reset flag after successful send
      state = IDLE;
      break;
    }
  }
      