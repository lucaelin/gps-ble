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


BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *currentGPSCharacteristic = NULL;
BLECharacteristic *historyGPSCharacteristic = NULL;
BLEAdvertising *pAdvertising = NULL;

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

struct GpsData {
  uint32_t time;
  uint32_t sats;
  float lat;
  float lng;
  float alt;
  float heading;
};

File gps_log;
NMEAGPS gps;
GpsData last_gps;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("BLE connected");
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("BLE disconnected");
    }
};

class GpsCallbacks: public BLECharacteristicCallbacks {
    void onRead (BLECharacteristic *pCharacteristic) {
      pCharacteristic->setValue((uint8_t*)&last_gps, sizeof(GpsData));
    }
};
class GpsCallbacks2: public BLECharacteristicCallbacks {
    void onRead (BLECharacteristic *pCharacteristic) {
      pCharacteristic->setValue((uint8_t*)&last_gps, sizeof(GpsData));
    }
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, (uint32_t) SERIAL_8N1, 14, 12);
  delay(100);

  BLEDevice::init("neo6m");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);

  currentGPSCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_GPS_UUID,
                                         BLECharacteristic::PROPERTY_READ | 
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  currentGPSCharacteristic->addDescriptor(new BLE2902());
  currentGPSCharacteristic->setValue((uint8_t*)&last_gps, sizeof(GpsData));
  
  historyGPSCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_HIS_UUID,
                                         BLECharacteristic::PROPERTY_READ | 
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  
  pService->start();

  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  delay(100);

  //FFat.format();
  Serial.setDebugOutput(true);
  if(!FFat.begin(true)){
      Serial.println("FFat Mount Failed");
      return;
  }

  Serial.println("1- Go to https://lucaelin.github.io/gps-ble");
  Serial.println("2- Connect to neo6m");
  Serial.println("3- Profit!");

}

void loop() {
  while (gps.available( Serial1 )) {
    boolean firstTimeSync = false;
    gps_fix fix = gps.read();

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
    }

    if (fix.valid.altitude)
    {
      last_gps.alt = fix.altitude();
    }

    if (fix.valid.heading)
    {
      last_gps.heading = fix.heading();
    }
    
    currentGPSCharacteristic->setValue((uint8_t*)&last_gps, sizeof(GpsData));
    currentGPSCharacteristic->notify();
    delay(3);

    if (!fix.valid.location && !firstTimeSync) continue;
    
    gps_log = FFat.open("/gps_log.bin", FILE_APPEND);
    if (!gps_log) {
      Serial.println("There was an error opening the file for writing");
      continue;
    }
    gps_log.write((uint8_t*) &last_gps, sizeof(GpsData));
    gps_log.close();
    Serial.println("Wrote location to file");

    /*

      if (fix.valid.speed)
      {
      Serial.print(F("SPEED:   KM/h="));
      Serial.println( fix.speed_kph() );
      }
    */

  }

}

void readGpsLog() {
  gps_log = FFat.open("/gps_log.bin");
  if (!gps_log) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  while (gps_log.available()) {
    GpsData new_gps;
    gps_log.read((uint8_t*) &new_gps, sizeof(GpsData));
    Serial.print(new_gps.time);
    Serial.print(", ");
  }
  Serial.print("\n");
  gps_log.close();
  Serial.println("Read locations from file");
}
