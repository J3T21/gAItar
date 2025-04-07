#ifndef PWM_SHIFT_SOLENOID_H
#define PWM_SHIFT_SOLENOID_H

#include "globals.h"
#include "shift_solenoid.h"

// Declare the functions
void pwmRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime);
void pwmLogRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime);
void pwmSineRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime);

#endif // PWM_SHIFT_SOLENOID_H
