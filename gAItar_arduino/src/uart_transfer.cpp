#include "uart.h"
#include <SD.h>
#include <SPI.h>

void fileReceiver(){
    static size_t fileSize = 0;
    static size_t receivedBytes = 0;
    static File sdFile; // Declare file variable here
    static String fileName = "";
    static bool receivingFile = false; // Flag to indicate if a file is being received

    while (Serial1.available()){
        if (!receivingFile) {
            String header = Serial1.readStringUntil('\n'); // Read the header line until newline
            Serial.println(header); // Print the header for debugging
            if (header.startsWith("START:")){
                int startIdx = header.indexOf("START:");
                int sizeIdx = header.indexOf("SIZE:");
                if (startIdx != -1 && sizeIdx != -1){
                    fileName = header.substring(startIdx + 6, sizeIdx); // Extract file name
                    fileSize = header.substring(sizeIdx + 5).toInt(); // Extract file size
                    receivedBytes = 0; // Reset received bytes count
                    receivingFile = true; // Set receiving flag to true
                    sdFile = SD.open(fileName, FILE_WRITE); // Open file for writing
                    if (!sdFile) {
                        Serial.println("Failed to open file for writing");
                        receivingFile = false; // Reset receiving flag on failure
                    }
                }
            }
        } else {
            // Receiving file data
            char buffer[64]; // Buffer to hold incoming data
            size_t bytesRead = Serial1.readBytes(buffer, sizeof(buffer)); // Read data into buffer
            sdFile.write(buffer, bytesRead); // Write data to SD card file
            receivedBytes += bytesRead; // Update received bytes count

            if (receivedBytes >= fileSize) { // Check if the entire file has been received
                sdFile.close(); // Close the file after writing
                Serial.println("File transfer complete");
                receivingFile = false; // Reset receiving flag
            }
        }
    }

}

void listFilesOnSD() {
    File root = SD.open("/"); // Open the root directory
    Serial.println("Files on SD card:");

    while (true) {
        File entry = root.openNextFile(); // Get the next file in the directory
        if (!entry) {
            break; // No more files, exit loop
        }
        Serial.print(entry.name()); // Print file name
        Serial.print(" - ");
        Serial.print(entry.size()); // Print file size
        Serial.println(" bytes");
        entry.close(); // Close the file entry
    }
}