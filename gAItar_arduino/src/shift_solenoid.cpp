#include "shift_solenoid.h"

void shiftLSB(int dataPin, int clkPin, uint8_t pattern) {
    // Clear shift register first
    for (int i = 0; i < 8; i++) {
        digitalWrite(clkPin, LOW);
        digitalWrite(dataPin, LOW);
        digitalWrite(clkPin, HIGH);
    }

    // Send pattern LSB-first
    for (int i = 0; i < 8; i++) {
        digitalWrite(clkPin, LOW);
        digitalWrite(dataPin, (pattern >> i) & 1);
        digitalWrite(clkPin, HIGH);
    }
}


void clearInactiveFrets(int fretActive) {
    for (int i = 0; i < NUM_FRETS; i++) {
        if (i != fretActive) {
            digitalWrite(fretPins[i][2], HIGH); // Set clear pin high to enable the shift register
            shiftOut(fretPins[i][1], fretPins[i][0], LSBFIRST, 0); // Clear inactive frets
            digitalWrite(fretPins[i][2], LOW); // Set clear pin low to disable the shift register
        }
    }
}

void instantPress(int fretIndex, int stringIndex, int hold_ms) {
    int clk = fretPins[fretIndex - 1][0];  
    int data = fretPins[fretIndex - 1][1];
    
    byte pattern = stringOrder[stringIndex - 1];
    shiftLSB(data, clk, pattern);  // Turn solenoid ON

    delay(hold_ms);                // Hold the solenoid

    shiftLSB(data, clk, 0);        // Turn solenoid OFF
}

void clearAllFrets() {
    for (int i = 0; i < NUM_FRETS; i++) {
        fretStates[i] = 0; // Clear the software state
        digitalWrite(fretPins[i][2], HIGH); // Enable the shift register
        shiftOut(fretPins[i][1], fretPins[i][0], LSBFIRST, 0); // Clear all bits (release all solenoids)
        digitalWrite(fretPins[i][2], LOW); // Disable the shift register
    }
}