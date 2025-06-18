#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <Arduino.h>
#include "globals.h"
#include "servo_toggle.h"
#include "shift_solenoid.h"

/**
 * Binary guitar playback system function declarations
 * Handles real-time binary file parsing and hardware control for automated guitar playing
 */

/**
 * Sends playback status information over UART interface
 * Used for synchronizing external control systems with current playback state
 * 
 * @param instrUart UART interface for status transmission
 * @param totalTime Total song duration in milliseconds
 */
void sendPlaybackStatusSafe(Uart &instrUart, unsigned long totalTime);

/**
 * Main binary guitar playback engine
 * Streams binary song data and controls servo motors and solenoid fret actuators
 * Designed for real-time operation with minimal memory allocation
 * 
 * @param filePath Path to binary song file on SD card storage
 */
void playGuitarRTOS_Binary(const char* filePath);

/**
 * Low-level hardware control function for individual guitar events
 * Processes single note events and translates to servo and solenoid actions
 * 
 * @param string Guitar string number (1-6, where 1 = High E, 6 = Low E)
 * @param fret Fret number (0 = open string, -1 = string off, 1-12 = fretted notes)
 * @param moveServo Flag indicating whether servo actuation is required
 */
void processGuitarEvent(int string, int fret, bool moveServo);

#endif // TRANSLATE_H