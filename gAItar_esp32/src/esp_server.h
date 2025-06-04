#ifndef ESP_SERVER_H
#define ESP_SERVER_H

#include <ESPAsyncWebServer.h>

void setupTestServer(AsyncWebServer &server);

void setupWebSocket(AsyncWebServer &server);

void listSPIFFSFiles();

void formatSPIFFS();

void notifyProgress(const String& stage, int percentage, const String& message = ""); 
void notifyPlaybackStatus(const String& jsonData);  // Add this

#endif