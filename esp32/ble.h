#ifndef BLE_H
#define BLE_H

#include "Arduino.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "FFat.h"
#include "FS.h"

#include "data.h"
#include "file.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID            "2e2d0001-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_GPS_UUID "2e2d0002-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_HIS_UUID "2e2d0003-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_STS_UUID "2e2d0004-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_MTU 200

extern BLEServer *pServer;
extern BLEService *pService;
extern BLEAdvertising *pAdvertising;

extern BLECharacteristic *currentGPSCharacteristic;
extern BLECharacteristic *historyGPSCharacteristic;
extern BLECharacteristic *statusCharacteristic;

extern uint32_t history_transfer_index;

void setupBLE();

#endif
