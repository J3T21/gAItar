#include "translate.h"
#include <SdFat.h>
#include <FreeRTOS_SAMD51.h>
#include "uart_transfer.h"

// External global playback state variables
extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern char currentSongPath[128];
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;

/**
 * Hardware control function for processing individual guitar events
 * Handles three types of events: string off (-1), open string (0), and fretted notes (1-12)
 * Controls both servo motors for string actuation and shift registers for fret solenoids
 * 
 * @param string Guitar string number (1-6, High E to Low E)
 * @param fret Fret position (-1=off, 0=open, 1-12=fretted)
 * @param moveServo Actuate the string servo motor
 */
void processGuitarEvent(int string, int fret, bool moveServo) {
    if (fret == -1) {
        // String off event - clear all fret solenoids for this string
        for (int f = 0; f < NUM_FRETS; f++) {
            // Clear the string bit from each fret's state register
            switch (string) {
                case 1: fretStates[f] &= ~string1; break;
                case 2: fretStates[f] &= ~string2; break;
                case 3: fretStates[f] &= ~string3; break;
                case 4: fretStates[f] &= ~string4; break;
                case 5: fretStates[f] &= ~string5; break;
                case 6: fretStates[f] &= ~string6; break;
            }
            
            // Update shift register hardware with new state
            int clkPin = fretPins[f][0];
            int dataPin = fretPins[f][1];
            int clearPin = fretPins[f][2];
            digitalWrite(clearPin, HIGH);
            shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
        }
    } else if (fret == 0) {
        // Open string event - actuate servo and clear all frets
        if (moveServo) {
            // Actuate appropriate servo motor for string picking
            switch (string) {
                case 1: servo1.move(0); break;
                case 2: servo2.move(0); break;
                case 3: servo3.move(0); break;
                case 4: servo4.move(0); break;
                case 5: servo5.move(0); break;
                case 6: servo6.move(0); break;
            }
        }
        
        // Clear all fret solenoids for this string (open string requires no fretting)
        for (int f = 0; f < NUM_FRETS; f++) {
            switch (string) {
                case 1: fretStates[f] &= ~string1; break;
                case 2: fretStates[f] &= ~string2; break;
                case 3: fretStates[f] &= ~string3; break;
                case 4: fretStates[f] &= ~string4; break;
                case 5: fretStates[f] &= ~string5; break;
                case 6: fretStates[f] &= ~string6; break;
            }
            
            // Update shift register hardware
            int clkPin = fretPins[f][0];
            int dataPin = fretPins[f][1];
            int clearPin = fretPins[f][2];
            digitalWrite(clearPin, HIGH);
            shiftOut(dataPin, clkPin, LSBFIRST, fretStates[f]);
        }
    } else if (fret >= 1 && fret <= NUM_FRETS) {
        // Fretted note event - actuate servo and engage appropriate fret solenoid
        if (moveServo) {
            // Actuate servo motor for string picking
            switch (string) {
                case 1: servo1.move(0); break;
                case 2: servo2.move(0); break;
                case 3: servo3.move(0); break;
                case 4: servo4.move(0); break;
                case 5: servo5.move(0); break;
                case 6: servo6.move(0); break;
            }
        }
        
        int fretIndex = fret - 1; // Convert to zero-based array index
        bool alreadyHeld = false;
        
        // Check if this string is already being held at this fret
        switch (string) {
            case 1: alreadyHeld = (fretStates[fretIndex] & string1); break;
            case 2: alreadyHeld = (fretStates[fretIndex] & string2); break;
            case 3: alreadyHeld = (fretStates[fretIndex] & string3); break;
            case 4: alreadyHeld = (fretStates[fretIndex] & string4); break;
            case 5: alreadyHeld = (fretStates[fretIndex] & string5); break;
            case 6: alreadyHeld = (fretStates[fretIndex] & string6); break;
        }
        
        // Only engage solenoid if not already held (prevents unnecessary actuations)
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
        
        // Update shift register hardware for the specific fret
        int clkPin = fretPins[fretIndex][0];
        int dataPin = fretPins[fretIndex][1];
        int clearPin = fretPins[fretIndex][2];
        digitalWrite(clearPin, HIGH);
        shiftOut(dataPin, clkPin, LSBFIRST, fretStates[fretIndex]);
    }
}

/**
 * Transmits current playback status over UART interface
 * Sends JSON-formatted status information for external monitoring systems
 * Uses static buffer allocation to prevent dynamic memory fragmentation
 * 
 * @param instrUart UART interface for status transmission
 * @param totalTime Total song duration in milliseconds
 */
void sendPlaybackStatusSafe(Uart &instrUart, unsigned long totalTime) {
    unsigned long currentPlayTime;
    
    // Calculate current playback position based on system state
    if (isPaused) {
        currentPlayTime = pauseOffset; // Use saved position when paused
    } else if (isPlaying) {
        currentPlayTime = millis() - startTime; // Calculate elapsed time during playback
    } else {
        currentPlayTime = 0; // No playback active
    }
    
    // Format status message using static buffer (no heap allocation)
    char statusBuffer[64];
    snprintf(statusBuffer, sizeof(statusBuffer), 
             "STATUS:{\"currentTime\":%lu,\"totalTime\":%lu}\n",
             currentPlayTime, totalTime);
    
    instrUart.print(statusBuffer);
}

/**
 * Main binary guitar playback engine
 * Streams binary song files and controls hardware in real-time
 * Implements streaming file parser to minimize memory usage for large songs
 * 
 * Binary file format:
 * - Header: 4 bytes duration (big-endian) + 2 bytes event count (big-endian)
 * - Events: 5 bytes each (4 bytes timestamp + 1 byte packed string/fret data)
 * 
 * @param filePath Path to binary song file on SD card
 */
void playGuitarRTOS_Binary(const char* filePath) {
    // Static variables maintain state between function calls for streaming operation
    static File file;                    // SD card file handle
    static bool fileLoaded = false;      // File initialization state
    static unsigned long lastStatus = 0; // Status update timing
    static bool fretsCleared = false;    // Hardware cleanup state
    
    // Binary file parsing state variables
    static unsigned long totalDurationMs = 0;  // Song duration from header
    static uint16_t eventCount = 0;             // Total events from header
    static unsigned long currentEventTime = 0;  // Current event timestamp
    static uint8_t currentString = 0;           // Current event string number
    static int8_t currentFret = 0;              // Current event fret number
    static bool eventReady = false;             // Event parsing completion flag
    
    // Access external global variables for playback control
    extern size_t currentEventIndex;
    extern unsigned long startTime;
    extern unsigned long pauseOffset;
    extern volatile bool newSongRequested;
    
    // Status update timing control
    bool shouldSendStatus = false;
    if (millis() - lastStatus > 1000) {
        shouldSendStatus = true;
        lastStatus = millis();
    }

    // Handle non-playing states (paused or stopped)
    if (!isPlaying || isPaused) {
        if (shouldSendStatus) {
            sendPlaybackStatusSafe(instructionUart, totalDurationMs);
        }

        // Clear hardware state once when playback stops
        if (!fretsCleared) {
            clearAllFrets(); // Release all solenoids and dampen servos
            fretsCleared = true;
        }
        return;
    } else {
        fretsCleared = false; // Reset flag when playback resumes
    }

    // File loading and header parsing (occurs once per song or on song change)
    if (!fileLoaded || newSongRequested) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            // Close any existing file handle to prevent resource leaks
            if (file) {
                file.close();
            }
            
            // Reset parsing state for new songs
            if (newSongRequested) {
                totalDurationMs = 0;
                eventCount = 0;
                currentEventTime = 0;
                currentString = 0;
                currentFret = 0;
                eventReady = false;
            }
            
            // Open binary song file
            file = sd.open(currentSongPath, FILE_READ);
            if (!file) {
                xSemaphoreGive(sdSemaphore);
                Serial.println("Failed to open binary file for reading");
                isPlaying = false;
                fileLoaded = false;
                return;
            }
            
            // Validate minimum file size (6-byte header)
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

            // Parse header using big-endian byte order
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
            
            // Set up timing for new songs vs. resume operations
            if (newSongRequested) {
                currentEventIndex = 0;  // Start from beginning for new songs
                startTime = millis();
                pauseOffset = 0;
                newSongRequested = false;
            } else {
                // Resume from pause - maintain timing continuity
                startTime = millis() - pauseOffset;
            }
            
            xSemaphoreGive(sdSemaphore);
        } else {
            Serial.println("Failed to take SD semaphore");
            return;
        }
    }

    // Send periodic status updates to external systems
    if (shouldSendStatus && fileLoaded) {
        sendPlaybackStatusSafe(instructionUart, totalDurationMs);
    }

    // Event streaming: Load next event data when needed
    if (!eventReady && currentEventIndex < eventCount && fileLoaded) {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            // Calculate file position for current event (skip 6-byte header)
            size_t eventPosition = 6 + (currentEventIndex * 5);
            
            if (file && file.seek(eventPosition)) {
                uint8_t eventData[5];
                if (file.read(eventData, 5) == 5) {
                    // Parse 5-byte event: timestamp (4 bytes) + packed data (1 byte)
                    currentEventTime = (eventData[0] << 24) | (eventData[1] << 16) | 
                                     (eventData[2] << 8) | eventData[3];
                    uint8_t packedByte = eventData[4];
                    
                    // Unpack string and fret data from single byte
                    // Format: [SSS][FFFFF] where S=string bits, F=fret bits
                    currentString = (packedByte >> 5) & 0x07; // Upper 3 bits for string (1-6)
                    uint8_t fretValue = packedByte & 0x1F;    // Lower 5 bits for fret (0-31)
                    
                    // Convert fret encoding: 31 = string off (-1), otherwise direct value
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

    // Event execution: Process current event when its time arrives
    if (eventReady && (millis() - startTime >= currentEventTime)) {
        // Validate string number range
        if (currentString >= 1 && currentString <= 6) {
            // Determine if servo actuation is needed (for fretted notes)
            bool moveServo = (currentFret > 0);
            
            // Execute hardware control for this event
            processGuitarEvent(currentString, currentFret, moveServo);
        } else {
            Serial.printf("ERROR: Invalid string number: %u\n", currentString);
        }
        
        // Advance to next event
        currentEventIndex++;
        eventReady = false;
    }

    // Song completion handling
    if (currentEventIndex >= eventCount && fileLoaded) {
        // Send final status update
        if (shouldSendStatus) {
            sendPlaybackStatusSafe(instructionUart, totalDurationMs);
        }
        
        // Clean up file resources
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (file) {
                file.close();
            }
            xSemaphoreGive(sdSemaphore);
        }
        
        // Reset all playback state for next song
        currentSongPath[0] = '\0';
        isPlaying = false;
        fileLoaded = false;
        newSongRequested = true;
        currentEventIndex = 0;
        
        Serial.println("Binary playback finished");
    }
}