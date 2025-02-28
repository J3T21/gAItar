#include <Arduino.h>
#include <Servo.h>

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
    Serial.begin(9600); // Serial comm begin at 9600bps
    // Initialization handled in the ServoController constructor
}

void loop() {
    if (Serial.available()) // if serial value is available
    { 
    int val = Serial.read();// then read the serial value
    switch (val) {
        case '1':
            servo1.move(0);
            break;
        case '2':
            servo2.move(0);
            break;
        case '3':
            servo3.move(0);
            break;
        case '4':
            servo4.move(0);
            break;
        case '5':
            servo5.move(0);
            break;
        case '6':
            servo6.move(0);
            break;
        default:
            break;
    }
    //delay(100); // Delay to control the speed of the servo
    }
}