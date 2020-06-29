#ifndef DATA_H
#define DATA_H


#include "Arduino.h"
#include <NMEAGPS.h>
#include "FS.h"
#include "FFat.h"
#include "file.h"

// max 20 byte to fit into BLE notify payload
__attribute__((packed)) struct GpsData {
  uint32_t time;
  uint8_t status;
  uint8_t sats;
  uint8_t err_lat;
  uint8_t err_lng;
  float lat;
  float lng;
  float alt;
};

enum Situation { NOT_VALID, STATIONARY, MOVING_STRAIGHT, SIGNIFICANT };

__attribute__((packed)) struct StatusData {
  uint32_t flash_total;
  uint32_t flash_free;
  uint32_t history_length;
  char logFile[12];
  uint32_t logFileSize;
};

const uint32_t history_length = 1000;
extern uint32_t history_index;
extern GpsData history_gps[history_length];

GpsData parseFix(gps_fix fix);
bool pointInEllipse(int x, int y, int a, int b);
void loadGpsHistory();
void storeGpsEntry(GpsData *entry);
GpsData getPreviousEntry();

#endif
