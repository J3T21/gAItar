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

IPAddress local_IP(10, 245, 188, 46);      // Choose an IP in your network range
IPAddress gateway(10, 245, 188, 229);       // Your actual gateway
IPAddress subnet(255, 255, 255, 0);         // Your actual subnet mask
IPAddress primaryDNS(10, 245, 188, 229);    // Use your gateway as DNS
IPAddress secondaryDNS(8, 8, 8, 8);   

static AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  WiFi.mode(WIFI_STA);  // Set WiFi to station mode
  
  // Try a static IP outside the DHCP range
  IPAddress local_IP(10, 245, 188, 200);      // Higher number, likely outside DHCP range
  IPAddress gateway(10, 245, 188, 229);       // Your actual gateway
  IPAddress subnet(255, 255, 255, 0);         // Your actual subnet mask
  IPAddress primaryDNS(10, 245, 188, 229);    // Use your gateway as DNS
  IPAddress secondaryDNS(8, 8, 8, 8);         // Google DNS as backup
  
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure static IP");
  } else {
    Serial.println("Static IP configured: 10.245.188.200");
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
  
  // Verify static IP worked
  if (WiFi.localIP() == local_IP) {
    Serial.println("✓ Static IP configuration successful!");
  } else {
    Serial.println("⚠ Using DHCP instead of static IP");
  }
  
  setupTestServer(server);
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
