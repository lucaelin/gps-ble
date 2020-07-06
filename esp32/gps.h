#ifndef GPS_H
#define GPS_H

#define TBEAM

#include "Arduino.h"
#include <NMEAGPS.h>
extern NMEAGPS gps;

void setupGPS();
void setUBXSpeed(uint16_t ms);

#endif
