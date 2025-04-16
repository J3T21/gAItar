#include <Arduino.h>
#include <Servo.h>
#include <iostream>
#include <vector>
#include <tuple>
#include "servo_toggle.h"
#include "pwm_shift_solenoid.h"
#include "shift_solenoid.h"
#include "test_funcs.h"
#include "globals.h"
#include "translate.h"


void setup() {
    Serial.begin(115200); // Initialize serial communication for debugging
    Serial1.begin(1000000);
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
}

void loop() {
    if (Serial1.available()) {
        Serial.println("Serial1 available");
        Serial.write(Serial1.read());
      }
    if (Serial4.available()) {
        Serial.println("Serial4 available");
        Serial.write(Serial4.read());
      }
    
      if (Serial.available()) {
        char c = Serial.read(); // Read a character from the serial monitor
        Serial1.write(c);
        Serial4.write(c);
      }

}