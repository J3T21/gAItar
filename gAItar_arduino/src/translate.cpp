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

// FIX: Define a single file-scope JsonDocument instead of extern
JsonDocument doc;

void playGuitarRTOS(const char* filePath) {
    static File file;
    // REMOVE: static JsonDocument doc; - now using file-scope doc
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
    // REMOVE: static JsonDocument doc; - now using file-scope doc
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
        Serial.println("Playbook finished.");
    }
}


// FIX: resumePlaybackAtCurrentEvent now works correctly with file-scope doc
void resumePlaybackAtCurrentEvent() {
    extern size_t currentEventIndex;
    extern unsigned long startTime;

    if (doc["events"].size() > currentEventIndex) {
        unsigned long eventTime = doc["events"][currentEventIndex]["time"].as<unsigned long>();
        startTime = millis() - eventTime;
        Serial.printf("Resuming at event %zu (time %lu)\n", currentEventIndex, eventTime);
    } else {
        startTime = millis();
        Serial.println("Resuming at end of song");
    }
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
    // REMOVE: static JsonDocument doc; - now using file-scope doc
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

void playGuitarRTOS_Binary(const char* filePath) {
    static File file;
    static bool fileLoaded = false;
    static unsigned long lastStatus = 0;
    static bool fretsCleared = false;
    
    // ✅ FIX: Make these static so they persist between function calls
    static unsigned long totalDurationMs = 0;
    static uint16_t eventCount = 0;
    static unsigned long currentEventTime = 0;
    static uint8_t currentString = 0;
    static int8_t currentFret = 0;
    static bool eventReady = false;
    
    // ✅ Access global variables for resume support
    extern size_t currentEventIndex;
    extern unsigned long startTime;
    extern unsigned long pauseOffset;
    extern volatile bool newSongRequested;
    
    bool shouldSendStatus = false;
    if (millis() - lastStatus > 1000) {
        shouldSendStatus = true;
        lastStatus = millis();
    }

    if (!isPlaying || isPaused) {
        if (shouldSendStatus) {
            sendPlaybackStatusSafe(instructionUart, totalDurationMs);
        }

        if (!fretsCleared) {
            clearAllFrets();
            fretsCleared = true;
        }
        return;
    } else {
        fretsCleared = false;
    }

    // Load binary file header and prepare for event streaming
    if (!fileLoaded || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            // ✅ FIX: Always close existing file first to prevent handle leaks
            if (file) {
                file.close();
            }
            
            // ✅ FIX: Reset static variables when switching songs
            if (newSongRequested) {
                totalDurationMs = 0;
                eventCount = 0;
                currentEventTime = 0;
                currentString = 0;
                currentFret = 0;
                eventReady = false;
            }
            
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open binary file for reading");
                isPlaying = false;
                fileLoaded = false;
                return;
            }
            
            // Check minimum file size (6 byte header)
            size_t fileSize = file.size();
            if (fileSize < 6) {
                Serial.println("ERROR: Binary file too small");
                file.close();
                xSemaphoreGive(sdSemaphore);
                isPlaying = false;
                fileLoaded = false;
                currentSongPath[0] = '\0';
                instructionUart.println("ERROR:Invalid binary file");
                return;
            }

            // Read 6-byte header: duration (4 bytes) + event count (2 bytes)
            uint8_t header[6];
            if (file.read(header, 6) != 6) {
                Serial.println("ERROR: Failed to read binary header");
                file.close();
                xSemaphoreGive(sdSemaphore);
                isPlaying = false;
                fileLoaded = false;
                return;
            }

            // Parse header (big-endian format)
            totalDurationMs = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
            eventCount = (header[4] << 8) | header[5];
            
            Serial.printf("Binary file loaded: %u events, duration: %lu ms\n", eventCount, totalDurationMs);
            
            // Validate file size matches expected event count
            size_t expectedSize = 6 + (eventCount * 5); // 6 byte header + 5 bytes per event
            if (fileSize != expectedSize) {
                Serial.printf("ERROR: File size mismatch. Expected: %u, Actual: %u\n", expectedSize, fileSize);
                file.close();
                xSemaphoreGive(sdSemaphore);
                isPlaying = false;
                fileLoaded = false;
                return;
            }

            fileLoaded = true;
            
            // ✅ FIX: Proper timing logic for new songs vs resume
            if (newSongRequested) {
                currentEventIndex = 0;  // Reset global index for new song
                startTime = millis();
                pauseOffset = 0;
                newSongRequested = false;
            } else {
                // Resuming from pause
                startTime = millis() - pauseOffset;
            }
            
            xSemaphoreGive(sdSemaphore);
        } else {
            Serial.println("Failed to take SD semaphore");
            return;
        }
    }

    // Send status updates
    if (shouldSendStatus && fileLoaded) {
        sendPlaybackStatusSafe(instructionUart, totalDurationMs);
    }

    // ✅ FIX: Bounds checking with current song's event count
    if (!eventReady && currentEventIndex < eventCount && fileLoaded) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            // Calculate file position for current event
            size_t eventPosition = 6 + (currentEventIndex * 5); // 6 byte header + 5 bytes per event
            
            if (file && file.seek(eventPosition)) {
                uint8_t eventData[5];
                if (file.read(eventData, 5) == 5) {
                    // Parse 5-byte event: time_ms (4 bytes) + packed_string_fret (1 byte)
                    currentEventTime = (eventData[0] << 24) | (eventData[1] << 16) | (eventData[2] << 8) | eventData[3];
                    uint8_t packedByte = eventData[4];
                    
                    // Unpack: string (3 bits) + fret (5 bits)
                    currentString = (packedByte >> 5) & 0x07; // Extract upper 3 bits for string (1-6)
                    uint8_t fretValue = packedByte & 0x1F;     // Extract lower 5 bits for fret (0-31)
                    
                    // Convert fret: 31 means fret-off (-1), otherwise 0-30
                    currentFret = (fretValue == 31) ? -1 : (int8_t)fretValue;
                    
                    eventReady = true;
                } else {
                    Serial.println("ERROR: Failed to read event data");
                    isPlaying = false;
                    fileLoaded = false;
                }
            } else {
                Serial.println("ERROR: Failed to seek to event position or invalid file");
                isPlaying = false;
                fileLoaded = false;
            }
            
            xSemaphoreGive(sdSemaphore);
        } else {
            Serial.println("Failed to take SD semaphore for event reading");
            return;
        }
    }

    // Play the current event if it's time
    if (eventReady && (millis() - startTime >= currentEventTime)) {
        // Validate string range
        if (currentString >= 1 && currentString <= 6) {
            bool moveServo = false;
            if (currentFret > 0) {
                moveServo = true;
            }
            
            // Process the guitar event using existing function
            processGuitarEvent(currentString, currentFret, moveServo);
        } else {
            Serial.printf("ERROR: Invalid string number: %u\n", currentString);
        }
        
        // Move to next event
        currentEventIndex++;
        eventReady = false;
    }

    // Check if song is finished
    if (currentEventIndex >= eventCount && fileLoaded) {
        // Song finished
        if (shouldSendStatus) {
            sendPlaybackStatusSafe(instructionUart, totalDurationMs);
        }
        
        // ✅ FIX: Proper cleanup
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (file) {
                file.close();
            }
            xSemaphoreGive(sdSemaphore);
        }
        
        // Reset all state
        currentSongPath[0] = '\0';
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        currentEventIndex = 0;
        
        Serial.println("Binary playback finished");
    }
}