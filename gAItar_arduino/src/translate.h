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

size_t calculateMaxFileSize();
void playGuitarEvents();  // Function declaration

void playGuitarEventsOpen();  // Function declaration

void playGuitarFromFile(const char* filePath);

void playGuitarRTOS(const char* filePath);
void playGuitarRTOS_Hammer(const char* filePath);

void playFrets();
void sendPlaybackStatus(Uart &instrUart, JsonArray *eventsPtr = nullptr);
// Add these declarations to translate.h
void playGuitarRTOS_Stream(const char* filePath);
bool seekToEventsArray(File& file);
void loadNextEventBatch(File& file, struct Event* eventBuffer, size_t& bufferSize);
bool parseEventLine(const String& line, struct Event& event);
unsigned long calculateNoteDuration(struct Event* eventBuffer, size_t currentIndex, size_t bufferSize, int string, unsigned long currentTime);
void processGuitarEvent(int string, int fret, bool moveServo);
void sendPlaybackStatusStreaming(Uart &instrUart);

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