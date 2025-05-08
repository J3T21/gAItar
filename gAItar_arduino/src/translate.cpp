#include "translate.h"

unsigned long currentTime = 0;
unsigned long lastUpdateTime = 0;

NoteState noteStates[6];  // One per string

// void playGuitarEvents() {
//     unsigned long currentTime = getCurrentTime();

//     for (size_t i = 0; i < eventCount; ++i) {
//         // Retrieve event details from the event array
//         unsigned long eventTime = 50*events[i][0];  // Absolute event time in ms
//         int string = events[i][1];
//         int fret = events[i][2];

//         // Adjust for 0-indexed array
//         int stringIndex = string - 1;
//         int fretIndex = fret - 1;

//         // Process only if the current time has passed the event time
//         if (currentTime >= eventTime) {
//             if (fret != -1) {  // Note ON
//                 // If the string is not already in use, play the note
//                 if (!noteStates[stringIndex].isActive) {
//                     noteStates[stringIndex].isActive = true;
//                     noteStates[stringIndex].sustainEndTime = eventTime + 500;  // Sustain for 500ms (or desired duration)

//                     // Mark the fret and string as active
//                     lh_state[fretIndex] |= (1 << stringIndex);  // Set the bit for this string and fret
//                     int clkPin = fretPins[fretIndex][0];
//                     int dataPin = fretPins[fretIndex][1];
//                     int clearPin = fretPins[fretIndex][2];

//                     // Send the shift register update
//                     digitalWrite(clearPin, HIGH);
//                     shiftOut(dataPin, clkPin, LSBFIRST, lh_state[fretIndex]);

//                     // Pluck the string using servo control
//                     if (stringIndex == 0) servo1.move(0);
//                     if (stringIndex == 1) servo2.move(0);
//                     if (stringIndex == 2) servo3.move(0);
//                     if (stringIndex == 3) servo4.move(0);
//                     if (stringIndex == 4) servo5.move(0);
//                     if (stringIndex == 5) servo6.move(0);
//                 }

//             } else {  // Note OFF (fret == -1)
//                 // Deactivate the note if it's still active
//                 if (noteStates[stringIndex].isActive) {
//                     unsigned long eventEndTime = eventTime + 500;  // Sustain end time
//                     if (currentTime >= eventEndTime) {
//                         noteStates[stringIndex].isActive = false;
//                         noteStates[stringIndex].sustainEndTime = 0;

//                         // Turn off the specific string and fret
//                         for (int f = 0; f < NUM_FRETS; ++f) {
//                             if (lh_state[f] & (1 << stringIndex)) {
//                                 lh_state[f] &= ~(1 << stringIndex);  // Clear the bit for this string and fret
//                                 int clkPin = fretPins[f][0];
//                                 int dataPin = fretPins[f][1];
//                                 int clearPin = fretPins[f][2];

//                                 // Send the updated state to the shift register
//                                 digitalWrite(clearPin, HIGH);
//                                 shiftOut(dataPin, clkPin, LSBFIRST, lh_state[f]);
//                             }
//                         }
//                     }
//                 }
//             }

//             // Update last event time after processing the current event
//             lastUpdateTime = currentTime;
//         }
//     }
// }


unsigned long getCurrentTime() {
    return millis();
}