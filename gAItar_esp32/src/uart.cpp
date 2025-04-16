#include "uart.h"

HardwareSerial& instruction_uart = Serial1; // SAMD instruction UART
HardwareSerial& upload_uart = Serial2; // SAMD upload UART


void setupUART() {
    instruction_uart.begin(BAUDRATE, SERIAL_8N1, INSTR_RX, INSTR_TX);
    upload_uart.begin(BAUDRATE, SERIAL_8N1, UPLOAD_RX, UPLOAD_TX);
}

void instructionToSAMD(uint8_t instruction) {
    instruction_uart.write(instruction);
}

void uploadToSAMD(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
      Serial2.write(data[i]);
    }
  }


void handleUARTTraffic() {
    if (SAMDSerial.available()) {
        Serial.write(SAMDSerial.read());
    }
}
