#include "pwm_shift_solenoid.h"

void pwmRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime) {
    int clk = fretPins[fretIndex - 1][0];
    int data = fretPins[fretIndex - 1][1];

    int rampDelay = rampTime / 255;
    byte pattern = stringOrder[stringIndex - 1];
    for (int frame = 0; frame < 255; frame++) {
        int brightness = map(frame, 0, 255, 0, targetBrightness);
        brightness = constrain(brightness, 0, 255);

        
        shiftLSB(data, clk, pattern);  // Replaced shiftOut with shiftLSB

        delay(rampDelay);
    }
    shiftLSB(data, clk, pattern);  // Set the final brightness
    delay(holdMs);
}
void pwmLogRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime) {
    int clk = fretPins[fretIndex - 1][0];
    int data = fretPins[fretIndex - 1][1];

    int rampDelay = rampTime / 255;
    byte pattern = stringOrder[stringIndex - 1];
    for (int frame = 0; frame < 255; frame++) {
        float logBrightness = log(frame + 1) / log(256.0);  // Ensure float division
        int brightness = map(logBrightness * targetBrightness, 0, 255, 0, 255);
        brightness = constrain(brightness, 0, 255);

        
        shiftLSB(data, clk, pattern);  // Replaced shiftOut with shiftLSB

        delay(rampDelay);
    }
    shiftLSB(data, clk, pattern);
    delay(holdMs);
}
void pwmSineRampPress(int fretIndex, int stringIndex, int targetBrightness, int holdMs, float rampTime) {
    int clk = fretPins[fretIndex - 1][0];
    int data = fretPins[fretIndex - 1][1];

    int rampDelay = rampTime / 255;
    byte pattern = stringOrder[stringIndex - 1];
    for (int frame = 0; frame < 255; frame++) {
        float sineBrightness = (sin(PI * frame / 255.0 - PI / 2) + 1) / 2;
        int brightness = map(sineBrightness * targetBrightness, 0, 255, 0, 255);
        brightness = constrain(brightness, 0, 255);

        
        shiftLSB(data, clk, pattern);  // Replaced shiftOut with shiftLSB

        delay(rampDelay);
    }
    shiftLSB(data, clk, pattern);
    delay(holdMs);
}

void softStart(int clkPin, int dataPin, int clearPin, byte fretState, byte softStartMask, SoftStartState &state) {
    const unsigned long rampDuration = 1000; // in milliseconds
    const unsigned long pwmPeriod = 5000;    // in microseconds

    unsigned long nowMicros = micros();
    unsigned long nowMillis = millis();

    if (!state.ramping) {
        state.startTime = nowMillis;
        state.lastToggleTime = nowMicros;
        state.pwmOn = false;
        state.ramping = true;
    }

    unsigned long elapsed = nowMillis - state.startTime;

    // Done ramping — full ON
    if (elapsed >= rampDuration) {
        digitalWrite(clearPin, HIGH);
        shiftOut(dataPin, clkPin, LSBFIRST, fretState);
        state.ramping = false;
        return;
    }

    // Calculate duty cycle based on progress
    float progress = float(elapsed) / rampDuration;
    int dutyCycle = int(progress * 100);
    if (dutyCycle > 100) dutyCycle = 100;

    unsigned long onTime = (pwmPeriod * dutyCycle) / 100;
    unsigned long offTime = pwmPeriod - onTime;

    // PWM timing
    if (state.pwmOn && (nowMicros - state.lastToggleTime >= onTime)) {
        byte maskedState = fretState & ~softStartMask;
        digitalWrite(clearPin, HIGH);
        shiftOut(dataPin, clkPin, LSBFIRST, maskedState);
        state.pwmOn = false;
        state.lastToggleTime = nowMicros;
    }
    else if (!state.pwmOn && (nowMicros - state.lastToggleTime >= offTime)) {
        digitalWrite(clearPin, HIGH);
        shiftOut(dataPin, clkPin, LSBFIRST, fretState);
        state.pwmOn = true;
        state.lastToggleTime = nowMicros;
    }
}
