#include "uart.h"
#include "translate.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include "globals.h"
#include <FreeRTOS_SAMD51.h>

extern volatile bool isPlaying;
extern volatile bool isPaused;
extern volatile bool newSongRequested;
extern String currentSongPath;
extern size_t currentEventIndex;
extern unsigned long startTime;
extern unsigned long pauseOffset;
extern SemaphoreHandle_t playbackSemaphore;
extern SemaphoreHandle_t sdSemaphore;

String prevTitle = "";
String prevArtist = "";
String prevGenre = "";

void fileReceiver(Uart &fileUart) {
    static size_t fileSize = 0;
    static size_t receivedBytes = 0;
    static File sdFile;
    static String fileName = "";
    static bool receivingFile = false;
    static String headerBuffer = "";
    static unsigned long lastByteTime = 0;
    const unsigned long timeoutMs = 100000; // 3 seconds timeout

    while (fileUart.available()) {
        lastByteTime = millis(); // reset timeout on any byte received

        if (!receivingFile) {
            // Read one character at a time and build the header line
            char c = fileUart.read();
            if (c == '\n') {
                Serial.println("Header received: " + headerBuffer);

                if (headerBuffer.startsWith("START:")) {
                    int sizeIdx = headerBuffer.indexOf("SIZE:");
                    if (sizeIdx != -1) {
                        fileName = headerBuffer.substring(7, sizeIdx);
                        fileSize = headerBuffer.substring(sizeIdx + 5).toInt();
                        receivedBytes = 0;
                        sdFile = sd.open(fileName, FILE_WRITE);
                        if (sdFile) {
                            receivingFile = true;
                            Serial.println("Receiving file: " + fileName + " (" + String(fileSize) + " bytes)");
                        } else {
                            Serial.println("Failed to open file");
                        }
                    }
                }
                headerBuffer = ""; // reset for next header
            } else {
                headerBuffer += c;
            }
        } else {
            // Read file data
            char buffer[64];
            size_t bytesRead = fileUart.readBytes(buffer, sizeof(buffer));
            if (bytesRead > 0) {
                sdFile.write((uint8_t*)buffer, bytesRead);
                receivedBytes += bytesRead;

                if (receivedBytes >= fileSize) {
                    sdFile.close();
                    Serial.println("File transfer complete!");
                    receivingFile = false;
                }
            }
        }
    }

    // Check timeout condition
    if (receivingFile && millis() - lastByteTime > timeoutMs) {
        Serial.println("Timeout: file transfer aborted");
        if (sdFile) sdFile.close();
        receivingFile = false;
        receivedBytes = 0;
        fileSize = 0;
        headerBuffer = "";
    }
}

void fileReceiver_chunk(Uart &fileUart) {
    static bool receivingFile = false;   // Flag to check if we're currently receiving a file
    static size_t fileSize = 0;          // Total size of the file we're expecting
    static size_t receivedBytes = 0;     // Number of bytes received so far
    static File sdFile;                 // File handle for writing to SD
    static String fileName = "";         // File name
    static uint16_t chunkId = 0;         // Chunk ID for acknowledging
  
    while (fileUart.available()) {
      String line = fileUart.readStringUntil('\n');  // Read one line from UART
  
      if (!receivingFile) {
        // Process the header to start receiving a file
        if (line.startsWith("START:")) {
          // Parse the START header
          int fileNameIdx = line.indexOf("START:") + 6;
          int sizeIdx = line.indexOf("SIZE:");
          fileName = line.substring(fileNameIdx, sizeIdx - 1);  // Extract file name
          fileSize = line.substring(sizeIdx + 5).toInt();        // Extract file size
          receivedBytes = 0;  // Reset byte counter
  
          // Open the file for writing on SD
          sdFile = sd.open(fileName.c_str(), FILE_WRITE);
          if (!sdFile) {
            Serial.println("Failed to open file for writing");
            return;
          }
  
          Serial.println("Receiving file: " + fileName + " (" + String(fileSize) + " bytes)");
          receivingFile = true;  // Mark that we are receiving a file
        }
      } else {
        // Process each chunk
        if (line.startsWith("CHUNK:")) {
          int colonPos1 = line.indexOf(':');
          int colonPos2 = line.indexOf(':', colonPos1 + 1);
          int sizePos = line.indexOf("SIZE:", colonPos2 + 1);
  
          if (colonPos1 != -1 && colonPos2 != -1 && sizePos != -1) {
            uint16_t currentChunkId = line.substring(colonPos1 + 1, colonPos2).toInt();
            size_t chunkSize = line.substring(sizePos + 5).toInt();
  
            if (currentChunkId != chunkId) {
              Serial.printf("Error: Expected chunk %u, but received chunk %u\n", chunkId, currentChunkId);
              return;  // Out-of-order chunk, ignore it for now (you can handle retries here)
            }
  
            // Read the chunk data
            uint8_t buffer[chunkSize];
            fileUart.readBytes(buffer, chunkSize);
  
            // Write data to SD
            sdFile.write(buffer, chunkSize);
            receivedBytes += chunkSize;
            chunkId++;
  
            // Send ACK for this chunk
            fileUart.printf("ACK:%u\n", currentChunkId);
  
            // If we've received the entire file, close it
            if (receivedBytes >= fileSize) {
              sdFile.close();
              Serial.println("File transfer complete.");
              receivingFile = false;  // Reset the flag
            }
          }
        }
      }
  
      // Check if we've received the 'DONE' signal
      if (line == "DONE") {
        Serial.println("End of file transfer received.");
      }
    }
  }

  const char* findFile(const String& title, const String& artist, const String& genre) {
    static char matchingFilePath[128]; // Static buffer to store the matching file path
    String directoryPath = "/" + genre + "/" + artist; // Construct the directory path

    SdFile directory;
    SdFile entry;

    // Open the directory corresponding to the genre and artist
    if (!directory.open(directoryPath.c_str())) {
        Serial.print("Failed to open directory: ");
        Serial.println(directoryPath);
        return nullptr;
    }

    // Iterate through the files in the directory
    while (entry.openNext(&directory, O_RDONLY)) {
        char name[64];
        entry.getName(name, sizeof(name));

        // Skip directories
        if (entry.isDir()) {
            entry.close();
            continue;
        }

        // Open the file and parse its metadata
        File file = sd.open((directoryPath + "/" + name).c_str(), FILE_READ);
        if (!file) {
            Serial.print("Failed to open file: ");
            Serial.println(name);
            entry.close();
            continue;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.print("Failed to parse JSON in file: ");
            Serial.println(name);
            entry.close();
            continue;
        }

        // Check if metadata matches
        if (String(doc["title"].as<const char*>()) == title) {
            String fullPath = directoryPath + "/" + name;
            strncpy(matchingFilePath, fullPath.c_str(), sizeof(matchingFilePath) - 1);
            matchingFilePath[sizeof(matchingFilePath) - 1] = '\0'; // Ensure null-termination
            entry.close();
            directory.close();
            return matchingFilePath; // Return the matching file path
        }

        entry.close();
    }

    directory.close();
    return nullptr; // Return nullptr if no match is found
}

const char* findFileRTOS(const String& title, const String& artist, const String& genre) {
    static char matchingFilePath[128]; // Static buffer to store the matching file path
    String directoryPath = "/" + genre + "/" + artist;

    // Acquire semaphore before accessing SD
    if (!xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
        Serial.println("Failed to take SD semaphore");
        return nullptr;
    }

    SdFile directory;
    SdFile entry;

    if (!directory.open(directoryPath.c_str())) {
        Serial.print("Failed to open directory: ");
        Serial.println(directoryPath);
        xSemaphoreGive(sdSemaphore);
        return nullptr;
    }

    while (entry.openNext(&directory, O_RDONLY)) {
        char name[64];
        entry.getName(name, sizeof(name));

        if (entry.isDir()) {
            entry.close();
            continue;
        }

        File file = sd.open((directoryPath + "/" + name).c_str(), FILE_READ);
        if (!file) {
            Serial.print("Failed to open file: ");
            Serial.println(name);
            entry.close();
            continue;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.print("Failed to parse JSON in file: ");
            Serial.println(name);
            entry.close();
            continue;
        }

        if (String(doc["title"].as<const char*>()) == title) {
            String fullPath = directoryPath + "/" + name;
            strncpy(matchingFilePath, fullPath.c_str(), sizeof(matchingFilePath) - 1);
            matchingFilePath[sizeof(matchingFilePath) - 1] = '\0';
            entry.close();
            directory.close();
            xSemaphoreGive(sdSemaphore);
            return matchingFilePath;
        }

        entry.close();
    }

    directory.close();
    xSemaphoreGive(sdSemaphore);
    return nullptr;
}

  

void instructionReceiver(Uart &instrUart) {
    static enum { WAIT_FOR_HEADER, WAIT_FOR_LENGTH, WAIT_FOR_PAYLOAD } state = WAIT_FOR_HEADER;
    static uint8_t length = 0;
    static uint8_t receivedBytes = 0;
    static uint8_t buffer[256];

    while (instrUart.available() > 0) {
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
                    Serial.print("Received command: ");
                    for (uint8_t i = 0; i < length; i++) {
                        Serial.write(buffer[i]);  // Print as characters (if ASCII)
                    }
                    Serial.println();
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, buffer, length);
                    if (error) {
                        Serial.println("Failed to parse command JSON");
                        state = WAIT_FOR_HEADER;
                        return;
                    }

                    // Extract metadata fields
                    String command = doc["command"];
                    if (command == "Play") {
                        String title = doc["title"];
                        String artist = doc["artist"];
                        String genre = doc["genre"];

                        // Find the file on the SD card
                        const char* filePath = findFile(title, artist, genre);
                        if (filePath) {
                            Serial.print("Found file: ");
                            Serial.println(filePath);
                        } else {
                            Serial.println("File not found with matching metadata");
                        }
                    }
                    // Reset for next message
                    state = WAIT_FOR_HEADER;
                }
                break;
        }
    }
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

                    // Check if the buffer starts with "[Play]"
                // Check if the buffer starts with "[Play]"
                    if (xSemaphoreTake(playbackSemaphore, portMAX_DELAY)) {
                        if (strncmp((char*)buffer, "[Play]", 6) == 0) {
                            const char* jsonPart = (char*)buffer + 6;
                        
                            // Parse the JSON regardless, but use it to compare with previous metadata
                            JsonDocument doc;
                            DeserializationError error = deserializeJson(doc, jsonPart);
                            if (error) {
                                Serial.println("Failed to parse command JSON");
                                Serial.print("Error: ");
                                Serial.println(error.c_str());
                                state = WAIT_FOR_HEADER;
                                xSemaphoreGive(playbackSemaphore);
                                return;
                            }
                        
                            String title = doc["title"];
                            String artist = doc["artist"];
                            String genre = doc["genre"];

                            if (!newSongRequested &&
                                isPaused &&
                                title == prevTitle &&
                                artist == prevArtist &&
                                genre == prevGenre) {
                                startTime = millis() - pauseOffset;
                                isPlaying = true;
                                isPaused = false;
                                Serial.println("Resuming previous song (metadata matched)");
                            } else {
                                // Update cached metadata
                                prevTitle = title;
                                prevArtist = artist;
                                prevGenre = genre;

                                // Find file based on metadata
                                const char* filePath = findFileRTOS(title, artist, genre);
                                if (filePath) {
                                    Serial.print("Found file: ");
                                    Serial.println(filePath);
                                    if (currentSongPath != filePath) {
                                        currentSongPath = filePath;
                                        newSongRequested = true;
                                        isPlaying = true;
                                        isPaused = false;
                                        startTime = millis();
                                    } else if (isPaused) {
                                        startTime = millis() - pauseOffset;
                                        isPlaying = true;
                                        isPaused = false;
                                    }
                                } else {
                                    Serial.println("File not found with matching metadata");
                                }
                            }
                        }else if (strncmp((char*)buffer, "Pause", 5) == 0) {
                            isPaused = true;
                            pauseOffset = millis() - startTime;
                            Serial.println("Paused: isPaused true");
                        } else {
                            Serial.println("Invalid command prefix");
                        }
                        // Other commands like "Pause"...
                        xSemaphoreGive(playbackSemaphore);
                    }

           

                    // Reset for next message
                    state = WAIT_FOR_HEADER;
                }
                break;
        }
    }
}

bool createDirectories(String fullPath) {
    // Remove the file name (keep only directories)
    int lastSlash = fullPath.lastIndexOf('/');
    if (lastSlash <= 0) {
      return true;  // No directories to create
    }
  
    String dirPath = fullPath.substring(0, lastSlash);
    String tempPath = "";
  
    uint start = 1; // skip leading slash
    while (start < dirPath.length()) {
      int nextSlash = dirPath.indexOf('/', start);
      if (nextSlash == -1) nextSlash = dirPath.length();
  
      tempPath += "/" + dirPath.substring(start, nextSlash);
  
      if (!sd.exists(tempPath.c_str())) {
        if (!sd.mkdir(tempPath.c_str())) {
          Serial.print("Failed to create: ");
          Serial.println(tempPath);
          return false;
        }
      }
  
      start = nextSlash + 1;
    }
  
    return true;
  }

bool createDirectoriesRTOS(String fullPath) {
    int lastSlash = fullPath.lastIndexOf('/');
    if (lastSlash <= 0) {
        return true;  // No directories to create
    }

    String dirPath = fullPath.substring(0, lastSlash);
    String tempPath = "";

    uint start = 1; // skip leading slash
    while (start < dirPath.length()) {
        int nextSlash = dirPath.indexOf('/', start);
        if (nextSlash == -1) nextSlash = dirPath.length();

        tempPath += "/" + dirPath.substring(start, nextSlash);

        // --- SD card access protected by mutex ---
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)) {
            if (!sd.exists(tempPath.c_str())) {
                if (!sd.mkdir(tempPath.c_str())) {
                    Serial.print("Failed to create: ");
                    Serial.println(tempPath);
                    return false;
                }
            }
            xSemaphoreGive(sdSemaphore);
        } else {
            Serial.println("Failed to take sdSemaphore in createDirectoriesRTOS");
            return false;
        }
        // --- End mutex section ---

        start = nextSlash + 1;
    }

    return true;
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

void fileReceiver_state(Uart &fileUart){
    static  ReceiveState state = PARSE_HEADER;
    static size_t fileSize = 0;
    static String filePath = "";
    static size_t chunkSize = 0;
    static uint8_t buffer[64];
    static uint16_t chunkId = 0;
    static size_t receivedBytes = 0;
    static File file;
    static unsigned long lastByteTime = 0;
    static const unsigned long TIMEOUT = 10000; 

    // Timeout handling (only if not in idle/initial state)
    if (state != PARSE_HEADER && state != OPEN_FILE && millis() - lastByteTime > TIMEOUT) {
        Serial.println("Timeout occurred, resetting receiver.");
        receivedBytes = 0;
        fileSize = 0;
        chunkId = 0;
        filePath = "";
        chunkSize = 0;
        if (file) file.close();
        state = PARSE_HEADER;
        return;
    }

    switch (state){
        case PARSE_HEADER:
            if (fileUart.available()){
                String header = fileUart.readStringUntil('\n');
                header.trim(); // Remove any trailing whitespace
                // Parse header in format: "START:<file_path>:SIZE:<file_size>"
                if (header.startsWith("START:")){
                    int startIdx = header.indexOf(":", 6);
                    int sizeIdx = header.indexOf(":", startIdx + 1);
                    if (startIdx != -1 && sizeIdx != -1){
                        filePath = header.substring(6, startIdx);
                        fileSize = header.substring(sizeIdx + 1).toInt();
                        //Serial.printf("Receiving file: %s (%d bytes)\n", filePath.c_str(), fileSize);
                        fileUart.printf("ACK:START:SIZE:%u\n", fileSize);
                        //Serial.printf("ACK:START:SIZE:%u\n", fileSize);
                        state = OPEN_FILE;
                    }else{
                        //Serial.printf("Invalid header format recvd: %s\n", header.c_str());
                        state = PARSE_HEADER; // Reset state
                    }
                }
            }
            break;
        case OPEN_FILE:
            if (!createDirectories(filePath)){
                Serial.printf("Failed to create directories for %s\n", filePath.c_str());
                state = PARSE_HEADER; // Reset state
                return;
            }
            file = sd.open(filePath.c_str(), FILE_WRITE);
            if(file){
                //Serial.printf("File %s opened for writing\n", filePath.c_str());
                lastByteTime = millis();
                chunkId = 0;
                state = PARSE_CHUNK_HEADER;
            }else{
                //Serial.printf("Failed to open file on SD %s\n", filePath.c_str());
                state = PARSE_HEADER; // Reset state
            }
            break;
    
        case PARSE_CHUNK_HEADER:
            if (fileUart.available()){
                lastByteTime = millis(); // Reset timeout on any byte received
                String chunkHeader = fileUart.readStringUntil('\n');
                chunkHeader.trim(); //
                //Serial.println("SAMD recvd: "+chunkHeader);
                // Parse chunk header in format: "CHUNK:<chunk_id>:SIZE:<chunk_size>"
                if (chunkHeader.startsWith("CHUNK:")){
                    int colon1 = chunkHeader.indexOf(':', 6);
                    int colon2 = chunkHeader.indexOf(':', colon1 + 1);
                    if (colon1 != -1 && colon2 != -1){
                        uint16_t receivedId = chunkHeader.substring(6, colon1).toInt();
                        chunkSize = chunkHeader.substring(colon2 + 1).toInt();
                        //Serial.printf("RECVD ID: %u EXPCTD ID: %u\n",chunkId, receivedId);
                        if (receivedId == chunkId){
                            //Serial.printf("Receiving chunk %u of size %u\n", chunkId, chunkSize);
                            fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                            state = READ_CHUNK;
                        }else{
                            //Serial.printf("Chunk ID mismatch: expected %u, received %u\n", chunkId, receivedId);
                            state = PARSE_CHUNK_HEADER; // Reset state
                        }   
                    }else{
                        //Serial.printf("Invalid chunk header format: %s\n", chunkHeader.c_str());
                        state = PARSE_CHUNK_HEADER; // Reset state
                    }
                }
            }break;
        case READ_CHUNK:
            if (fileUart.available()){
                lastByteTime = millis(); // Reset timeout on any byte received
                size_t bytesRead = fileUart.readBytes(buffer, chunkSize);
                if (bytesRead == chunkSize){
                    file.write(buffer, bytesRead);
                    receivedBytes += bytesRead;
                    fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                    //Serial.printf("Chunk %u received and written to file\n", chunkId);
                    chunkId++;
                    if (receivedBytes >= fileSize){
                        state = DONE;
                    }
                    else{
                        state = PARSE_CHUNK_HEADER; // Prepare for next chunk
                    }
                } else{
                    //Serial.printf("Error reading chunk: expected %u bytes, got %u\n", chunkSize, bytesRead);
                    state = PARSE_CHUNK_HEADER; // Reset state probably retry here and verify total size of chunks
                }
            } break;

        case DONE:
            if (file){
                file.close();
                Serial.printf("File %s transfer complete\n", filePath.c_str());                
            }
            receivedBytes = 0; //reset counter
            state = PARSE_HEADER; // Reset state for next file transfer
            break;
    }   
}

void fileReceiverRTOS(Uart &fileUart){
    static  ReceiveState state = PARSE_HEADER;
    static size_t fileSize = 0;
    static String filePath = "";
    static size_t chunkSize = 0;
    static uint8_t buffer[64];
    static uint16_t chunkId = 0;
    static size_t receivedBytes = 0;
    static File file;
    static unsigned long lastByteTime = 0;
    static const unsigned long TIMEOUT = 10000; 

    // Timeout handling (only if not in idle/initial state)
    if (state != PARSE_HEADER && state != OPEN_FILE && millis() - lastByteTime > TIMEOUT) {
        Serial.println("Timeout occurred, resetting receiver.");
        receivedBytes = 0;
        fileSize = 0;
        chunkId = 0;
        filePath = "";
        chunkSize = 0;
        if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
            if (file){file.close();}
        }
        xSemaphoreGive(sdSemaphore);           
        state = PARSE_HEADER;
        return;
    }

    switch (state){
        case PARSE_HEADER:
            if (fileUart.available()){
                String header = fileUart.readStringUntil('\n');
                header.trim(); // Remove any trailing whitespace
                // Parse header in format: "START:<file_path>:SIZE:<file_size>"
                if (header.startsWith("START:")){
                    int startIdx = header.indexOf(":", 6);
                    int sizeIdx = header.indexOf(":", startIdx + 1);
                    if (startIdx != -1 && sizeIdx != -1){
                        filePath = header.substring(6, startIdx);
                        fileSize = header.substring(sizeIdx + 1).toInt();
                        //Serial.printf("Receiving file: %s (%d bytes)\n", filePath.c_str(), fileSize);
                        fileUart.printf("ACK:START:SIZE:%u\n", fileSize);
                        //Serial.printf("ACK:START:SIZE:%u\n", fileSize);
                        state = OPEN_FILE;
                    }else{
                        //Serial.printf("Invalid header format recvd: %s\n", header.c_str());
                        state = PARSE_HEADER; // Reset state
                    }
                }
            }
            break;
        case OPEN_FILE:
            if (!createDirectoriesRTOS(filePath)){
                //Serial.printf("Failed to create directories for %s\n", filePath.c_str());
                state = PARSE_HEADER; // Reset state
                return;
            }
            if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                file = sd.open(filePath.c_str(), FILE_WRITE);

                if(file){
                    //Serial.printf("File %s opened for writing\n", filePath.c_str());
                    lastByteTime = millis();
                    chunkId = 0;
                    state = PARSE_CHUNK_HEADER;
                }else{
                //Serial.printf("Failed to open file on SD %s\n", filePath.c_str());
                state = PARSE_HEADER; // Reset state
                }
                xSemaphoreGive(sdSemaphore);
            }else{
                Serial.println("Failed to take sdSemaphore in OPEN_FILE");
                state = PARSE_HEADER; // Reset state
            }
            break;
    
        case PARSE_CHUNK_HEADER:
            if (fileUart.available()){
                lastByteTime = millis(); // Reset timeout on any byte received
                String chunkHeader = fileUart.readStringUntil('\n');
                chunkHeader.trim(); //
                //Serial.println("SAMD recvd: "+chunkHeader);
                // Parse chunk header in format: "CHUNK:<chunk_id>:SIZE:<chunk_size>"
                if (chunkHeader.startsWith("CHUNK:")){
                    int colon1 = chunkHeader.indexOf(':', 6);
                    int colon2 = chunkHeader.indexOf(':', colon1 + 1);
                    if (colon1 != -1 && colon2 != -1){
                        uint16_t receivedId = chunkHeader.substring(6, colon1).toInt();
                        chunkSize = chunkHeader.substring(colon2 + 1).toInt();
                        //Serial.printf("RECVD ID: %u EXPCTD ID: %u\n",chunkId, receivedId);
                        if (receivedId == chunkId){
                            //Serial.printf("Receiving chunk %u of size %u\n", chunkId, chunkSize);
                            fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                            state = READ_CHUNK;
                        }else{
                            //Serial.printf("Chunk ID mismatch: expected %u, received %u\n", chunkId, receivedId);
                            state = PARSE_CHUNK_HEADER; // Reset state
                        }   
                    }else{
                        //Serial.printf("Invalid chunk header format: %s\n", chunkHeader.c_str());
                        state = PARSE_CHUNK_HEADER; // Reset state
                    }
                }
            }break;
        case READ_CHUNK:{
            static size_t bytesAccumulated = 0;
            while (fileUart.available() && bytesAccumulated < chunkSize){
                buffer[bytesAccumulated++] = fileUart.read();
                lastByteTime = millis();
            }
            if (bytesAccumulated == chunkSize){
                if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                    size_t written = file.write(buffer, chunkSize);
                    xSemaphoreGive(sdSemaphore);
                    if (written == chunkSize){
                        receivedBytes += chunkSize;
                        fileUart.printf("ACK:CHUNK:%u\n", chunkId);
                        chunkId++;
                        bytesAccumulated = 0;
                        state = (receivedBytes >= fileSize) ? DONE : PARSE_CHUNK_HEADER;
                    }else{
                        Serial.printf("Write failed for chunk %u\n", chunkId);
                    }
                }else{
                    Serial.printf("Semaphore timeout or failure retry time..");
                }
            }
            break;
        }
        case DONE:
            if (xSemaphoreTake(sdSemaphore, portMAX_DELAY)){
                if (file){file.close();}
                xSemaphoreGive(sdSemaphore);
                Serial.printf("File %s transfer complete\n", filePath.c_str());                
            }
            receivedBytes = 0; //reset counter
            state = PARSE_HEADER; // Reset state for next file transfer
            break;
    }   
}


