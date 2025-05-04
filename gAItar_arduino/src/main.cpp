#include <Arduino.h>
#include <Servo.h>
#include <iostream>
#include <vector>
#include <tuple>
#include <SPI.h>
#include <SD.h>
#include "servo_toggle.h"
#include "pwm_shift_solenoid.h"
#include "shift_solenoid.h"
#include "test_funcs.h"
#include "globals.h"
#include "translate.h"
#include "uart_transfer.h"

void setup() {
    Serial.begin(115200); // Initialize serial communication or debugging
    Serial1.begin(1000000); // Initialize Serial1 for UART communication
    Serial4.begin(1000000);
    delay(1000); // Wait for serial to initialize
    for (int i = 0; i < NUM_FRETS; i++)
    {
        pinMode(fretPins[i][0], OUTPUT); // Set clock pin as output
        pinMode(fretPins[i][1], OUTPUT); // Set data pin as output
        pinMode(fretPins[i][2], OUTPUT); // Set clear pin as output
        digitalWrite(fretPins[i][0], LOW); // Set clock pin LOW to start
        digitalWrite(fretPins[i][2], LOW); // Set clear pin LOW to clear the shift register initially
    }
    if (!SD.begin()){
        Serial.println("SD card initialization failed!"); // SD card initialization failed
        return;
    } else {
        Serial.println("SD card initialized successfully!"); // SD card initialized successfully
    }
    listFilesOnSD();
}

void loop() {
  fileReceiver(); // Call the file receiver function to handle incoming data
}