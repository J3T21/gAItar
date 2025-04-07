#ifndef SERVO_TOGGLE_H
#define SERVO_TOGGLE_H

#include <Servo.h> // Include the Servo library
#include <Arduino.h> // Include the Arduino library for basic functions

class ServoController {
    private:
        Servo servo;         // Servo object from the Servo library
        int positionA;       // Initial position
        int positionB;       // Target position
        int currentPosition; // Current position of the servo
        bool movingToB;      // Flag to toggle between positions
    
    public:
        ServoController(int pin, int posA, int posB); // Constructor
        void move(int delayMs = 0); // Method to move the servo
};



#endif // SERVO_TOGGLE_H
