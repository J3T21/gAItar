#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include "HardwareSerial.h"

const char* ssid = "INTERNETTY-28071";  // Your WiFi SSID
const char* password = "daffodil76";    // Your WiFi password

AsyncWebServer server(80);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for(size_t i=0; i<len; i++){
    Serial.print(data[i]);
  }
  if(final){
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());


  server.on("/test", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile()){ //p->isPost() is also true
      Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      handleUpload(request, p->name(), 0, (uint8_t*)p->value().c_str(), p->size(), true);
    } else if(p->isPost()){
      Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
    // Placeholder for the request handler
    request->send(200, "text/plain", "Received");
  });


  // Handle 404 errors
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  // Start the server
  server.begin();

}

void loop() {
  if (Serial2.available()) {
    Serial.println("Serial2 available");
    Serial.write(Serial2.read());
  }

  if (Serial1.available()) {
    Serial.println("Serial1 available");
    Serial.write(Serial1.read());
  }

  if (Serial.available()) {
    char c = Serial.read();  // Read once
    Serial2.write(c);
    Serial1.write(c);
  }
}
