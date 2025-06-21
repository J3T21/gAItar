#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include "uart.h"
#include "esp_server.h"
#include "globals.h"

const char* ssid = "PixelJ";  // Your WiFi SSID
const char* password = "12345678";    // Your WiFi password

// Updated to match your computer's network
IPAddress local_IP(192, 168, 113, 200);      // Static IP you want
IPAddress gateway(192, 168, 113, 82);        // Your actual gateway
IPAddress subnet(255, 255, 255, 0);          // Your actual subnet mask
IPAddress primaryDNS(192, 168, 113, 82);     // Use your gateway as DNS
IPAddress secondaryDNS(8, 8, 8, 8);          // Google DNS as backup

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  WiFi.mode(WIFI_STA);  // Set WiFi to station mode
  
  // Configure static IP with correct network settings
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure static IP");
  } else {
    Serial.printf("Static IP configured: %s\n", local_IP.toString().c_str());
  }
  
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet: ");
  Serial.println(WiFi.subnetMask());
  
  // Verify static IP worked
  if (WiFi.localIP() == local_IP) {
    Serial.println("✓ Static IP configuration successful!");
  } else {
    Serial.println("⚠ Using DHCP instead of static IP");
  }
  
  setupTestServer(server);
  Serial.printf("HTTP server started on: http://%s\n", WiFi.localIP().toString().c_str());
  
  setupUARTs();
  listSPIFFSFiles();
  Serial.println("Clearing files...");
  formatSPIFFS();
  listSPIFFSFiles();
}

void loop() {
    uploadToSAMD_state(sendFile, filePath);
    handlePlaybackMessages();
}