#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include "uart.h"
#include "server.h"

const char* ssid = "gAItar_wifi";  // Your WiFi SSID
const char* password = "gAItar123";    // Your WiFi password

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  WiFi.softAP(ssid, password);
  Serial.print("Setting up WiFi..");
  Serial.println("\nConnected!");
  Serial.println("IP Address: " + WiFi.softAPIP().toString());
  setupTestServer(server); //test post requests to server
  setupUARTs();  // Initialize UART communication

}

void loop() {
}
