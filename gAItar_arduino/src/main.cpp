#include <Arduino.h>
#include <Servo.h>
#include <iostream>
#include <vector>
#include <tuple>
#include <SPI.h>
#include <SdFat.h>
#include <FreeRTOS_SAMD51.h>
#include "servo_toggle.h"
#include "pwm_shift_solenoid.h"
#include "shift_solenoid.h"
#include "test_funcs.h"
#include "globals.h"
#include "translate.h"
#include "uart_transfer.h"

volatile bool isPlaying = false;
volatile bool isPaused = false;
volatile bool newSongRequested = false;
String currentSongPath = "";
size_t currentEventIndex = 0;
unsigned long startTime = 0;
unsigned long pauseOffset = 0;
SemaphoreHandle_t playbackSemaphore;

TaskHandle_t instructionTaskHandle;
TaskHandle_t playbackTaskHandle;
TaskHandle_t heapTaskHandle;

void instructionTask(void *pvParameters) {
    Serial.println("Instruction task");
    while (true) {
        instructionReceiverJson(instructionUart); // Call the instruction receiver function to handle incoming instructions
        vTaskDelay(10 / portTICK_PERIOD_MS); // Delay to prevent task starvation
        // Serial.print("InstrTask stack left: ");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    }
}

void heapMonitorTask(void *pvParameters) {
    while (true) {
        Serial.print("Free heap: ");
        Serial.println(xPortGetFreeHeapSize());
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Print every 1 second
    }
}

void playbackTask(void *pvParameters) {
    Serial.println("Playback task started");
    for (;;){
        if(xSemaphoreTake(playbackSemaphore, portMAX_DELAY)){
            playGuitarRTOS(currentSongPath.c_str());
            xSemaphoreGive(playbackSemaphore);
        }
        // Serial.print("PlaybackTask stack left: ");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

}

void testTask(void *pvParameters) {
    while (1) {
        Serial.println("Test task alive!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// void setup() {
//     Serial.begin(115200);
//     unsigned long serialTimeout = millis();
//     while (!Serial && millis() - serialTimeout < 2000) { ; }
//     xTaskCreate(testTask, "Test", 1024, NULL, 1, NULL);
//     vTaskStartScheduler();
// }

void setup() {
    Serial.begin(115200); // Initialize serial communication or debugging
    dataUart.begin(BAUDRATE); // Initialize dataUart for UART communication
    instructionUart.begin(BAUDRATE); //intialize instructionUart for UART communication
    while (!Serial) {
        ; // Wait for serial port to connect. Needed for native USB port only
    }
    for (int i = 0; i < NUM_FRETS; i++)
    {
        pinMode(fretPins[i][0], OUTPUT); // Set clock pin as output
        pinMode(fretPins[i][1], OUTPUT); // Set data pin as output
        pinMode(fretPins[i][2], OUTPUT); // Set clear pin as output
        digitalWrite(fretPins[i][0], LOW); // Set clock pin LOW to start
        digitalWrite(fretPins[i][2], LOW); // Set clear pin LOW to clear the shift register initially
    }
    if (!sd.begin(83, SD_SCK_MHZ(25))) {
        Serial.println("SD card initialization failed!"); // SD card initialization failed
        return;
    } else {
        Serial.println("SD card initialized successfully!"); // SD card initialized successfully
    }

    listFilesOnSD();

    delay(100);

    playbackSemaphore = xSemaphoreCreateMutex();
    if (playbackSemaphore == NULL) {
        Serial.println("Failed to create playbackSemaphore!");
        while (1); // Halt if semaphore creation fails
    }

    BaseType_t result = xTaskCreate(
        instructionTask, // Function to implement the task
        "Instruction Task", // Name of the task
        1024, // Stack size in words
        NULL, // Task input parameter
        2, // Priority of the task
        &instructionTaskHandle); // Task handle

    if (result != pdPASS){
        Serial.println("Instr task failed to create");
    }
    result = xTaskCreate(
        playbackTask, // Function to implement the task
        "Playback Task", // Name of the task
        2048, // Stack size in words
        NULL, // Task input parameter
        1, // Priority of the task
        &playbackTaskHandle); // Task handle
        if (result != pdPASS){
        Serial.println("Playback task failed to create");
    }
    //xTaskCreate(testTask, "Test", 1024, NULL, 1, NULL);
    // result = xTaskCreate(
    //     heapMonitorTask,
    //     "Heap Monitor",
    //     512,
    //     NULL,
    //     3, // Lowest priority
    //     &heapTaskHandle
    // );   
    // // if (result != pdPASS){
    // //     Serial.println("Instr task failed to create");
    // // }
    // Serial.println(xPortGetFreeHeapSize());
    vTaskStartScheduler();

}

void loop() {
//   fileReceiver_state(dataUart); // Call the file receiver function to handle incoming data
//   instructionReceiverJson(instructionUart); // Call the instruction receiver function to handle incoming instructions
  //testSerialControlservo();
  //testFret(1, 500, 10); // Test the fret function with a range of frets
  //testSerialControlservo();

  //playGuitarEventsOpen();
  //playGuitarFromFile("/Ragtime/Joplin/Entertainer.json");
//   if (isPlaying_glob){
//     playGuitarCommand();
//   }
}