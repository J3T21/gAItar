#ifndef SERVER_H
#define SERVER_H

#include <ESPAsyncWebServer.h>

void setupTestServer(AsyncWebServer &server);

void listSPIFFSFiles();

void formatSPIFFS();

#endif