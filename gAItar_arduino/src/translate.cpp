#include "translate.h"
#include <ArduinoJson.h>
#include <SdFat.h>
#include <FreeRTOS_SAMD51.h>
#include "uart_transfer.h"

extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern char currentSongPath[128];
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;
static JsonArray events;

void playGuitarRTOS(const char* filePath) {
    static File file;
    static JsonDocument doc;
    static bool fileLoaded = false;

    if (!isPlaying || isPaused) {
        // If not playing or paused, cleanup and return
        
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            if (fileLoaded && file) {
            file.close();
            fileLoaded = false;
            }
            xSemaphoreGive(sdSemaphore);
        }
        clearAllFrets();
        return;
    }

    // Only load the file and parse JSON once per song
    if (!fileLoaded) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open file for reading");
                isPlaying = false;
                return;
            }
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            xSemaphoreGive(sdSemaphore);
            if (error) {
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                isPlaying = false;
                fileLoaded = false;
                return;
            }
            events = doc["events"];
            fileLoaded = true;
            if (newSongRequested){
                currentEventIndex = 0;
                startTime = millis();
                pauseOffset = 0;
                newSongRequested = false; 
            }
        }else {
            Serial.println("Failed to take SD semaphore");
            return;
        }
    }

    // Play events incrementally
    if (currentEventIndex < events.size()) {
        JsonObject event = events[currentEventIndex];
        unsigned long eventTime = event["time"];
        int string = event["string"];
        int fret = event["fret"];

        if (millis() - startTime >= eventTime) {
            // --- Handle the event (same as your playGuitarFromFile logic) ---
            if (fret == -1) {
                switch (string) {
                    case 1: servo1.damper(); break;
                    case 2: servo2.damper(); break;
                    case 3: servo3.damper(); break;
                    case 4: servo4.damper(); break;
                    case 5: servo5.damper(); break;
                    case 6: servo6.damper(); break;
                    default: Serial.println("Invalid string number!"); break;
                }
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                    }
                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else if (fret == 0) {
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default: Serial.println("Invalid string number!"); break;
                }
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                    }
                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else {
                if (fret < 1 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    isPlaying = false;
                    fileLoaded = false;
                    return;
                }
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default: Serial.println("Invalid string number!"); break;
                }
                int fretIndex = fret - 1;
                bool alreadyHeld = false;
                switch (string) {
                    case 1: alreadyHeld = (fretStates[fretIndex] & string1); break;
                    case 2: alreadyHeld = (fretStates[fretIndex] & string2); break;
                    case 3: alreadyHeld = (fretStates[fretIndex] & string3); break;
                    case 4: alreadyHeld = (fretStates[fretIndex] & string4); break;
                    case 5: alreadyHeld = (fretStates[fretIndex] & string5); break;
                    case 6: alreadyHeld = (fretStates[fretIndex] & string6); break;
                }
                if (!alreadyHeld){
                    switch (string) {
                        case 1: fretStates[fretIndex] |= string1; break;
                        case 2: fretStates[fretIndex] |= string2; break;
                        case 3: fretStates[fretIndex] |= string3; break;
                        case 4: fretStates[fretIndex] |= string4; break;
                        case 5: fretStates[fretIndex] |= string5; break;
                        case 6: fretStates[fretIndex] |= string6; break;
                    }
                }
                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }
            currentEventIndex++;
        }
    } else {
        // Song finished
        currentSongPath[0] = '\0';
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        Serial.println("Playback finished.");
    }
}


void playGuitarRTOS_Hammer(const char* filePath) {
    static File file;
    static JsonDocument doc;
    static bool fileLoaded = false;
    static const unsigned long SERVO_THRESH = 100;
    size_t MAX_FILE_SIZE = 150000;


    if (!isPlaying || isPaused) {
        if (fileLoaded && !doc.isNull() && !events.isNull()) {
            sendPlaybackStatus(instructionUart, &events);
        } else {
            sendPlaybackStatus(instructionUart, nullptr);  // Safe fallback
        }
        // If not playing or paused, cleanup and return
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            if (fileLoaded && file) {
                doc.clear();
                file.close();
                fileLoaded = false;
                }
            xSemaphoreGive(sdSemaphore);
        }
        clearAllFrets();
        return;
    }

    // Only load the file and parse JSON once per song
    if (!fileLoaded || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            doc.clear();
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open file for reading");
                isPlaying = false;
                return;
            }
            size_t fileSize = file.size();
            if (fileSize > MAX_FILE_SIZE) {
                Serial.println("ERROR: File too large ");
                
                file.close();
                xSemaphoreGive(sdSemaphore);
                
                isPlaying = false;
                fileLoaded = false;
                currentSongPath[0] = '\0';
                
                // Send detailed error message
                instructionUart.println("ERROR:File too large");
                return;
            }

            DeserializationError error = deserializeJson(doc, file);
            file.close();
            xSemaphoreGive(sdSemaphore);
            if (error) {
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                doc.clear();
                isPlaying = false;
                fileLoaded = false;
                return;
            }
            events = doc["events"];
            fileLoaded = true;
            if (newSongRequested){
                currentEventIndex = 0;
                startTime = millis();
                pauseOffset = 0;
                newSongRequested = false; 
            }
        }else {
            Serial.println("Failed to take SD semaphore");
            return;
        }
    }

    if (fileLoaded && !doc.isNull() && !events.isNull()) {
        sendPlaybackStatus(instructionUart, &events);
    }

    // Play events incrementally
    if (currentEventIndex < events.size()) {
        JsonObject event = events[currentEventIndex];
        unsigned long eventTime = event["time"];
        int string = event["string"];
        int fret = event["fret"];

        if (millis() - startTime >= eventTime) {
            // --- Handle the event (same as your playGuitarFromFile logic) ---
            unsigned long noteDuration = 0;
            bool moveServo = false;
            if (fret > 0) { // Only for fretted notes
                // Look for the next event on the same string
                for (size_t i = currentEventIndex + 1; i < events.size(); i++) {
                    JsonObject nextEvent = events[i];
                    int nextString = nextEvent["string"];
                    unsigned long nextEventTime = nextEvent["time"];
                    
                    if (nextString == string) {
                        // Found next event on same string
                        noteDuration = nextEventTime - eventTime;
                        moveServo = (noteDuration >= SERVO_THRESH);
                        break;
                    }
                }
            }
            if (fret == -1) {
                // switch (string) {
                //     case 1: servo1.damper(); break;
                //     case 2: servo2.damper(); break;
                //     case 3: servo3.damper(); break;
                //     case 4: servo4.damper(); break;
                //     case 5: servo5.damper(); break;
                //     case 6: servo6.damper(); break;
                //     default: Serial.println("Invalid string number!"); break;
                // }
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                    }
                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else if (fret == 0) {
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default: Serial.println("Invalid string number!"); break;
                }
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                    }
                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else {
                if (fret < 1 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    isPlaying = false;
                    fileLoaded = false;
                    return;
                }
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default: Serial.println("Invalid string number!"); break;
                }
                int fretIndex = fret - 1;
                bool alreadyHeld = false;
                switch (string) {
                    case 1: alreadyHeld = (fretStates[fretIndex] & string1); break;
                    case 2: alreadyHeld = (fretStates[fretIndex] & string2); break;
                    case 3: alreadyHeld = (fretStates[fretIndex] & string3); break;
                    case 4: alreadyHeld = (fretStates[fretIndex] & string4); break;
                    case 5: alreadyHeld = (fretStates[fretIndex] & string5); break;
                    case 6: alreadyHeld = (fretStates[fretIndex] & string6); break;
                }
                if (!alreadyHeld){
                    switch (string) {
                        case 1: fretStates[fretIndex] |= string1; break;
                        case 2: fretStates[fretIndex] |= string2; break;
                        case 3: fretStates[fretIndex] |= string3; break;
                        case 4: fretStates[fretIndex] |= string4; break;
                        case 5: fretStates[fretIndex] |= string5; break;
                        case 6: fretStates[fretIndex] |= string6; break;
                    }
                }
                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }
            currentEventIndex++;
        }
    } else {
        // Song finished
        if (fileLoaded && !doc.isNull()) {
            sendPlaybackStatus(instructionUart, &events);
        }
        doc.clear();
        currentSongPath[0] = '\0';
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        Serial.println("Playback finished.");
    }
}


// Add these helper functions to translate.cpp

// Add to translate.cpp
void resumePlaybackAtCurrentEvent() {
    extern size_t currentEventIndex;
    extern unsigned long startTime;
    
    // Access the static doc from playGuitarRTOS_safe
    // We need to make doc accessible or pass the event time differently
    
    // Simple approach: calculate startTime based on currentEventIndex
    // This assumes the next event should play "now"
    startTime = millis();
    Serial.printf("Resuming at event %zu\n", currentEventIndex);
}


void processGuitarEvent(int string, int fret, bool moveServo) {
    if (fret == -1) {
        // String off - clear all frets for this string
        for (int f = 0; f < NUM_FRETS; f++) {
            switch (string) {
                case 1: fretStates[f] &= ~string1; break;
                case 2: fretStates[f] &= ~string2; break;
                case 3: fretStates[f] &= ~string3; break;
                case 4: fretStates[f] &= ~string4; break;
                case 5: fretStates[f] &= ~string5; break;
                case 6: fretStates[f] &= ~string6; break;
            }
            
            int clkPin = fretPins[f][0];
            int dataPin = fretPins[f][1];
            int clearPin = fretPins[f][2];
            digitalWrite(clearPin, HIGH);
            shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
        }
    } else if (fret == 0) {
        // Open string
        if (moveServo) {
            switch (string) {
                case 1: servo1.move(0); break;
                case 2: servo2.move(0); break;
                case 3: servo3.move(0); break;
                case 4: servo4.move(0); break;
                case 5: servo5.move(0); break;
                case 6: servo6.move(0); break;
            }
        }
        
        // Clear all frets for this string
        for (int f = 0; f < NUM_FRETS; f++) {
            switch (string) {
                case 1: fretStates[f] &= ~string1; break;
                case 2: fretStates[f] &= ~string2; break;
                case 3: fretStates[f] &= ~string3; break;
                case 4: fretStates[f] &= ~string4; break;
                case 5: fretStates[f] &= ~string5; break;
                case 6: fretStates[f] &= ~string6; break;
            }
            
            int clkPin = fretPins[f][0];
            int dataPin = fretPins[f][1];
            int clearPin = fretPins[f][2];
            digitalWrite(clearPin, HIGH);
            shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
        }
    } else if (fret >= 1 && fret <= NUM_FRETS) {
        // Fretted note
        if (moveServo) {
            switch (string) {
                case 1: servo1.move(0); break;
                case 2: servo2.move(0); break;
                case 3: servo3.move(0); break;
                case 4: servo4.move(0); break;
                case 5: servo5.move(0); break;
                case 6: servo6.move(0); break;
            }
        }
        
        int fretIndex = fret - 1;
        bool alreadyHeld = false;
        
        switch (string) {
            case 1: alreadyHeld = (fretStates[fretIndex] & string1); break;
            case 2: alreadyHeld = (fretStates[fretIndex] & string2); break;
            case 3: alreadyHeld = (fretStates[fretIndex] & string3); break;
            case 4: alreadyHeld = (fretStates[fretIndex] & string4); break;
            case 5: alreadyHeld = (fretStates[fretIndex] & string5); break;
            case 6: alreadyHeld = (fretStates[fretIndex] & string6); break;
        }
        
        if (!alreadyHeld) {
            switch (string) {
                case 1: fretStates[fretIndex] |= string1; break;
                case 2: fretStates[fretIndex] |= string2; break;
                case 3: fretStates[fretIndex] |= string3; break;
                case 4: fretStates[fretIndex] |= string4; break;
                case 5: fretStates[fretIndex] |= string5; break;
                case 6: fretStates[fretIndex] |= string6; break;
            }
        }
        
        int clkPin = fretPins[fretIndex][0];
        int dataPin = fretPins[fretIndex][1];
        int clearPin = fretPins[fretIndex][2];
        digitalWrite(clearPin, HIGH);
        shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
    }
}


void sendPlaybackStatusSafe(Uart &instrUart, unsigned long totalTime) {

    
    unsigned long currentPlayTime;
    if (isPaused) {
        currentPlayTime = pauseOffset;
    } else if (isPlaying) {
        currentPlayTime = millis() - startTime;
    } else {
        currentPlayTime = 0;
    }
    
    // Use static buffer - no dynamic allocations
    char statusBuffer[64];
    snprintf(statusBuffer, sizeof(statusBuffer), 
             "STATUS:{\"currentTime\":%lu,\"totalTime\":%lu}\n",
             currentPlayTime, totalTime);
    
    instrUart.print(statusBuffer);
}
void playGuitarRTOS_safe(const char* filePath) {
    static File file;
    static JsonDocument doc;
    static bool fileLoaded = false;
    static unsigned long cachedTotalTime = 0;
    static bool totalTimeCached = false;
    static const unsigned long SERVO_THRESH = 100;
    size_t MAX_FILE_SIZE = 150000;
    static unsigned long lastStatus = 0;
    static bool fretsCleared = false;
    
    bool shouldSendStatus = false;
    if (millis() - lastStatus > 1000) {
        shouldSendStatus = true;
        lastStatus = millis();
    }

    if (!isPlaying || isPaused) {
        if (shouldSendStatus) {
            sendPlaybackStatusSafe(instructionUart, cachedTotalTime);
        }

        if (!fretsCleared) {
            clearAllFrets();
            fretsCleared = true;
        }
        return;
    } else {
        fretsCleared = false;
    }

    // Only load the file and parse JSON once per song
    if (!fileLoaded || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            doc.clear();
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open file for reading");
                isPlaying = false;
                return;
            }
            
            size_t fileSize = file.size();
            if (fileSize > MAX_FILE_SIZE) {
                Serial.println("ERROR: File too large");
                file.close();
                xSemaphoreGive(sdSemaphore);
                isPlaying = false;
                fileLoaded = false;
                currentSongPath[0] = '\0';
                instructionUart.println("ERROR:File too large");
                return;
            }

            DeserializationError error = deserializeJson(doc, file);
            file.close();
            xSemaphoreGive(sdSemaphore);
            
            if (error) {
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                doc.clear();
                isPlaying = false;
                fileLoaded = false;
                return;
            }
            
            // ← NO MORE JsonArray - work directly with document
            fileLoaded = true;
            
            // Cache total time directly from document
            if (doc["events"].size() > 0) {
                size_t lastIndex = doc["events"].size() - 1;
                cachedTotalTime = doc["events"][lastIndex]["time"].as<unsigned long>();
                totalTimeCached = true;
                Serial.printf("New song loaded, total time: %lu\n", cachedTotalTime);
            }
            
            if (newSongRequested){
                currentEventIndex = 0;
                startTime = millis();
                pauseOffset = 0;
                newSongRequested = false;
            }
        } else {
            Serial.println("Failed to take SD semaphore");
            return;
        }
    }

    // Send status with cached time
    if (shouldSendStatus && fileLoaded && totalTimeCached) {
        sendPlaybackStatusSafe(instructionUart, cachedTotalTime);
    }

    // ← CRITICAL FIX: Direct document access - no JsonArray
    if (currentEventIndex < doc["events"].size()) {
        JsonObject event = doc["events"][currentEventIndex];  // Direct access
        unsigned long eventTime = event["time"].as<unsigned long>();
        int string = event["string"].as<int>();
        int fret = event["fret"].as<int>();

        if (millis() - startTime >= eventTime) {
            bool moveServo = false;
            if (fret > 0) {
                moveServo = true;
            }
            
            processGuitarEvent(string, fret, moveServo);
            currentEventIndex++;
        }
    } else {
        // Song finished
        if (fileLoaded && !doc.isNull()) {
            sendPlaybackStatusSafe(instructionUart, cachedTotalTime);
        }
        
        doc.clear(); 
        currentSongPath[0] = '\0';
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        totalTimeCached = false;
        cachedTotalTime = 0;
        
        Serial.println("Playback finished - memory cleaned");
    }
}