#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h> // contains byte
#include "servo_toggle.h"

#define NUM_FRETS 10 //number of implemented frets

#define string1 0b00000001 // Initialize string1 LSB first
#define string2 0b00000010 // Initialize string2 LSB first
#define string3 0b00100000 // Initialize string3 LSB first
#define string4 0b00010000 // Initialize string4 LSB first
#define string5 0b10000000 // Initialize string5 LSB first
#define string6 0b01000000 // Initialize string6 LSB first
 
#define clearPin1 24
#define clkPin1 22
#define dataPin1 26

#define clearPin2 30
#define clkPin2 28
#define dataPin2 32

#define clearPin3 36
#define clkPin3 34
#define dataPin3 38

#define clearPin4 42
#define clkPin4 40
#define dataPin4 44

#define clearPin5 48
#define clkPin5 50
#define dataPin5 46

#define clearPin6 25
#define clkPin6 23
#define dataPin6 27

#define clearPin7 31
#define clkPin7 29
#define dataPin7 33

#define clearPin8 37
#define clkPin8 35
#define dataPin8 39

#define clearPin9 43
#define clkPin9 41
#define dataPin9 45

#define clearPin10 49
#define clkPin10 47
#define dataPin10 51

extern const int fretPins[NUM_FRETS][3];  // clk, data, clear
extern const byte stringOrder[6];
extern const int events[][3]; // {delta_ms, string, fret}
extern const size_t eventCount;
extern byte lh_state[NUM_FRETS]; // Left hand state for each fret
extern PwmServoController servo1;
extern PwmServoController servo2;
extern PwmServoController servo3;
extern PwmServoController servo4;
extern PwmServoController servo5;
extern PwmServoController servo6;

#endif // GLOBALS_H