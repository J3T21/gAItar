#include "test_funcs.h"

void testFret(int start_fret, int timeHold, int stop_Fret){
    for (int i = start_fret - 1 ; i < stop_Fret; i++){
        int clkPin = fretPins[i][0];
        int dataPin = fretPins[i][1];
        int clearPin = fretPins[i][2];
        clearInactiveFrets(i); // Clear inactive frets
        for (int string = 0; string < 6; string++) {  
            // Send the one-hot encoded value to the shift register
            digitalWrite(clearPin, HIGH); // Set clear pin high to enable the shift register
            shiftOut(dataPin, clkPin, LSBFIRST, stringOrder[string]);
            delay(timeHold); // delay 1000ms to observe the change
            shiftOut(dataPin, clkPin, LSBFIRST, 0);
            delay(timeHold); // delay 1000ms to observe the change   
            clearInactiveFrets(i); // Clear inactive frets 
        }
        shiftOut(dataPin, clkPin, LSBFIRST, 255);
        delay(timeHold); // delay 1000ms to observe the change
        shiftOut(dataPin, clkPin, LSBFIRST, 0);
        delay(timeHold); // delay 1000ms to observe the change
    }

}


void testCombined(int start_fret, int timeHold){
    // make array of servo objects
    ServoController servos[] = {servo1, servo2, servo3, servo4, servo5, servo6};
    for (int i = start_fret - 1 ; i < NUM_FRETS; i++){
        int clkPin = fretPins[i][0];
        int dataPin = fretPins[i][1];
        int clearPin = fretPins[i][2];
        clearInactiveFrets(i); // Clear inactive frets
        for (int string = 0; string < 6; string++) {  
            // Send the one-hot encoded value to the shift register
            digitalWrite(clearPin, HIGH); // Set clear pin high to enable the shift register
            servos[string].move(0); // Move the servo to the target position
            shiftOut(dataPin, clkPin, LSBFIRST, stringOrder[string]);
            delay(timeHold); 
            shiftOut(dataPin, clkPin, LSBFIRST, 0);
            delay(timeHold);   
            clearInactiveFrets(i); // Clear inactive frets 
        }
        for (int k = 0; k < 6; k++) {
            servos[k].move(0); // strum
        }
        shiftOut(dataPin, clkPin, MSBFIRST, 0b11111111); // Send all strings to the shift register
        delay(timeHold); 
        shiftOut(dataPin, clkPin, MSBFIRST, 0);
        delay(timeHold); 
    }
}

    void testSerialControlservo() {
        if (Serial.available()) { // if serial value is available
            int val = Serial.read(); // then read the serial value
            switch (val) {
                case '1':
                    servo1.move(0);
                    delay(100);
                    break;
                case '2':
                    servo2.move(0);
                    delay(100);
                    break;
                case '3':
                    servo3.move(0);
                    delay(100);
                    break;
                case '4':
                    servo4.move(0);
                    delay(100);
                    break;
                case '5':
                    servo5.move(0);
                    delay(100);
                    break;
                case '6':
                    servo6.move(0);
                    delay(100);
                    break;
                default:
                    break;
            }
        }
        
}

void testSerialControlsolenoid(){
    if (Serial.available()){
        String input = "";
        while (Serial.available() > 0) {
            char c = Serial.read();
            input += c;
            if (input.length() == 3) {
                break;
            }
        }
        int stringIndex = input[0] - '0';
        int fretIndex = input[1] - '0'; 
        char pwmType = input[2]; // Get the PWM type character
        int clearPin = fretPins[fretIndex-1][2];
        clearInactiveFrets(fretIndex);  // Clear inactive frets
        digitalWrite(clearPin, HIGH);  // Enable shift register
        if (pwmType == 'r') {
            pwmRampPress(fretIndex, stringIndex, 255, 1000, 1000); // Call the function to test the frets with PWM
        } else if (pwmType == 'l') {
            pwmLogRampPress(fretIndex, stringIndex, 255, 1000, 1000); // Call the function to test the frets with PWM log
        } else if (pwmType == 's') {
            pwmSineRampPress(fretIndex, stringIndex, 255, 1000, 1000); // Call the function to test the frets with PWM sine
        } else if (pwmType == 'i') {
            instantPress(fretIndex, stringIndex, 1000); // Call the function to test the frets with PWM sine
        } else {
            Serial.println("Invalid input. Please enter a valid command.");
        }
    }
}

void testFretPWM(int start_fret, int timeHold, int stop_Fret) {
    for (int i = start_fret - 1; i < stop_Fret; i++) {
        int clkPin = fretPins[i][0];
        int dataPin = fretPins[i][1];
        int clearPin = fretPins[i][2];

        clearInactiveFrets(i); // Clear other frets

        for (int string = 0; string < 6; string++) {
            byte stringBit = stringOrder[string];
            byte newFretState = stringBit;
            byte softStartMask = 0x00;  // Since all other strings are off here

            int stateIndex = i * 6 + string;
            SoftStartState& state = softStartStates[stateIndex];

            // Reset the state before starting a new soft start
            state.ramping = false;

            unsigned long start = millis();
            while (millis() - start < timeHold) {
                softStart(clkPin, dataPin, clearPin, newFretState, softStartMask, state);
                delayMicroseconds(500); // Polling period for softStart (adjustable)
            }

            // Now turn it off
            shiftOut(dataPin, clkPin, LSBFIRST, 0);
            delay(timeHold);

            clearInactiveFrets(i);
        }

        // All bits ON test
        byte allOn = 0xFF;
        byte mask = 0x00;
        int dummyIndex = i * 6 + 0;  // Any unused slot
        SoftStartState& allOnState = softStartStates[dummyIndex];
        allOnState.ramping = false;

        unsigned long start = millis();
        while (millis() - start < timeHold) {
            softStart(clkPin, dataPin, clearPin, allOn, mask, allOnState);
            delayMicroseconds(500);
        }

        shiftOut(dataPin, clkPin, LSBFIRST, 0);
        delay(timeHold);
    }
}
