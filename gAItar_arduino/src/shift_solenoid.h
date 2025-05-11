#ifndef SHIFT_SOLENOID_H
#define SHIFT_SOLENOID_H

#include "globals.h"

// Function to clear inactive frets, arg is active fret
void clearInactiveFrets(int fretActive);
//Function to depress a solenoid with binary strength, fretIndex (1st fret, fretIndex=1 etc), stringIndex (High E, stringIndex = 1 etc), 
void instantPress (int fretIndex, int stringIndex, int hold_ms);
void shiftLSB(int dataPin, int clkPin, uint8_t pattern);
void clearString(int stringIndex);

#endif // SHIFT_SOLENOID_H