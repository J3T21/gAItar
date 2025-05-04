#include "server.h"
#include "uart.h"  // For UART send
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }

  uploadToSAMD(data, len);  // Send data to SAMD

  if (final) {
    Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
  }
}

void setupWebServer(AsyncWebServer& server) {
  server.on("/upload", HTTP_POST,
    [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Upload complete");
    },
    handleUpload
  );

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

void setupTestServer(AsyncWebServer& server) {
  // Handle preflight (CORS OPTIONS) requests globally

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
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", label + " command received");
      request->send(response);
    };
  };

  auto handleBody = [](const String &label) {
    return [label](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
      }
      Serial.println("[" + label + "] Body: " + body);

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", label + " command with body received");
      request->send(response);
    };
  };

  auto handleFile = [](const String &label){
    return [label](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      Serial.printf("Upload[%s]: index=%u, len=%u, final=%d\n", filename.c_str(), index, len, final);
  
      if (!index) {
          request->_tempFile=SPIFFS.open("/" + filename, FILE_WRITE);  // Open file for writing
          Serial.printf("Starting upload: %s\n", filename.c_str());
      }
  
      if (len) {
          request->_tempFile.write(data, len); // Write incoming data to the file
          Serial.printf("Writing %u bytes to %s\n", len, filename.c_str());
      }
  
      if (final) {
          request->_tempFile.close();  // Close the file after upload
          Serial.printf("Upload complete: %s\n", filename.c_str());
      }
  };
  };

  // Routes
  server.on("/play", HTTP_POST, handleRequest("Play"), nullptr, handleBody("Play"));
  server.on("/pause", HTTP_POST, handleRequest("Pause"), nullptr, handleBody("Pause"));
  server.on("/skip", HTTP_POST, handleRequest("Skip"), nullptr, handleBody("Skip"));
  server.on("/shuffle", HTTP_POST, handleRequest("Shuffle"), nullptr, handleBody("Shuffle"));
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("Upload request handler");
    request->send(200, "text/plain", "upload handler esp");
  },handleFile("Upload"), handleBody("Upload")
);
  server.begin();}
