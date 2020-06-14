/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <NMEAGPS.h>
#include "FS.h"
#include "FFat.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID            "2e1d0001-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_GPS_UUID "2e1d0002-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_HIS_UUID "2e1d0003-cc74-4675-90a3-ec80f1037391"
#define CHARACTERISTIC_MTU 512

BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *currentGPSCharacteristic = NULL;
BLECharacteristic *historyGPSCharacteristic = NULL;
BLEAdvertising *pAdvertising = NULL;

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

// max 20 byte in BLE notify payload
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

File gps_log;
NMEAGPS gps;
GpsData last_gps;
const uint32_t history_length = 100;
uint32_t history_index = 0;
GpsData history_gps[history_length];

uint32_t history_transfer_index = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("BLE connected");
      history_transfer_index = 0;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("BLE disconnected");
      BLEDevice::startAdvertising();
    }
};

class GpsCallbacks: public BLECharacteristicCallbacks {
  void onRead (BLECharacteristic *pCharacteristic) {
    uint32_t max_entries = floor(CHARACTERISTIC_MTU / sizeof(GpsData));
    Serial.print("max_entries: ");
    Serial.println(max_entries);
    Serial.print("history_length: ");
    Serial.println(history_length);
    Serial.print("history_transfer_index: ");
    Serial.println(history_transfer_index);
    uint32_t num_entries = min(max_entries, history_length-history_transfer_index);
    Serial.print("num_entries: ");
    Serial.println(num_entries);
    pCharacteristic->setValue((uint8_t*)&history_gps[history_transfer_index], num_entries * sizeof(GpsData));
    history_transfer_index += num_entries;
    Serial.println("Transmission complete");
  }
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, (uint32_t) SERIAL_8N1, 14, 12);
  delay(100);
  gps.send_P( &Serial1, F("PUBX,40,GST,0,1,0,0,0,0") );
  
  delay(1000);

  //FFat.format();
  Serial.setDebugOutput(true);
  if(!FFat.begin(true)){
      Serial.println("FFat Mount Failed");
      return;
  }

  loadGpsLog();

  delay(100);
  setupBLE();
  delay(100);
  
  Serial.println("1- Go to https://lucaelin.github.io/gps-ble");
  Serial.println("2- Connect to neo6m");
  Serial.println("3- Profit!");
}

void setupBLE() {
  BLEDevice::init("neo6m2");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);
  
  historyGPSCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_HIS_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );
  historyGPSCharacteristic->addDescriptor(new BLE2902());
  historyGPSCharacteristic->setCallbacks(new GpsCallbacks());

  currentGPSCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_GPS_UUID,
                                         BLECharacteristic::PROPERTY_READ | 
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  currentGPSCharacteristic->addDescriptor(new BLE2902());
  currentGPSCharacteristic->setValue((uint8_t*) &last_gps, sizeof(GpsData));
  
  pService->start();

  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}

void loop() {
  while (gps.available( Serial1 )) {
    NeoGPS::Location_t last_location( last_gps.lat, last_gps.lng);
    
    boolean firstTimeSync = false;
    gps_fix fix = gps.read();

    if (last_gps.status != fix.status) {
      Serial.print("GPS status change: ");
      Serial.println(fix.status);
    }
    last_gps.status = fix.status;

    if (fix.valid.date && fix.valid.time)
    {
      if (last_gps.time == 0) Serial.println("Got valid time");
      firstTimeSync = last_gps.time == 0;
      last_gps.time = (NeoGPS::clock_t) fix.dateTime;
    }

    if (fix.valid.satellites)
    {
      last_gps.sats = fix.satellites;
    }

    if (fix.valid.location)
    {
      last_gps.lat = fix.latitude();
      last_gps.lng = fix.longitude();
      last_gps.err_lat = ceil(fix.lat_err());
      last_gps.err_lng = ceil(fix.lon_err());
    }

    if (fix.valid.altitude)
    {
      last_gps.alt = fix.altitude();
    }
    
    float travelled = fix.location.DistanceKm( last_location ) * 1000;
    
    currentGPSCharacteristic->setValue((uint8_t*)&last_gps, sizeof(GpsData));
    currentGPSCharacteristic->notify();
    delay(3);


    if (!fix.valid.location) continue;
    if (travelled < max(last_gps.err_lat, last_gps.err_lng) * 2) continue;

    history_gps[history_index] = last_gps;
    history_index = (history_index + 1) % history_length;
    
    gps_log = FFat.open("/gps_log.bin", FILE_APPEND);
    if (!gps_log) {
      Serial.println("There was an error opening the file for writing");
      continue;
    }
    gps_log.write((uint8_t*) &last_gps, sizeof(GpsData));
    gps_log.close();
    
    Serial.println("Wrote location to history");

  }

}

void loadGpsLog() {
  gps_log = FFat.open("/gps_log.bin");
  if (!gps_log) {
    Serial.println("There was an error opening the file for reading");
    return;
  }
  while (gps_log.available()) {
    GpsData new_gps;
    if(!gps_log.read((uint8_t*) &new_gps, sizeof(GpsData))) continue;
    
    history_gps[history_index] = new_gps;
    history_index = (history_index + 1) % history_length;
    Serial.print(".");
  }
  Serial.println("");
  gps_log.close();
  Serial.println("Read locations from file");
}
