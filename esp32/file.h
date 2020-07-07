#ifndef FILE_H
#define FILE_H

#include "FS.h"
#include "FFat.h"
#include "data.h"
#include <WiFiClient.h>

bool isGpsLogFile( const char* );
File getGPSLog();
void readGPSLog( WiFiClient* );
uint32_t getGPSLogSize();
bool deleteOldestFile();
void listAllFiles();

#endif
