#ifndef TRANSLATE_H
#define TRANSLATE_H
#include <Arduino.h>
#include "globals.h"
#include "servo_toggle.h"
#include "pwm_shift_solenoid.h"
#include "shift_solenoid.h"

void playGuitarEvents();  // Function declaration

unsigned long getCurrentTime();  // Function to get the current time
// If needed elsewhere, also expose time and note state:
extern unsigned long currentTime;
extern unsigned long lastUpdateTime;

struct NoteState {
    bool isActive;
    unsigned long sustainEndTime;
};

extern NoteState noteStates[6];
extern byte lh_state[NUM_FRETS];  // State of each fret's shift register

#endif // TRANSLATE_H