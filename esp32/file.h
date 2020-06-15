#ifndef FILE_H
#define FILE_H

#include "FS.h"
#include "FFat.h"
#include "data.h"

bool isGpsLogFile( const char* );
File getLatestFile();

#endif
