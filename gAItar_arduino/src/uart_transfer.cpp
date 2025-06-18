#include "uart_transfer.h"
#include "translate.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include "globals.h"
#include <FreeRTOS_SAMD51.h>

// External global playback state variables
extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern char currentSongPath[128];
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;

// Command caching variables for resume functionality
char prevTitle[64] = "";
char prevArtist[64] = "";
char prevGenre[64] = "";

/**
 * File search implementation using hierarchical directory structure
 * Constructs paths in format: /genre/artist/title.bin
 * Thread-safe with semaphore protection for SD card access
 * 
 * @param title Song title for filename construction
 * @param artist Artist name for directory path
 * @param genre Genre category for top-level directory
 * @return Static buffer containing full file path or nullptr if not found
 */
const char* findFileSimple(const char* title, const char* artist, const char* genre) {
    static char matchingFilePath[128];
    
    // Construct the full file path: /genre/artist/title.bin
    snprintf(matchingFilePath, sizeof(matchingFilePath), "/%s/%s/%s.bin", 
             genre, artist, title);

    // Debug output for file search tracking
    Serial.print("Checking if file exists: ");
    Serial.println(matchingFilePath);

    // Thread-safe SD card access with semaphore protection
    if (!xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
        Serial.println("Failed to take SD semaphore");
        return nullptr;
    }

    // Check file existence on SD card
    bool fileExists = sd.exists(matchingFilePath);
    
    xSemaphoreGive(sdSemaphore);

    if (fileExists) {
        Serial.print("File found: ");
        Serial.println(matchingFilePath);
        return matchingFilePath;
    } else {
        Serial.print("File not found: ");
        Serial.println(matchingFilePath);
        return nullptr;
    }
}

/**
 * Recursive directory traversal for file listing over UART
 * Transmits file paths and handles nested directory structures
 * Filters out system directories and hidden files
 * 
 * @param dir Directory handle for traversal
 * @param uart UART interface for file list transmission
 * @param basePath Current directory path for recursive calls
 */
void listFilesRecursiveUart(SdFile& dir, Uart& uart, const char* basePath = "/") {
    SdFile entry;
    char name[64];
    
    while (entry.openNext(&dir, O_RDONLY)) {
        entry.getName(name, sizeof(name));
        if (entry.isDir()) {
            // Skip system and hidden directories
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && strcmp(name, "System Volume Information") != 0) {
                // Construct subdirectory path for recursive traversal
                char subDirPath[256];
                snprintf(subDirPath, sizeof(subDirPath), "%s%s/", basePath, name);
                listFilesRecursiveUart(entry, uart, subDirPath);
            }
        } else {
            // Transmit file information over UART
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s%s", basePath, name);
            
            uart.print(filePath);
            uart.print("\r\n");
            uart.flush();
            
            // Mirror output to Serial for debugging
            Serial.print(filePath);
            Serial.print("\r\n");
            Serial.flush();
        }
        entry.close();
    }
}

/**
 * UART file listing interface for remote file system browsing
 * Provides complete directory structure over UART for external systems
 * 
 * @param uart UART interface for file list transmission
 */
void listFilesOnSDUart(Uart& uart) {
    SdFile root;
    if (!root.open("/")) {
        return; // Silent failure - root directory access failed
    }
    listFilesRecursiveUart(root, uart, "/");
    root.close();
}

/**
 * Binary protocol instruction receiver with state machine implementation
 * Handles three-stage protocol: header detection, length parsing, payload processing
 * Supports play, pause, and list commands with JSON payload parsing
 * Thread-safe with semaphore protection for shared resources
 * 
 * Protocol format:
 * - Header: 0xAA (start byte)
 * - Length: 1 byte payload length
 * - Payload: Variable length command data
 * 
 * @param instrUart UART interface for command reception
 */
void instructionReceiverRTOS(Uart &instrUart) {
    // State machine states for reliable packet parsing
    static enum { WAIT_FOR_HEADER, WAIT_FOR_LENGTH, WAIT_FOR_PAYLOAD } state = WAIT_FOR_HEADER;
    static uint8_t length = 0;
    static uint8_t receivedBytes = 0;
    static uint8_t buffer[256];

    if (instrUart.available()) {
        uint8_t incomingByte = instrUart.read();

        switch (state) {
            case WAIT_FOR_HEADER:
                // Look for protocol start byte
                if (incomingByte == 0xAA) {
                    state = WAIT_FOR_LENGTH;
                    length = 0;
                    receivedBytes = 0;
                    Serial.println("Start byte received");
                }
                break;

            case WAIT_FOR_LENGTH:
                // Validate payload length
                if (incomingByte > 0 && incomingByte <= sizeof(buffer)) {
                    length = incomingByte;
                    receivedBytes = 0;
                    state = WAIT_FOR_PAYLOAD;
                    Serial.print("Payload length: ");
                    Serial.println(length);
                } else {
                    Serial.println("Invalid length, resetting");
                    state = WAIT_FOR_HEADER;
                }
                break;

            case WAIT_FOR_PAYLOAD:
                // Accumulate payload bytes
                buffer[receivedBytes++] = incomingByte;
                if (receivedBytes == length) {
                    // Complete command received - process based on command type
                    buffer[length] = '\0'; // Null-terminate for string operations
                    Serial.print("Received command: ");
                    Serial.println((char*)buffer);

                    // Handle List command (read-only SD operations)
                    if (strncmp((char*)buffer, "List", 4) == 0) {
                        Serial.println("Processing file list request");
                        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
                            listFilesOnSDUart(instructionUart);
                            xSemaphoreGive(sdSemaphore);
                        } else {
                            Serial.println("Failed to take sdSemaphore in instructionReceiverRTOS");
                        }
                    }
                    // Handle Play and Pause commands (require playback state synchronization)
                    else if (xSemaphoreTake(playbackSemaphore, portMAX_DELAY)) {
                        if (strncmp((char*)buffer, "[Play]", 6) == 0) {
                            // Extract JSON payload from play command
                            const char* jsonPart = (char*)buffer + 6;
                        
                            // Parse JSON metadata with memory-safe document
                            JsonDocument doc;
                            DeserializationError error = deserializeJson(doc, jsonPart);
                            if (error) {
                                Serial.println("Failed to parse command JSON");
                                Serial.print("Error: ");
                                Serial.println(error.c_str());
                                doc.clear();
                                state = WAIT_FOR_HEADER;
                                xSemaphoreGive(playbackSemaphore);
                                return;
                            }
                        
                            // Extract song metadata from JSON
                            const char* rawTitle = doc["title"];
                            const char* rawArtist = doc["artist"];
                            const char* rawGenre = doc["genre"];

                            // Check if this matches the previously requested song
                            bool isSameSong = (strcmp(rawTitle, prevTitle) == 0 &&
                                                strcmp(rawArtist, prevArtist) == 0 &&
                                                strcmp(rawGenre, prevGenre) == 0);

                            if (isSameSong && isPaused && !newSongRequested) {
                                // Resume previously paused song
                                startTime = millis() - pauseOffset;
                                isPlaying = true;
                                isPaused = false;
                                Serial.println("Resuming previous song (metadata matched)");
                            }
                            else {
                                // Handle new song request or song change
                                // Update metadata cache for future resume operations
                                strncpy(prevTitle, rawTitle, sizeof(prevTitle) - 1);
                                strncpy(prevArtist, rawArtist, sizeof(prevArtist) - 1);
                                strncpy(prevGenre, rawGenre, sizeof(prevGenre) - 1);
                                prevTitle[sizeof(prevTitle) - 1] = '\0';
                                prevArtist[sizeof(prevArtist) - 1] = '\0';
                                prevGenre[sizeof(prevGenre) - 1] = '\0';

                                // Locate song file using metadata
                                const char* filePath = findFileSimple(prevTitle, prevArtist, prevGenre);
                                if (filePath) {
                                    Serial.print("Found file: ");
                                    Serial.println(filePath);

                                    // Initialize new song playback state
                                    strncpy(currentSongPath, filePath, sizeof(currentSongPath) - 1);
                                    currentSongPath[sizeof(currentSongPath) - 1] = '\0';
                                    newSongRequested = true;
                                    isPlaying = true;
                                    isPaused = false;
                                    startTime = millis();
                                    pauseOffset = 0; // Reset pause state for new song

                                    Serial.println("Starting new song (interrupting current if any)");
                                } else {
                                    Serial.println("File not found with matching metadata");
                                }
                            }
                            doc.clear(); // Release JSON document memory
                        } else if (strncmp((char*)buffer, "Pause", 5) == 0) {
                            // Handle pause command - save current playback position
                            isPaused = true;
                            pauseOffset = millis() - startTime;
                            Serial.println("Paused: isPaused true");
                        } else {
                            Serial.println("Invalid command prefix");
                        }
                        
                        xSemaphoreGive(playbackSemaphore);
                    }

                    // Reset state machine for next message
                    state = WAIT_FOR_HEADER;
                }
                break;
        }
    }
}

/**
 * Recursive directory traversal for Serial output debugging
 * Displays complete file system structure with file sizes
 * Used for local debugging and system verification
 * 
 * @param dir Directory handle for traversal
 * @param path Current directory path string
 */
void listFilesRecursive(SdFile& dir, String path = "/") {
    SdFile entry;
    char name[64];

    while (entry.openNext(&dir, O_RDONLY)) {
        entry.getName(name, sizeof(name));
        if (entry.isDir()) {
            // Skip system directories for clean output
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                String subDirPath = path + name + "/";
                listFilesRecursive(entry, subDirPath);
            }
        } else {
            // Display file information with size
            Serial.print(path);
            Serial.print(name);
            Serial.print(" - ");
            Serial.print(entry.fileSize());
            Serial.println(" bytes");
        }
        entry.close();
    }
}

/**
 * Serial output file listing for local debugging
 * Provides complete directory structure to Serial monitor
 */
void listFilesOnSD() {
    SdFile root;
    if (!root.open("/")) {
        Serial.println("Failed to open root directory");
        return;
    }
    Serial.println("Files on SD card:");
    listFilesRecursive(root, "/");
    root.close();
}

/**
 * File transfer protocol state enumeration
 * Defines stages of binary file reception process
 */
enum ReceiveState{
    PARSE_HEADER,       // Waiting for transfer initiation
    OPEN_FILE,          // Creating file and directory structure
    PARSE_CHUNK_HEADER, // Processing chunk metadata
    READ_CHUNK,         // Receiving chunk data
    DONE                // Transfer completion
};

/**
 * Thread-safe directory creation with incremental path building
 * Creates nested directory structure as needed for file storage
 * Uses static buffers to avoid dynamic memory allocation
 * 
 * @param fullPath Complete file path including directory hierarchy
 * @return true if all directories created successfully, false on failure
 */
bool createDirectoriesRTOS_static(const char* fullPath) {
    // Extract directory path from full file path
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash || lastSlash == fullPath) {
        return true;  // No directories to create
    }

    // Copy directory path to static buffer for processing
    static char dirPath[128];
    size_t dirLen = lastSlash - fullPath;
    if (dirLen >= sizeof(dirPath)) {
        Serial.println("Directory path too long");
        return false;
    }
    
    strncpy(dirPath, fullPath, dirLen);
    dirPath[dirLen] = '\0';

    // Build directory path incrementally
    char tempPath[128] = "";
    char* token;
    char* dirPathCopy = dirPath;
    
    // Handle root directory prefix
    if (dirPathCopy[0] == '/') {
        dirPathCopy++;
        strcpy(tempPath, "/");
    }

    // Process each directory component
    char* saveptr;
    token = strtok_r(dirPathCopy, "/", &saveptr);
    
    while (token != NULL) {
        // Build incremental path for each directory level
        if (strlen(tempPath) > 1) {  // Avoid double slash after root
            strcat(tempPath, "/");
        }
        strcat(tempPath, token);

        // Create directory if it doesn't exist
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (!sd.exists(tempPath)) {
                if (!sd.mkdir(tempPath)) {
                    Serial.print("Failed to create: ");
                    Serial.println(tempPath);
                    xSemaphoreGive(sdSemaphore);
                    return false;
                }
            }
            xSemaphoreGive(sdSemaphore);
        } else {
            Serial.println("Failed to take sdSemaphore in createDirectoriesRTOS_static");
            return false;
        }

        token = strtok_r(NULL, "/", &saveptr);
    }

    return true;
}

/**
 * Binary file receiver with chunked protocol implementation
 * Implements reliable file transfer with acknowledgment and retry mechanisms
 * Handles large files through chunked transmission with timeout protection
 * 
 * Protocol stages:
 * 1. Header parsing: START:<filepath>:<size>
 * 2. File creation with directory structure
 * 3. Chunk reception: CHUNK:<id>:<size> followed by binary data
 * 4. Completion with file closure and cleanup
 * 
 * @param fileUart UART interface for file data reception
 */
void fileReceiverRTOS_char(Uart &fileUart){
    // State machine variables for transfer management
    static ReceiveState state = PARSE_HEADER;
    static size_t fileSize = 0;
    static char filePath[128] = "";
    static size_t chunkSize = 0;
    static uint8_t buffer[128];
    static uint16_t chunkId = 0;
    static size_t receivedBytes = 0;
    static File file;
    static unsigned long lastByteTime = 0;
    static const unsigned long TIMEOUT = 5000; // 5 second timeout
    
    // Chunk accumulation counter (function scope for proper state management)
    static size_t bytesAccumulated = 0;

    // State reset helper function for error recovery
    auto resetState = [&]() {
        // Clean up file handle with thread safety
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (file) {
                file.close();
            }
            xSemaphoreGive(sdSemaphore);
        }
        // Clear UART buffer of any remaining data
        while (fileUart.available()) {
            fileUart.read();
        }
        // Reset all state variables
        receivedBytes = 0;
        fileSize = 0;
        chunkId = 0;
        filePath[0] = '\0';
        chunkSize = 0;
        bytesAccumulated = 0;
        state = PARSE_HEADER;
    };

    // Timeout protection for stalled transfers
    if (state != PARSE_HEADER && millis() - lastByteTime > TIMEOUT) {
        Serial.println("Transfer timeout");
        fileUart.println("ERROR:TIMEOUT");
        while (fileUart.available()) {
            fileUart.read(); // Clear buffer
        }
        resetState();
        return;
    }

    switch (state){
        case PARSE_HEADER:
            // Parse transfer initiation header
            if (fileUart.available()){
                char headerBuffer[256];
                int headerLen = fileUart.readBytesUntil('\n', headerBuffer, sizeof(headerBuffer) - 1);
                if (headerLen <= 0) break;
                
                headerBuffer[headerLen] = '\0';
                
                // Clean up trailing whitespace
                while (headerLen > 0 && (headerBuffer[headerLen-1] == '\r' || headerBuffer[headerLen-1] == ' ')) {
                    headerBuffer[--headerLen] = '\0';
                }
                
                lastByteTime = millis();
                
                if (strncmp(headerBuffer, "START:", 6) == 0){
                    // Parse START:<filepath>:<size> format
                    char* firstColon = strchr(headerBuffer + 6, ':');
                    char* secondColon = firstColon ? strchr(firstColon + 1, ':') : nullptr;
                    
                    if (firstColon && secondColon){
                        // Extract file path component
                        size_t pathLen = firstColon - (headerBuffer + 6);
                        if (pathLen < sizeof(filePath)) {
                            strncpy(filePath, headerBuffer + 6, pathLen);
                            filePath[pathLen] = '\0';
                        }
                        
                        // Extract and validate file size
                        fileSize = strtoul(secondColon + 1, NULL, 10);
                        
                        if (fileSize > 0 && fileSize < 10485760) { // 10MB limit
                            Serial.printf("Transfer start: %s (%u bytes)\n", filePath, fileSize);
                            fileUart.printf("ACK:START:SIZE:%u\n", fileSize);
                            receivedBytes = 0;
                            chunkId = 0;
                            bytesAccumulated = 0;
                            state = OPEN_FILE;
                        } else {
                            fileUart.println("ERROR:INVALID_SIZE");
                        }
                    } else {
                        fileUart.println("ERROR:INVALID_HEADER");
                    }
                }
            }
            break;

        case OPEN_FILE:
            // Create directory structure and open file for writing
            if (!createDirectoriesRTOS_static(filePath)){
                resetState();
                return;
            }
            
            if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                file = sd.open(filePath, FILE_WRITE);
                if(file){
                    lastByteTime = millis();
                    state = PARSE_CHUNK_HEADER;
                } else {
                    fileUart.println("ERROR:FILE_OPEN_FAILED");
                    resetState();
                }
                xSemaphoreGive(sdSemaphore);
            } else {
                resetState();
            }
            break;

        case PARSE_CHUNK_HEADER:
            // Parse chunk metadata: CHUNK:<id>:<size>
            if (fileUart.available()){
                lastByteTime = millis();
                
                char chunkHeaderBuffer[64];
                int chunkHeaderLen = fileUart.readBytesUntil('\n', chunkHeaderBuffer, sizeof(chunkHeaderBuffer) - 1);
                if (chunkHeaderLen <= 0) break;
                
                chunkHeaderBuffer[chunkHeaderLen] = '\0';
                
                // Clean up trailing whitespace
                while (chunkHeaderLen > 0 && (chunkHeaderBuffer[chunkHeaderLen-1] == '\r' || chunkHeaderBuffer[chunkHeaderLen-1] == ' ')) {
                    chunkHeaderBuffer[--chunkHeaderLen] = '\0';
                }
                
                // Handle header retransmission (restart scenario)
                if (strncmp(chunkHeaderBuffer, "START:", 6) == 0) {
                    state = PARSE_HEADER;
                    break;
                }
                
                if (strncmp(chunkHeaderBuffer, "CHUNK:", 6) == 0){
                    // Parse CHUNK:<id>:<size> format
                    char* firstColon = strchr(chunkHeaderBuffer + 6, ':');
                    char* secondColon = firstColon ? strchr(firstColon + 1, ':') : nullptr;
                    
                    if (firstColon && secondColon){
                        uint16_t receivedId = strtoul(chunkHeaderBuffer + 6, NULL, 10);
                        chunkSize = strtoul(secondColon + 1, NULL, 10);
                        
                        if (receivedId == chunkId && chunkSize > 0 && chunkSize <= 128){
                            // Expected chunk received
                            fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                            bytesAccumulated = 0;
                            state = READ_CHUNK;
                        } else if (receivedId < chunkId) {
                            // Duplicate chunk - acknowledge previous
                            fileUart.printf("ACK:CHUNK:%u\n", receivedId);
                        } 
                    }
                }
            }
            break;

        case READ_CHUNK:
            // Receive chunk data bytes
            while (fileUart.available() && bytesAccumulated < chunkSize){
                buffer[bytesAccumulated++] = fileUart.read();
                lastByteTime = millis();
            }
            
            if (bytesAccumulated >= chunkSize){
                // Complete chunk received - write to file
                bool writeSuccess = false;
                
                if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                    size_t written = file.write(buffer, chunkSize);
                    file.flush(); // Ensure data is written to SD card
                    xSemaphoreGive(sdSemaphore);
                    
                    writeSuccess = (written == chunkSize);
                }
                
                if (writeSuccess) {
                    // Successful write - acknowledge and advance
                    fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                    chunkId++;
                    bytesAccumulated = 0;
                    receivedBytes += chunkSize;
                    
                    if (receivedBytes >= fileSize){
                        state = DONE; // Transfer complete
                    } else {
                        state = PARSE_CHUNK_HEADER; // Continue with next chunk
                    }
                } else {
                    // Write failure - reset and report error
                    bytesAccumulated = 0;
                    fileUart.println("ERROR:WRITE_FAILED");
                    resetState();
                }
            }
            break;

        case DONE:
            // Transfer completion - cleanup and reset
            if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                if (file) file.close();
                xSemaphoreGive(sdSemaphore);
            }
            
            Serial.printf("Transfer complete: %s (%u bytes)\n", filePath, receivedBytes);
            
            resetState();
            break;
    }   
}