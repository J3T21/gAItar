#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h> // contains byte
#include <SdFat.h>
#include "servo_toggle.h"

#define BAUDRATE 115200
#define NUM_FRETS 12 //number of implemented frets

#define string1 0b00000001 // Initialize string1 LSB first
#define string2 0b00000010 // Initialize string2 LSB first
#define string3 0b00100000 // Initialize string3 LSB first
#define string4 0b00010000 // Initialize string4 LSB first
#define string5 0b10000000 // Initialize string5 LSB first
#define string6 0b01000000 // Initialize string6 LSB first
 
#define clearPin1 25
#define clkPin1 23
#define dataPin1 27

#define clearPin2 31
#define clkPin2 29
#define dataPin2 33

#define clearPin3 37
#define clkPin3 35
#define dataPin3 39

#define clearPin4 43
#define clkPin4 41
#define dataPin4 45

#define clearPin5 49
#define clkPin5 47
#define dataPin5 51

#define clearPin6 24
#define clkPin6 22
#define dataPin6 26

#define clearPin7 30
#define clkPin7 28
#define dataPin7 32

#define clearPin8 36
#define clkPin8 34
#define dataPin8 38

#define clearPin9 42
#define clkPin9 40
#define dataPin9 44

#define clearPin10 48
#define clkPin10 46
#define dataPin10 50

#define clearPin11 A14
#define clkPin11 A15
#define dataPin11 A13

#define clearPin12 A11
#define clkPin12 A12
#define dataPin12 A10

extern Uart &dataUart; // Define the UART interface for data transfer
extern Uart &instructionUart; // Define the UART interface for instructions
extern SdFat sd; // SD card object
extern const int fretPins[NUM_FRETS][3];  // clk, data, clear
extern const byte stringOrder[6];
//extern const size_t eventCount;
extern byte lh_state[NUM_FRETS]; // Left hand state for each fret
extern ServoController servo1;
extern ServoController servo2;
extern ServoController servo3;
extern ServoController servo4;
extern ServoController servo5;
extern ServoController servo6;
extern byte fretStates[NUM_FRETS]; // State of each fret's shift register
struct SoftStartState {
    unsigned long startTime = 0;
    unsigned long lastToggleTime = 0;
    bool pwmOn = false;
    bool ramping = false;
};

// Declare the array for all frets (or more if needed)
extern SoftStartState softStartStates[NUM_FRETS*6];

#endif // GLOBALS_H