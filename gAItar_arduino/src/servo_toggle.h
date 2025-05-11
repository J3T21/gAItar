#ifndef SERVO_TOGGLE_H
#define SERVO_TOGGLE_H

#include <Servo.h> // Include the Servo library
#include <Arduino.h> // Include the Arduino library for basic functions
#include <SAMD_PWM.h>

class ServoController {
    private:
        Servo servo;         // Servo object from the Servo library
        int positionA;       // Initial position
        int positionB;       // Target position
        int positionDamper; // Damping position
        int currentPosition; // Current position of the servo
        bool movingToB;      // Flag to toggle between positions
    
    public:
        ServoController(int pin, int posA, int posB); // Constructor
        void move(int delayMs = 0); // Method to move the servo
        void damper();
};

class PwmServoController {
    private:
        int pin;              // PWM pin
        int positionA;        // Initial position (in degrees)
        int positionB;        // Target position (in degrees)
        int currentPosition;  // Current position of the servo (in degrees)
        bool movingToB;       // Flag to toggle between positions
        int minPulse;         // Minimum pulse width for the servo (in microseconds)
        int maxPulse;         // Maximum pulse width for the servo (in microseconds)
        SAMD_PWM* pwmInstance; // Instance of SAMD_PWM for controlling PWM

        // Helper function to convert angles (0-300°) to PWM duty cycle (0-100%)
        float angleToDutyCycle(int angle);
    public:
        // Constructor to initialize PWM pin, positionA, and positionB (in degrees)
        PwmServoController(int pin, int posA, int posB, int minPulseMicros = 500, int maxPulseMicros = 2500);
        // Method to move the servo gradually with a delay (mimics toggling between two positions)
        void move(int delayMs = 0);
};

#endif // SERVO_TOGGLE_H
