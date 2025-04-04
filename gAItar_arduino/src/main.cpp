#include <Arduino.h>
#include <Servo.h>

#define NUM_FRETS 9

#define string1 0b00000010 // Initialize string1
#define string2 0b00000001 // Initialize string2
#define string3 0b00001000 // Initialize string3
#define string4 0b00000100 // Initialize string4
#define string6 0b10000000 // Initialize string5
#define string5 0b01000000 // Initialize string6

#define clearPin1 24
#define clkPin1 22
#define dataPin1 26

#define clearPin2 30
#define clkPin2 28
#define dataPin2 32

#define clearPin3 36
#define clkPin3 34
#define dataPin3 38

#define clearPin4 42
#define clkPin4 40
#define dataPin4 44

#define clearPin5 48
#define clkPin5 50
#define dataPin5 46

#define clearPin6 25
#define clkPin6 23
#define dataPin6 27

#define clearPin7 31
#define clkPin7 29
#define dataPin7 33

#define clearPin8 37
#define clkPin8 35
#define dataPin8 39

#define clearPin9 43
#define clkPin9 41
#define dataPin9 45

const int fretPins[NUM_FRETS][3] = {
    {clkPin1, dataPin1, clearPin1},
    {clkPin2, dataPin2, clearPin2},
    {clkPin3, dataPin3, clearPin3},
    {clkPin4, dataPin4, clearPin4},
    {clkPin5, dataPin5, clearPin5},
    {clkPin6, dataPin6, clearPin6},
    {clkPin7, dataPin7, clearPin7},
    {clkPin8, dataPin8, clearPin8},
    {clkPin9, dataPin9, clearPin9} // Add more frets as needed need to fix fret 9 pushing away too much
}; 

const byte stringOrder[6] = {
    string1, // String 1
    string2, // String 2
    string3, // String 3
    string4, // String 4
    string5, // String 5
    string6 // String 6
};

void clearInactiveFrets(int fretActive) {
    for (int i = 0; i < NUM_FRETS; i++) {
        if (i != fretActive) {
            digitalWrite(fretPins[i][2], LOW); // Set clear pin low to disable the shift register
        }
    }
}

void testFret(int start_fret, int timeHold){
    for (int i = start_fret - 1 ; i < NUM_FRETS; i++){
        int clkPin = fretPins[i][0];
        int dataPin = fretPins[i][1];
        int clearPin = fretPins[i][2];
        clearInactiveFrets(i); // Clear inactive frets
        for (int string = 0; string < 6; string++) {  
            // Send the one-hot encoded value to the shift register
            digitalWrite(clearPin, HIGH); // Set clear pin high to enable the shift register
            shiftOut(dataPin, clkPin, MSBFIRST, stringOrder[string]);
            delay(timeHold); // delay 1000ms to observe the change
            shiftOut(dataPin, clkPin, MSBFIRST, 0);
            delay(timeHold); // delay 1000ms to observe the change    
        }
        shiftOut(dataPin, clkPin, MSBFIRST, 207);
        delay(timeHold); // delay 1000ms to observe the change
        shiftOut(dataPin, clkPin, MSBFIRST, 0);
        delay(timeHold); // delay 1000ms to observe the change
    }

}

class ServoController {
private:
    Servo servo;         // Servo object from the Servo library
    int positionA;       // Initial position
    int positionB;       // Target position
    int currentPosition; // Current position of the servo
    bool movingToB;      // Flag to toggle between positions

public:
    // Constructor to initialize positions and attach the servo
    ServoController(int pin, int posA, int posB) : positionA(posA), positionB(posB), currentPosition(posA), movingToB(true) {
        servo.attach(pin); // Attach the servo to the specified pin
        servo.write(positionA); // Set initial position
    }

    // Method to move the servo with a delay to control speed
    void move(int delayMs = 0) {
        int targetPosition = movingToB ? positionB : positionA;

        if (delayMs > 0) {
              servo.write(targetPosition);
            }
          else {
            servo.write(targetPosition);
            currentPosition = targetPosition;
        }

        movingToB = !movingToB; // Toggle the direction
    }
};


// Global instance of ServoController
ServoController servo1(14, 97, 111);
ServoController servo2(15, 107, 118);
ServoController servo3(16, 76, 89);
ServoController servo4(17, 100, 112);
ServoController servo5(18, 80, 91);
ServoController servo6(19, 95, 110);

void setup() {

    for (int i = 0; i < NUM_FRETS; i++)
    {
        pinMode(fretPins[i][0], OUTPUT); // Set clock pin as output
        pinMode(fretPins[i][1], OUTPUT); // Set data pin as output
        pinMode(fretPins[i][2], OUTPUT); // Set clear pin as output
        digitalWrite(fretPins[i][0], LOW); // Set clock pin LOW to start
        digitalWrite(fretPins[i][2], LOW); // Set clear pin LOW to clear the shift register initially
    }
    

}

void loop() {
    testFret(8, 1000);
}