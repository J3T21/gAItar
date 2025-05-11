#include "translate.h"
#include <ArduinoJson.h>
#include <SdFat.h>

unsigned long currentTime = 0;
unsigned long lastUpdateTime = 0;

NoteState noteStates[6];  // One per string

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