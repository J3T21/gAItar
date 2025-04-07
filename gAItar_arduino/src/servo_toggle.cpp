#include "servo_toggle.h"

// Constructor to initialize positions and attach the servo
ServoController::ServoController(int pin, int posA, int posB)
    : positionA(posA), positionB(posB), currentPosition(posA), movingToB(true) {
    servo.attach(pin); // Attach the servo to the specified pin
    servo.write(positionA); // Set initial position
}

// Method to move the servo gradually with a delay
void ServoController::move(int delayMs) {
    int targetPosition = movingToB ? positionB : positionA;

    if (delayMs > 0) {
        // Gradual movement with delay to control speed
        int step = (targetPosition > currentPosition) ? 1 : -1;
        while (currentPosition != targetPosition) {
            currentPosition += step;
            servo.write(currentPosition); // Update the servo position
            delay(delayMs);               // Wait for the delay before next move
        }
    } else {
        // Immediate movement without delay
        servo.write(targetPosition);
        currentPosition = targetPosition;
    }

    movingToB = !movingToB; // Toggle the direction
}


