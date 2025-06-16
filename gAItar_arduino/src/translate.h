#ifndef TRANSLATE_H
#define TRANSLATE_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "servo_toggle.h"
#include "pwm_shift_solenoid.h"
#include "shift_solenoid.h"
struct Event {
    unsigned long time;
    int string;
    int fret;
};


void playGuitarEvents();  // Function declaration

void playGuitarEventsOpen();  // Function declaration

void playGuitarFromFile(const char* filePath);

void playGuitarRTOS(const char* filePath);
void playGuitarRTOS_Hammer(const char* filePath);

void playFrets();
void sendPlaybackStatus(Uart &instrUart, JsonArray *eventsPtr = nullptr);
// Add these declarations to translate.h
void sendPlaybackStatusSafe(Uart &instrUart, unsigned long totalTime);
void playGuitarRTOS_safe(const char* filePath); 
void resumePlaybackAtCurrentEvent();
void playGuitarRTOS_Binary(const char* filePath);

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