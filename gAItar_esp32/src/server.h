#ifndef SERVER_H
#define SERVER_H

#include <ESPAsyncWebServer.h>


//setup server
void setupWebServer(AsyncWebServer &server);
//handle routes
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

void setupTestServer(AsyncWebServer &server);

void printMetadata();
#endif