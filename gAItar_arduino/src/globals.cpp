#include "globals.h"

Uart &dataUart = Serial1; // Define the UART interface for data transfer
Uart &instructionUart = Serial4; // Define the UART interface for instructions
SdFat sd;
ServoController servo6(2, 78, 98);
ServoController servo5(3, 75, 94);
ServoController servo4(4, 82, 102);
ServoController servo3(5, 81, 101);
ServoController servo2(6, 83, 103);
ServoController servo1(7, 79, 99);

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
    {clkPin10, dataPin10, clearPin10}
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