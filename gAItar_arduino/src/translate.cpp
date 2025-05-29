#include "translate.h"
#include <ArduinoJson.h>
#include <SdFat.h>
#include <FreeRTOS_SAMD51.h>
unsigned long currentTime = 0;
unsigned long lastUpdateTime = 0;

NoteState noteStates[6];  // One per string

uint16_t eventCount = 0; // Number of events deprecated
int events[0][3] = {};

extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern String currentSongPath;
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
            file = sd.open(currentSongPath.c_str(), FILE_READ);
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
        currentSongPath = "";
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
    static const unsigned long SERVO_THRESH = 300;

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
    if (!fileLoaded || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            file = sd.open(currentSongPath.c_str(), FILE_READ);
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
                    Serial.println("Releasing");
                    switch (string){
                        case 1: servo1.release(); break;
                        case 2: servo2.release(); break;
                        case 3: servo3.release(); break;
                        case 4: servo4.release(); break;
                        case 5: servo5.release(); break;
                        case 6: servo6.release(); break;
                        default: break;
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
        currentSongPath = "";
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        Serial.println("Playback finished.");
    }
}