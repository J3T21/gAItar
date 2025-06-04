#include "translate.h"
#include <ArduinoJson.h>
#include <SdFat.h>
#include <FreeRTOS_SAMD51.h>
#include "uart_transfer.h"


unsigned long currentTime = 0;
unsigned long lastUpdateTime = 0;

NoteState noteStates[6];  // One per string

uint16_t eventCount = 0; // Number of events deprecated
int events[0][3] = {};

extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern char currentSongPath[128];
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;

void playGuitarEvents() {
    static unsigned long startTime = millis(); // Record the start time
    static int currentEventIndex = 0;          // Track the current event being processed

    // Ensure we don't process events if the array is empty or invalid
    if (eventCount <= 0 || currentEventIndex >= eventCount) {
        return; // No events to process
    }



    // Process events in order
    while (currentEventIndex < eventCount) {
        unsigned long eventTime = events[currentEventIndex][0]; // Absolute time of the event
        int string = events[currentEventIndex][1];              // String number (1-based index)
        int fret = events[currentEventIndex][2];                // Fret number (positive for fret, -1 for string off)

        // Debugging: Print the current event details
        Serial.print("Processing Event Index: ");
        Serial.print(currentEventIndex);
        Serial.print(", Event Time: ");
        Serial.print(eventTime);
        Serial.print(", String: ");
        Serial.print(string);
        Serial.print(", Fret: ");
        Serial.println(fret);

        // Check if it's time to process the current event
        if (millis() - startTime >= eventTime) {
            if (fret == -1) {
                switch (string) {
                    case 1: servo1.damper(); break;
                    case 2: servo2.damper(); break;
                    case 3: servo3.damper(); break;
                    case 4: servo4.damper(); break;
                    case 5: servo5.damper(); break;
                    case 6: servo6.damper(); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                // Turn off the string for all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    // Use the predefined string constants to clear the correct bit
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else {
                // Activate the specified fret and string
                if (fret < 0 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    return; // Exit if invalid fret number
                }else if (fret == 0){
                    Serial.println("Open fret!");
                }
                int fretIndex = fret - 1; // Adjust for 0-based indexing

                // Use the predefined string constants to set the correct bit
                switch (string) {
                    case 1: servo1.move(0);
                            fretStates[fretIndex] |= string1;                     
                        break;
                    case 2: servo2.move(0);
                            fretStates[fretIndex] |= string2;                 
                        break;
                    case 3: servo3.move(0);
                            fretStates[fretIndex] |= string3; 
                        break;
                    case 4: servo4.move(0);
                            fretStates[fretIndex] |= string4; 
                        break;
                    case 5: servo5.move(0);
                            fretStates[fretIndex] |= string5;
                        break;
                    case 6: servo6.move(0);
                            fretStates[fretIndex] |= string6; 
                        break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }

                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];

                // Update the shift register
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }

            // Move to the next event
            currentEventIndex++;
        } else {
            // If it's not time for the current event, exit the loop
            break;
        }
    }
}

void playFrets() {
    static unsigned long startTime = millis(); // Record the start time
    static int currentEventIndex = 0;          // Track the current event being processed

    // Ensure we don't process events if the array is empty or invalid
    if (eventCount <= 0 || currentEventIndex >= eventCount) {
        return; // No events to process
    }



    // Process events in order
    while (currentEventIndex < eventCount) {
        unsigned long eventTime = events[currentEventIndex][0]; // Absolute time of the event
        int string = events[currentEventIndex][1];              // String number (1-based index)
        int fret = events[currentEventIndex][2];                // Fret number (positive for fret, -1 for string off)

        // Debugging: Print the current event details
        Serial.print("Processing Event Index: ");
        Serial.print(currentEventIndex);
        Serial.print(", Event Time: ");
        Serial.print(eventTime);
        Serial.print(", String: ");
        Serial.print(string);
        Serial.print(", Fret: ");
        Serial.println(fret);

        // Check if it's time to process the current event
        if (millis() - startTime >= eventTime) {
            if (fret == -1) {
                // Turn off the string for all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    // Use the predefined string constants to clear the correct bit
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else {
                // Activate the specified fret and string
                if (fret < 1 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    return; // Exit if invalid fret number
                }

                int fretIndex = fret - 1; // Adjust for 0-based indexing
                bool alreadyHeld = false;

                switch (string) {
                    case 1: alreadyHeld = (fretStates[fretIndex] & string1); break;
                    case 2: alreadyHeld = (fretStates[fretIndex] & string2); break;
                    case 3: alreadyHeld = (fretStates[fretIndex] & string3); break;
                    case 4: alreadyHeld = (fretStates[fretIndex] & string4); break;
                    case 5: alreadyHeld = (fretStates[fretIndex] & string5); break;
                    case 6: alreadyHeld = (fretStates[fretIndex] & string6); break;
                }

                // Use the predefined string constants to set the correct bit
                switch (string) {
                    case 1: fretStates[fretIndex] |= string1; break;
                    case 2: fretStates[fretIndex] |= string2; break;
                    case 3: fretStates[fretIndex] |= string3; break;
                    case 4: fretStates[fretIndex] |= string4; break;
                    case 5: fretStates[fretIndex] |= string5; break;
                    case 6: fretStates[fretIndex] |= string6; break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }

                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];

                // Update the shift register
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }

            // Move to the next event
            currentEventIndex++;
        } else {
            // If it's not time for the current event, exit the loop
            break;
        }
    }
}

void playGuitarEventsOpen() {
    static unsigned long startTime = millis(); // Record the start time
    static int currentEventIndex = 0;          // Track the current event being processed

    // Ensure we don't process events if the array is empty or invalid
    if (eventCount <= 0 || currentEventIndex >= eventCount) {
        return; // No events to process
    }

    // Process events in order
    while (currentEventIndex < eventCount) {
        unsigned long eventTime = events[currentEventIndex][0]; // Absolute time of the event
        int string = events[currentEventIndex][1];              // String number (1-based index)
        int fret = events[currentEventIndex][2];                // Fret number (positive for fret, 0 for open string, -1 for string off)

        // Debugging: Print the current event details
        Serial.print("Processing Event Index: ");
        Serial.print(currentEventIndex);
        Serial.print(", Event Time: ");
        Serial.print(eventTime);
        Serial.print(", String: ");
        Serial.print(string);
        Serial.print(", Fret: ");
        Serial.println(fret);

        // Check if it's time to process the current event
        if (millis() - startTime >= eventTime) {
            if (fret == -1) {
                // Handle string off (dampening)
                switch (string) {
                    case 1: servo1.damper(); break;
                    case 2: servo2.damper(); break;
                    case 3: servo3.damper(); break;
                    case 4: servo4.damper(); break;
                    case 5: servo5.damper(); break;
                    case 6: servo6.damper(); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                // Turn off the string for all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    // Use the predefined string constants to clear the correct bit
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else if (fret == 0) {
                // Handle open string
                Serial.print("Playing open string: ");
                Serial.println(string);
                                // Move the servo to the dampening position to ensure no fret is pressed
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                // Clear the string from all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }


            } else {
                // Handle fretted notes
                if (fret < 1 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    return; // Exit if invalid fret number
                }
                                // Move the servo to the dampening position to ensure no fret is pressed
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                int fretIndex = fret - 1; // Adjust for 0-based indexing

                // Use the predefined string constants to set the correct bit
                switch (string) {
                    case 1: fretStates[fretIndex] |= string1; break;
                    case 2: fretStates[fretIndex] |= string2; break;
                    case 3: fretStates[fretIndex] |= string3; break;
                    case 4: fretStates[fretIndex] |= string4; break;
                    case 5: fretStates[fretIndex] |= string5; break;
                    case 6: fretStates[fretIndex] |= string6; break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }

                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];

                // Update the shift register
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }

            // Move to the next event
            currentEventIndex++;
        } else {
            // If it's not time for the current event, exit the loop
            break;
        }
    }
}

void sendPlaybackStatus(Uart &instrUart, JsonArray *eventsPtr) {
    static unsigned long lastStatusUpdate = 0;
    static unsigned long cachedTotalTime = 0;
    static bool totalTimeCalculated = false;
    
    const unsigned long STATUS_UPDATE_INTERVAL = 1000; // Send status every 1 second
    
    // Only send status updates at intervals
    if (millis() - lastStatusUpdate < STATUS_UPDATE_INTERVAL) {
        return;
    }
    
    unsigned long currentPlayTime;
    if (isPaused) {
        currentPlayTime = pauseOffset;
    } else if (isPlaying) {
        currentPlayTime = millis() - startTime;
    } else {
        currentPlayTime = 0;
    }
    
    // Calculate total time ONCE and cache it to avoid repeated JSON access
    if (!totalTimeCalculated && eventsPtr != nullptr && !eventsPtr->isNull() && eventsPtr->size() > 0) {
        // Access the last event only once
        size_t lastIndex = eventsPtr->size() - 1;
        
        // Use array-style access instead of JsonObject to minimize memory allocation
        JsonVariant lastEventTime = (*eventsPtr)[lastIndex]["time"];
        
        if (!lastEventTime.isNull()) {
            cachedTotalTime = lastEventTime.as<unsigned long>();
            totalTimeCalculated = true;
            Serial.printf("Cached total time: %lu\n", cachedTotalTime);
        } else {
            cachedTotalTime = 0;
        }
    }
    
    // Use static buffer instead of printf to avoid dynamic formatting
    char statusBuffer[64];
    snprintf(statusBuffer, sizeof(statusBuffer), 
             "STATUS:{\"currentTime\":%lu,\"totalTime\":%lu}\n",
             currentPlayTime, cachedTotalTime);
    
    instrUart.print(statusBuffer);
    lastStatusUpdate = millis();
}

void playGuitarFromFile(const char* filePath){
    File file = sd.open(filePath, FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print(F("Failed to read file, error: "));
        Serial.println(error.c_str());
        file.close();
        return;
    }
    file.close();
    JsonArray events = doc["events"];
    static unsigned long startTime = millis();
    static size_t currentEventIndex = 0;          // Track the current event being processed
    while (currentEventIndex < events.size()) {
        JsonObject event = events[currentEventIndex];  
        unsigned long eventTime = event["time"];
        int string = event["string"];
        int fret = event["fret"];

        if (millis() - startTime >= eventTime) {
            if (fret == -1) {
                // Handle string off (dampening)
                switch (string) {
                    case 1: servo1.damper(); break;
                    case 2: servo2.damper(); break;
                    case 3: servo3.damper(); break;
                    case 4: servo4.damper(); break;
                    case 5: servo5.damper(); break;
                    case 6: servo6.damper(); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                // Turn off the string for all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    // Use the predefined string constants to clear the correct bit
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }
            } else if (fret == 0) {
                // Handle open string
                Serial.print("Playing open string: ");
                Serial.println(string);
                                // Move the servo to the dampening position to ensure no fret is pressed
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                // Clear the string from all frets
                for (int f = 0; f < NUM_FRETS; f++) {
                    switch (string) {
                        case 1: fretStates[f] &= ~string1; break;
                        case 2: fretStates[f] &= ~string2; break;
                        case 3: fretStates[f] &= ~string3; break;
                        case 4: fretStates[f] &= ~string4; break;
                        case 5: fretStates[f] &= ~string5; break;
                        case 6: fretStates[f] &= ~string6; break;
                        default:
                            Serial.println("Invalid string number!");
                            return; // Exit if invalid string number
                    }

                    int clkPin = fretPins[f][0];
                    int dataPin = fretPins[f][1];
                    int clearPin = fretPins[f][2];

                    // Update the shift register
                    digitalWrite(clearPin, HIGH);
                    shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
                }


            } else {
                // Handle fretted notes
                if (fret < 1 || fret > NUM_FRETS) {
                    Serial.println("Invalid fret number!");
                    return; // Exit if invalid fret number
                }
                                // Move the servo to the dampening position to ensure no fret is pressed
                switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }
                int fretIndex = fret - 1; // Adjust for 0-based indexing

                // Use the predefined string constants to set the correct bit
                switch (string) {
                    case 1: fretStates[fretIndex] |= string1; break;
                    case 2: fretStates[fretIndex] |= string2; break;
                    case 3: fretStates[fretIndex] |= string3; break;
                    case 4: fretStates[fretIndex] |= string4; break;
                    case 5: fretStates[fretIndex] |= string5; break;
                    case 6: fretStates[fretIndex] |= string6; break;
                    default:
                        Serial.println("Invalid string number!");
                        return; // Exit if invalid string number
                }

                int clkPin = fretPins[fretIndex][0];
                int dataPin = fretPins[fretIndex][1];
                int clearPin = fretPins[fretIndex][2];

                // Update the shift register
                digitalWrite(clearPin, HIGH);
                shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
            }

            currentEventIndex++;
        } else {
            // If it's not time for the current event, exit the loop
            break;
        }
    }
}

void playGuitarRTOS(const char* filePath) {
    static File file;
    static JsonDocument doc;
    static JsonArray events;
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
    static JsonArray events;
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
                if (moveServo){
                    switch (string) {
                    case 1: servo1.move(0); break;
                    case 2: servo2.move(0); break;
                    case 3: servo3.move(0); break;
                    case 4: servo4.move(0); break;
                    case 5: servo5.move(0); break;
                    case 6: servo6.move(0); break;
                    default: Serial.println("Invalid string number!"); break;
                    }
                } else{
                    //Serial.println("Releasing");
                    // switch (string){
                    //     case 1: servo1.release(); break;
                    //     case 2: servo2.release(); break;
                    //     case 3: servo3.release(); break;
                    //     case 4: servo4.release(); break;
                    //     case 5: servo5.release(); break;
                    //     case 6: servo6.release(); break;
                    //     default: break;
                    // }
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

bool seekToEventsArray(File& file) {
    char buffer[64];
    bool foundEvents = false;
    
    file.seek(0);  // Start from beginning
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.indexOf("\"events\"") >= 0 && line.indexOf("[") >= 0) {
            foundEvents = true;
            break;
        }
    }
    
    return foundEvents;
}

void loadNextEventBatch(File& file, Event* eventBuffer, size_t& bufferSize) {
    bufferSize = 0;
    char buffer[128];
    
    while (file.available() && bufferSize < 50) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        // Skip empty lines and structural characters
        if (line.length() < 10 || line.indexOf("{") < 0) {
            continue;
        }
        
        // Check for end of events array
        if (line.indexOf("]") >= 0 && line.indexOf("{") < 0) {
            break;
        }
        
        // Parse individual event line
        Event event;
        if (parseEventLine(line, event)) {
            eventBuffer[bufferSize] = event;
            bufferSize++;
        }
    }
    
    Serial.printf("Loaded %u events into buffer\n", bufferSize);
}

bool parseEventLine(const String& line, Event& event) {
    // Simple string parsing for: {"time":1000,"string":1,"fret":3}
    int timeStart = line.indexOf("\"time\":") + 7;
    int timeEnd = line.indexOf(",", timeStart);
    if (timeStart < 7 || timeEnd < 0) return false;
    
    int stringStart = line.indexOf("\"string\":") + 9;
    int stringEnd = line.indexOf(",", stringStart);
    if (stringStart < 9 || stringEnd < 0) return false;
    
    int fretStart = line.indexOf("\"fret\":") + 7;
    int fretEnd = line.indexOf("}", fretStart);
    if (fretStart < 7 || fretEnd < 0) return false;
    
    event.time = line.substring(timeStart, timeEnd).toInt();
    event.string = line.substring(stringStart, stringEnd).toInt();
    event.fret = line.substring(fretStart, fretEnd).toInt();
    
    return (event.time > 0 && event.string >= 1 && event.string <= 6);
}

unsigned long calculateNoteDuration(Event* eventBuffer, size_t currentIndex, size_t bufferSize, int string, unsigned long currentTime) {
    // Look ahead in buffer for next event on same string
    for (size_t i = currentIndex + 1; i < bufferSize; i++) {
        if (eventBuffer[i].string == string) {
            return eventBuffer[i].time - currentTime;
        }
    }
    
    // Default duration if not found in buffer
    return 200;  // Assume 200ms default
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

void sendPlaybackStatusStreaming(Uart &instrUart) {
    static unsigned long lastStatusUpdate = 0;
    
    if (millis() - lastStatusUpdate < 1000) {
        return;
    }
    
    unsigned long currentPlayTime = isPaused ? pauseOffset : 
                                   (isPlaying ? millis() - startTime : 0);
    
    // For streaming, we can't easily get total time, so send 0 or estimate
    instrUart.printf("STATUS:{\"currentTime\":%lu,\"totalTime\":%lu}\n",
                    currentPlayTime, 0UL);
    lastStatusUpdate = millis();
}


void playGuitarRTOS_Stream(const char* filePath) {
    static File file;
    static bool fileOpen = false;
    static bool songInitialized = false;
    static const unsigned long SERVO_THRESH = 100;
    
    // Event buffer - only keep a small number of events in memory
    static Event eventBuffer[50];  // Buffer for 120 events at a time
    static size_t bufferSize = 0;
    static size_t bufferIndex = 0;
    static bool fileComplete = false;

    if (!isPlaying || isPaused) {
        sendPlaybackStatus(instructionUart, nullptr);
        
        // Cleanup
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (fileOpen && file) {
                file.close();
                fileOpen = false;
            }
            xSemaphoreGive(sdSemaphore);
        }
        
        songInitialized = false;
        fileComplete = false;
        bufferSize = 0;
        bufferIndex = 0;
        clearAllFrets();
        return;
    }

    // Initialize song if needed
    if (!songInitialized || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (fileOpen && file) {
                file.close();
                fileOpen = false;
            }
            
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open file for reading");
                isPlaying = false;
                return;
            }
            
            fileOpen = true;
            
            // Skip to the events array in the JSON file
            if (!seekToEventsArray(file)) {
                Serial.println("Failed to find events array in JSON");
                file.close();
                fileOpen = false;
                xSemaphoreGive(sdSemaphore);
                isPlaying = false;
                return;
            }
            
            xSemaphoreGive(sdSemaphore);
            
            // Load initial buffer
            loadNextEventBatch(file, eventBuffer, bufferSize);
            bufferIndex = 0;
            fileComplete = false;
            songInitialized = true;
            
            if (newSongRequested) {
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

    // Send status updates
    sendPlaybackStatusStreaming(instructionUart);

    // Process events from buffer
    if (bufferIndex < bufferSize) {
        Event& event = eventBuffer[bufferIndex];
        
        if (millis() - startTime >= event.time) {
            // Calculate servo movement decision
            unsigned long noteDuration = 0;
            bool moveServo = false;
            
            if (event.fret > 0) {
                // Look ahead in buffer for next event on same string
                noteDuration = calculateNoteDuration(eventBuffer, bufferIndex, bufferSize, event.string, event.time);
                moveServo = (noteDuration >= SERVO_THRESH);
            }
            
            // Process the event
            processGuitarEvent(event.string, event.fret, moveServo);
            
            bufferIndex++;
            currentEventIndex++;
        }
    } else if (!fileComplete) {
        // Buffer exhausted, load next batch
        if (xSemaphoreTake(sdSemaphore, 100)) {  // Short timeout
            if (fileOpen && file && file.available()) {
                loadNextEventBatch(file, eventBuffer, bufferSize);
                bufferIndex = 0;
                
                if (bufferSize == 0) {
                    fileComplete = true;
                }
            } else {
                fileComplete = true;
            }
            xSemaphoreGive(sdSemaphore);
        }
    } else {
        // Song finished
        sendPlaybackStatusStreaming(instructionUart);
        
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (fileOpen && file) {
                file.close();
                fileOpen = false;
            }
            xSemaphoreGive(sdSemaphore);
        }
        
        currentSongPath[0] = '\0';
        isPlaying = false;
        songInitialized = false;
        newSongRequested = true;
        Serial.println("Playback finished.");
    }
}