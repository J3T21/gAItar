#include "uart.h"
#include "translate.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include "globals.h"
#include <FreeRTOS_SAMD51.h>

extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern char currentSongPath[128];
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;

char prevTitle[64] = "";
char prevArtist[64] = "";
char prevGenre[64] = "";


const char* findFileSimple(const char* title, const char* artist, const char* genre) {
    static char matchingFilePath[128];
    
    // Construct the full file path: /genre/artist/title.json
    snprintf(matchingFilePath, sizeof(matchingFilePath), "/%s/%s/%s.json", 
             genre, artist, title);  // ← Remove .c_str() calls

    // Debug output
    Serial.print("Checking if file exists: ");
    Serial.println(matchingFilePath);

    // Acquire semaphore before accessing SD
    if (!xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
        Serial.println("Failed to take SD semaphore");
        return nullptr;
    }

    // Check if the file exists
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


void listFilesRecursiveUart(SdFile& dir, Uart& uart, const char* basePath = "/") {
    SdFile entry;
    char name[64];
    
    while (entry.openNext(&dir, O_RDONLY)) {
        entry.getName(name, sizeof(name));
        if (entry.isDir()) {
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && strcmp(name, "System Volume Information") != 0) {
                // Create local buffer for subdirectory path
                char subDirPath[256];
                snprintf(subDirPath, sizeof(subDirPath), "%s%s/", basePath, name);
                listFilesRecursiveUart(entry, uart, subDirPath);
            }
        } else {
            // Create local buffer for file path
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s%s", basePath, name);
            
            uart.print(filePath);
            uart.print("\r\n");
            uart.flush();
            
            Serial.print(filePath);
            Serial.print("\r\n");
            Serial.flush();
        }
        entry.close();
    }
}

void listFilesOnSDUart(Uart& uart) {
    SdFile root;
    if (!root.open("/")) {
       // uart.println("Failed to open root directory");
        return;
    }
    listFilesRecursiveUart(root, uart, "/");
    root.close();
}

void instructionReceiverRTOS(Uart &instrUart) {
    static enum { WAIT_FOR_HEADER, WAIT_FOR_LENGTH, WAIT_FOR_PAYLOAD } state = WAIT_FOR_HEADER;
    static uint8_t length = 0;
    static uint8_t receivedBytes = 0;
    static uint8_t buffer[256];

    if (instrUart.available()) {
        uint8_t incomingByte = instrUart.read();

        switch (state) {
            case WAIT_FOR_HEADER:
                if (incomingByte == 0xAA) {
                    state = WAIT_FOR_LENGTH;
                    length = 0;
                    receivedBytes = 0;
                    Serial.println("Start byte received");
                }
                break;

            case WAIT_FOR_LENGTH:
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
                buffer[receivedBytes++] = incomingByte;
                if (receivedBytes == length) {
                    // Full command received
                    buffer[length] = '\0'; // Null-terminate the buffer
                    Serial.print("Received command: ");
                    Serial.println((char*)buffer);

                    // Handle List command (no semaphore needed for SD read-only operations)
                    if (strncmp((char*)buffer, "List", 4) == 0) {
                        // List files on SD card
                        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
                            listFilesOnSDUart(instructionUart);
                            xSemaphoreGive(sdSemaphore);
                        } else {
                            Serial.println("Failed to take sdSemaphore in instructionReceiverRTOS");
                        }
                    }
                    // Handle Play and Pause commands (need playback semaphore)
                    else if (xSemaphoreTake(playbackSemaphore, portMAX_DELAY)) {
                        if (strncmp((char*)buffer, "[Play]", 6) == 0) {
                            const char* jsonPart = (char*)buffer + 6;
                        
                            // Parse JSON with StaticJsonDocument for memory safety
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
                        
                            // Extract raw strings from JSON (no String objects yet)
                            const char* rawTitle = doc["title"];
                            const char* rawArtist = doc["artist"];
                            const char* rawGenre = doc["genre"];

                            bool isSameSong = (strcmp(rawTitle, prevTitle) == 0 &&
                                                strcmp(rawArtist, prevArtist) == 0 &&
                                                strcmp(rawGenre, prevGenre) == 0);


                            if (isSameSong && isPaused && !newSongRequested) {
                                // Resume the same paused song
                                startTime = millis() - pauseOffset;
                                isPlaying = true;
                                isPaused = false;
                                Serial.println("Resuming previous song (metadata matched)");
                            } else {
                                // New song (even if same metadata) or different song
                                // Update cached metadata
                                strncpy(prevTitle, rawTitle, sizeof(prevTitle) - 1);
                                strncpy(prevArtist, rawArtist, sizeof(prevArtist) - 1);
                                strncpy(prevGenre, rawGenre, sizeof(prevGenre) - 1);
                                prevTitle[sizeof(prevTitle) - 1] = '\0';
                                prevArtist[sizeof(prevArtist) - 1] = '\0';
                                prevGenre[sizeof(prevGenre) - 1] = '\0';

                                const char* filePath = findFileSimple(prevTitle, prevArtist, prevGenre);
                                if (filePath) {
                                    Serial.print("Found file: ");
                                    Serial.println(filePath);

                                    // Force new song regardless of current state
                                    strncpy(currentSongPath, filePath, sizeof(currentSongPath) - 1);
                                    currentSongPath[sizeof(currentSongPath) - 1] = '\0';
                                    newSongRequested = true;
                                    isPlaying = true;
                                    isPaused = false;
                                    startTime = millis();
                                    pauseOffset = 0; // Reset pause offset for new song

                                    Serial.println("Starting new song (interrupting current if any)");
                                } else {
                                    Serial.println("File not found with matching metadata");
                                }
                            }
                            doc.clear();
                        } else if (strncmp((char*)buffer, "Pause", 5) == 0) {
                            isPaused = true;
                            pauseOffset = millis() - startTime;
                            Serial.println("Paused: isPaused true");
                        } else {
                            Serial.println("Invalid command prefix");
                        }
                        
                        xSemaphoreGive(playbackSemaphore);
                    }

                    // Reset for next message
                    state = WAIT_FOR_HEADER;
                }
                break;
        }
    }
}




void listFilesRecursive(SdFile& dir, String path = "/") {
    SdFile entry;
    char name[64];

    while (entry.openNext(&dir, O_RDONLY)) {
        entry.getName(name, sizeof(name));
        if (entry.isDir()) {
            // Skip "." and ".." entries
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                String subDirPath = path + name + "/";
                listFilesRecursive(entry, subDirPath);
            }
        } else {
            Serial.print(path);
            Serial.print(name);
            Serial.print(" - ");
            Serial.print(entry.fileSize());
            Serial.println(" bytes");
        }
        entry.close();
    }
}

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



enum ReceiveState{
    PARSE_HEADER,
    OPEN_FILE,
    PARSE_CHUNK_HEADER,
    READ_CHUNK,
    DONE
};

bool createDirectoriesRTOS_static(const char* fullPath) {
    // Find the last slash to separate directory from filename
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash || lastSlash == fullPath) {
        return true;  // No directories to create
    }

    // Copy directory path to a static buffer
    static char dirPath[128];
    size_t dirLen = lastSlash - fullPath;
    if (dirLen >= sizeof(dirPath)) {
        Serial.println("Directory path too long");
        return false;
    }
    
    strncpy(dirPath, fullPath, dirLen);
    dirPath[dirLen] = '\0';

    // Build path incrementally
    char tempPath[128] = "";
    char* token;
    char* dirPathCopy = dirPath;
    
    // Skip leading slash
    if (dirPathCopy[0] == '/') {
        dirPathCopy++;
        strcpy(tempPath, "/");
    }

    // Parse directory components separated by '/'
    char* saveptr;
    token = strtok_r(dirPathCopy, "/", &saveptr);
    
    while (token != NULL) {
        // Build the incremental path
        if (strlen(tempPath) > 1) {  // Don't add slash after root "/"
            strcat(tempPath, "/");
        }
        strcat(tempPath, token);

        // Check if this path component exists, create if not
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

void fileReceiverRTOS_char(Uart &fileUart){
    static ReceiveState state = PARSE_HEADER;
    static size_t fileSize = 0;
    static char filePath[128] = "";
    static size_t chunkSize = 0;
    static uint8_t buffer[128];
    static uint16_t chunkId = 0;
    static size_t receivedBytes = 0;
    static File file;
    static unsigned long lastByteTime = 0;
    static const unsigned long TIMEOUT = 5000;
    static int retryCount = 0;
    static const int MAX_RETRIES = 3;
    
    // ← CRITICAL FIX: Move bytesAccumulated to function scope
    static size_t bytesAccumulated = 0;

    // Helper function to reset state
    auto resetState = [&]() {
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (file) {
                file.close();
            }
            xSemaphoreGive(sdSemaphore);
        }
        while (fileUart.available()) {
            fileUart.read();
        }
        receivedBytes = 0;
        fileSize = 0;
        chunkId = 0;
        filePath[0] = '\0';
        chunkSize = 0;
        retryCount = 0;
        bytesAccumulated = 0;  // ← CRITICAL: Reset in resetState
        state = PARSE_HEADER;
    };

    // Enhanced timeout handling
    if (state != PARSE_HEADER && millis() - lastByteTime > TIMEOUT) {
        Serial.println("Transfer timeout");
        fileUart.println("ERROR:TIMEOUT");
        while (fileUart.available()) {
            fileUart.read();
        }
        resetState();
        return;
    }

    switch (state){
        case PARSE_HEADER:
            if (fileUart.available()){
                char headerBuffer[256];
                int headerLen = fileUart.readBytesUntil('\n', headerBuffer, sizeof(headerBuffer) - 1);
                if (headerLen <= 0) break;
                
                headerBuffer[headerLen] = '\0';
                
                // Remove trailing whitespace manually
                while (headerLen > 0 && (headerBuffer[headerLen-1] == '\r' || headerBuffer[headerLen-1] == ' ')) {
                    headerBuffer[--headerLen] = '\0';
                }
                
                lastByteTime = millis();
                
                if (strncmp(headerBuffer, "START:", 6) == 0){
                    // Parse: START:<filepath>:<size>
                    char* firstColon = strchr(headerBuffer + 6, ':');
                    char* secondColon = firstColon ? strchr(firstColon + 1, ':') : nullptr;
                    
                    if (firstColon && secondColon){
                        // Extract file path
                        size_t pathLen = firstColon - (headerBuffer + 6);
                        if (pathLen < sizeof(filePath)) {
                            strncpy(filePath, headerBuffer + 6, pathLen);
                            filePath[pathLen] = '\0';
                        }
                        
                        // Extract file size
                        fileSize = strtoul(secondColon + 1, NULL, 10);
                        
                        if (fileSize > 0 && fileSize < 10485760) {
                            Serial.printf("Transfer start: %s (%u bytes)\n", filePath, fileSize);
                            fileUart.printf("ACK:START:SIZE:%u\n", fileSize);
                            receivedBytes = 0;
                            chunkId = 0;
                            retryCount = 0;
                            bytesAccumulated = 0;  // ← CRITICAL: Reset when starting new transfer
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
            if (fileUart.available()){
                lastByteTime = millis();
                
                char chunkHeaderBuffer[64];
                int chunkHeaderLen = fileUart.readBytesUntil('\n', chunkHeaderBuffer, sizeof(chunkHeaderBuffer) - 1);
                if (chunkHeaderLen <= 0) break;
                
                chunkHeaderBuffer[chunkHeaderLen] = '\0';
                
                // Remove trailing whitespace
                while (chunkHeaderLen > 0 && (chunkHeaderBuffer[chunkHeaderLen-1] == '\r' || chunkHeaderBuffer[chunkHeaderLen-1] == ' ')) {
                    chunkHeaderBuffer[--chunkHeaderLen] = '\0';
                }
                
                // Check for header retransmission
                if (strncmp(chunkHeaderBuffer, "START:", 6) == 0) {
                    state = PARSE_HEADER;
                    break;
                }
                
                if (strncmp(chunkHeaderBuffer, "CHUNK:", 6) == 0){
                    // Parse: CHUNK:<id>:<size>
                    char* firstColon = strchr(chunkHeaderBuffer + 6, ':');
                    char* secondColon = firstColon ? strchr(firstColon + 1, ':') : nullptr;
                    
                    if (firstColon && secondColon){
                        uint16_t receivedId = strtoul(chunkHeaderBuffer + 6, NULL, 10);
                        chunkSize = strtoul(secondColon + 1, NULL, 10);
                        
                        if (receivedId == chunkId && chunkSize > 0 && chunkSize <= 128){
                            fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                            bytesAccumulated = 0;  // ← CRITICAL: Reset for new chunk
                            state = READ_CHUNK;
                            retryCount = 0;
                        } else if (receivedId < chunkId) {
                            fileUart.printf("ACK:CHUNK:%u\n", receivedId);
                        } 
                    }
                }
            }
            break;

        case READ_CHUNK:
            // ← CRITICAL FIX: No more static variable in case block
            while (fileUart.available() && bytesAccumulated < chunkSize){
                buffer[bytesAccumulated++] = fileUart.read();
                lastByteTime = millis();
            }
            
            if (bytesAccumulated >= chunkSize){
                bool writeSuccess = false;
                
                if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                    size_t written = file.write(buffer, chunkSize);
                    file.flush();
                    xSemaphoreGive(sdSemaphore);
                    
                    writeSuccess = (written == chunkSize);
                }
                
                if (writeSuccess) {
                    fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                    chunkId++;
                    bytesAccumulated = 0;  // ← Reset for next chunk
                    retryCount = 0;
                    receivedBytes += chunkSize;
                    
                    if (receivedBytes >= fileSize){
                        state = DONE;
                    } else {
                        state = PARSE_CHUNK_HEADER;
                    }
                } else {
                    // ← CRITICAL: Reset on write failure
                    bytesAccumulated = 0;
                    fileUart.println("ERROR:WRITE_FAILED");
                    resetState();
                }
            }
            break;

        case DONE:
            if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                if (file) file.close();
                xSemaphoreGive(sdSemaphore);
            }
            
            Serial.printf("Transfer complete: %s (%u bytes)\n", filePath, receivedBytes);
            
            resetState();
            break;
    }   
}