#include "esp_server.h"
#include "uart.h"  // For UART send
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "uart.h"
#include "globals.h"

AsyncWebSocket ws("/ws");

void setupWebSocket(AsyncWebServer& server) {
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.printf("WebSocket client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
  });
  
  server.addHandler(&ws);
}


void notifyProgress(const String& stage, int percentage, const String& message) {
  static unsigned long lastUpdate = 0;
  static int lastPercentage = -1;
  static String lastStage = "";
  
  unsigned long now = millis();
  
  // Always send these critical messages regardless of throttling
  bool isCritical = (percentage == 0 || percentage == 100 || 
                     stage == "complete" || stage == "error" || 
                     stage != lastStage);  // Always send when stage changes
  
  // Apply throttling only for non-critical updates
  if (!isCritical && (now - lastUpdate < 100 || abs(percentage - lastPercentage) < 2)) {
    return;
  }
  
  lastUpdate = now;
  lastPercentage = percentage;
  lastStage = stage;
  
  JsonDocument doc;
  doc["type"] = "upload_progress";
  doc["stage"] = stage;
  doc["percentage"] = percentage;
  doc["message"] = message;
  doc["timestamp"] = now;
  
  String jsonString;
  serializeJson(doc, jsonString);
  ws.textAll(jsonString);
  doc.clear();
  Serial.printf("Progress: %s - %d%% - %s\n", stage.c_str(), percentage, message.c_str());
}

void setupTestServer(AsyncWebServer& server) {
  // Handle preflight (CORS OPTIONS) requests globally
  setupWebSocket(server);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");


  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse *response = request->beginResponse(204);
      request->send(response);
    } else {
      request->send(404, "text/plain", "Not found");
    }
  });





auto handleRequest = [](const String &label) {
    return [label](AsyncWebServerRequest *request) {
        Serial.println(label);
        String artist, title, genre;
        
        if (request->hasParam("artist", true)) {
            artist = request->getParam("artist", true)->value();
            Serial.printf("Artist: %s\n", artist.c_str());
        }
        if (request->hasParam("title", true)) {
            title = request->getParam("title", true)->value();
            Serial.printf("Title: %s\n", title.c_str());
        }
        if (request->hasParam("genre", true)) {
            genre = request->getParam("genre", true)->value();
            Serial.printf("Genre: %s\n", genre.c_str());
        }
        
        filePath = "/" + genre + "/" + artist + "/" + title + ".bin";
        
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", label + " command received");
        request->send(response);
    };
};

  auto handlePauseRequest = [](const String &label) {
    return [label](AsyncWebServerRequest *request) {
      Serial.println(label);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", label + " command received");
      request->send(response);
      instructionToSAMD(reinterpret_cast<const uint8_t *>(label.c_str()), label.length());
    };
  };

  auto handleBody = [](const String &label) {
    return [label](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      Serial.printf("[%s] Received %u bytes\n", label.c_str(), len);
  
      // Build full command: [label]:[data]
      String labelPrefix = "[" + label + "]";
      size_t prefixLen = labelPrefix.length();
      size_t totalLen = prefixLen + len;
  
      // Allocate buffer to hold full message
      uint8_t* fullMessage = new uint8_t[totalLen];
  
      // Copy label into the buffer
      memcpy(fullMessage, labelPrefix.c_str(), prefixLen);
  
      // Append raw data after the label
      memcpy(fullMessage + prefixLen, data, len);

      Serial.print("Full message (ascii): ");
      for (size_t i = 0; i < totalLen; i++) {
        char c = fullMessage[i];
        if (isPrintable(c)) {
          Serial.print(c);
        } else {
          Serial.print(".");
        }
      }
      Serial.println();

  
      // Send to SAMD
      instructionToSAMD(fullMessage, totalLen);
  
      // Clean up
      delete[] fullMessage;
  
      // Respond to client
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", label + " command with body received");
      request->send(response);
    };
  };

  auto handleFile = [](const String &label){
    return [label](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: index=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);
  
      if (!index) {
          request->_tempFile=SPIFFS.open("/temp", FILE_WRITE);  // Open file for writing
          Serial.printf("Starting upload: %s\n", "/temp");
      }
  
      if (len) {
          request->_tempFile.write(data, len); // Write incoming data to the file
          Serial.printf("Writing %u bytes to %s\n", len, filename.c_str());
      }
  
      if (final) {
          request->_tempFile.close();  // Close the file after upload
          Serial.printf("Upload complete: %s\n", filename.c_str());
          sendFile = true;
      }
  };
  };

  auto handleGet = [](const String &label) {
    return [label](AsyncWebServerRequest *request) {
        Serial.println("Processing GET request: " + label);
        // Send instruction to SAMD
        instructionToSAMD(reinterpret_cast<const uint8_t *>(label.c_str()), label.length());

        // Wait for response from SAMD
        String uartResponse = "";
        unsigned long startTime = millis();
        while (millis() - startTime < 1000) { // 1-second timeout
            if (instruction_uart.available()) {
                uartResponse += instruction_uart.readString();
            }
        }

        if (uartResponse.isEmpty()) {
            uartResponse = "NA";
        }
        Serial.println("uartResponse" + uartResponse);
        // Send the UART response back to the HTTP request
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", uartResponse);
        request->send(response);
    };
  };

  // Routes
  server.on("/play", HTTP_POST, handleRequest("Play"), nullptr, handleBody("Play"));
  server.on("/pause", HTTP_POST, handlePauseRequest("Pause"), nullptr, nullptr);
  server.on("/skip", HTTP_POST, handleRequest("Skip"), nullptr, handleBody("Skip"));
  server.on("/shuffle", HTTP_POST, handleRequest("Shuffle"), nullptr, handleBody("Shuffle"));
  server.on("/upload", HTTP_POST, handleRequest("Upload"),handleFile("Upload"), nullptr);
  server.on("/upload-binary", HTTP_POST, handleRequest("Upload-Binary"), handleFile("Upload"), nullptr);
  server.on("/existing-songs", HTTP_GET, handleGet("List"));
  server.begin();
}

void listSPIFFSFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void formatSPIFFS() {
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully.");
  } else {
    Serial.println("Failed to format SPIFFS.");
  }
}

// Add after the existing notifyProgress function
void notifyPlaybackStatus(const String& jsonData) {
  JsonDocument doc;
  doc["type"] = "playback_status";
  doc["timestamp"] = millis();
  
  // Parse the incoming JSON data from Grand Central
  JsonDocument statusDoc;
  DeserializationError error = deserializeJson(statusDoc, jsonData);
  
  if (!error) {
    // Only forward the essential time data
    if (!statusDoc["currentTime"].isNull()) {
      doc["currentTime"] = statusDoc["currentTime"];
    }
    if (!statusDoc["totalTime"].isNull()) {
      doc["totalTime"] = statusDoc["totalTime"];
    }
    
    // ESP32 can calculate these derived values:
    // - isPlaying = currentTime > 0 && currentTime < totalTime
    // - isPaused = check if currentTime hasn't changed for a while
    // - isFinished = currentTime >= totalTime
    // - progress = (currentTime / totalTime) * 100
    
  } else {
    // If JSON parsing fails, send raw data
    doc["rawData"] = jsonData;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  ws.textAll(jsonString);
  doc.clear();
  statusDoc.clear();
  
  Serial.printf("Playback Status: %s\n", jsonData.c_str());
}