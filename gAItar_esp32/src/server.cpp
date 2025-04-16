#include "server.h"
#include "uart.h"  // For UART send

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }

  for (size_t i = 0; i < len; i++) {
    sendToSAMD(data[i]);  // Send over UART
  }

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