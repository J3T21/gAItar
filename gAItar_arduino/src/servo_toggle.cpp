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




PwmServoController::PwmServoController(int pwmPin, int posA, int posB, int minPulseMicros, int maxPulseMicros)
    : pin(pwmPin), positionA(posA), positionB(posB), currentPosition(posA), movingToB(true), 
      minPulse(minPulseMicros), maxPulse(maxPulseMicros) {

    pwmInstance = new SAMD_PWM(pin, 50.0f, 0.0f);  // Initialize with 50Hz frequency and 0% duty cycle
    pwmInstance->setPWM(pin, 50.0f, angleToDutyCycle(positionA));  // Set frequency to 50Hz, which is standard for servos

}

void PwmServoController::move(int delayMs) {
    int targetPosition = movingToB ? positionB : positionA;

    if (delayMs > 0) {
        // Gradual movement with delay to control speed
        int step = (targetPosition > currentPosition) ? 1 : -1;
        while (currentPosition != targetPosition) {
            currentPosition += step;
            int pwmValue = angleToDutyCycle(currentPosition);  // Get the corresponding PWM duty cycle
            pwmInstance->setPWM(pin, 50.0f, pwmValue);  // Set the PWM duty cycle
            delay(delayMs);            // Wait for the delay before the next step
        }
    } else {
        // Immediate movement without delay
        int pwmValue = angleToDutyCycle(targetPosition);  // Get the corresponding PWM duty cycle
        pwmInstance->setPWM(pin, 50.0f, pwmValue);  // Set the PWM duty cycle
        currentPosition = targetPosition;  // Update the current position
    }

    movingToB = !movingToB;  // Toggle the direction
}

float PwmServoController::angleToDutyCycle(int angle) {
    // Map the angle to the corresponding pulse width in microseconds
    float pulseWidthMicros = map(angle, 0, 300, 2500, 500);  // Map angle to pulse width (500µs to 2500µs)
    
    // Calculate the duty cycle: (pulse width / period) * 100
    float dutyCycle = (pulseWidthMicros / 20000.0) * 100;  // Period is 20000µs (50Hz PWM)

    // Return the duty cycle as a float (0.0% to 12.5%)
    return dutyCycle;
}