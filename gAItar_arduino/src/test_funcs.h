#ifndef TEST_FUNCS_H
#define TEST_FUNCS_H

#include "globals.h"
#include "shift_solenoid.h"
#include "pwm_shift_solenoid.h"

void testFret(int start_fret, int timeHold, int stop_Fret);
void testCombined(int start_fret, int timeHold);
void testSerialControlservo(); // Function to test serial control of servos
void testSerialControlsolenoid(); // Function to test serial control of solenoids
void testFretPWM(int fret, int timehold, char pwmtype, int rampDelay);
#endif // TEST_FUNCS_H
