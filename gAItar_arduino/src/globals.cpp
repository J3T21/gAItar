#include "globals.h"

Uart &dataUart = Serial1; // Define the UART interface for data transfer
Uart &instructionUart = Serial4; // Define the UART interface for instructions
SdFat sd;
ServoController servo1(2, 86, 106);
ServoController servo2(3, 78, 96);
ServoController servo3(4, 79, 100);
ServoController servo4(5, 80, 96);
ServoController servo5(6, 85, 107);
ServoController servo6(7, 74, 97);

byte fretStates[NUM_FRETS] = {0}; // array to hold the state of the left hand

// Define your pin constants somewhere above this or use actual pin numbers
const int fretPins[NUM_FRETS][3] = {
    {clkPin1, dataPin1, clearPin1},
    {clkPin2, dataPin2, clearPin2},
    {clkPin3, dataPin3, clearPin3},
    {clkPin4, dataPin4, clearPin4},
    {clkPin5, dataPin5, clearPin5},
    {clkPin6, dataPin6, clearPin6},
    {clkPin7, dataPin7, clearPin7},
    {clkPin8, dataPin8, clearPin8},
    {clkPin9, dataPin9, clearPin9},
    {clkPin10, dataPin10, clearPin10},
    {clkPin11, dataPin11, clearPin11},
    {clkPin12, dataPin12, clearPin12}
};

const byte stringOrder[6] = {
    string1, // High E
    string2, // B
    string3, // G
    string4, // D
    string5, // A
    string6  // Low E
};

SoftStartState softStartStates[NUM_FRETS*6];