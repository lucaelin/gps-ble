/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID            "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_GPS_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID     "a7045ffc-6ed3-4ec9-ace2-5cdfeff5101d"

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

struct GpsData {
  uint32_t lastDate;
  uint32_t lastTime;
  uint32_t lastSats;
  uint32_t lastAge;
  double lastLat;
  double lastLng;
  double lastAlt;
};

GpsData gpsData;
std::string currentValue = "Hello world";

TinyGPSPlus gps;
BLEService *pService;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

class MyCallbacks: public BLECharacteristicCallbacks {
    void onRead (BLECharacteristic *pCharacteristic) {
      pCharacteristic->setValue(currentValue);
    }
    void onWrite(BLECharacteristic *pCharacteristic) {
      currentValue = pCharacteristic->getValue();

      if (currentValue.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < currentValue.length(); i++)
          Serial.print(currentValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


class GpsCallbacks: public BLECharacteristicCallbacks {
    void onRead (BLECharacteristic *pCharacteristic) {
      pCharacteristic->setValue((uint8_t*)&gpsData, sizeof(GpsData));
    }
};

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println("1- Go to https://lucaelin.github.io/gps-ble");
  Serial.println("2- Connect to neo6m");
  Serial.println("3- Profit!");

  BLEDevice::init("neo6m");
  BLEServer *pServer = BLEDevice::createServer();

  pService = pServer->createService(SERVICE_UUID);
  
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_GPS_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );

  pCharacteristic->setCallbacks(new GpsCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  // put your main code here, to run repeatedly:
  while (ss.available() > 0){
    gps.encode(ss.read());

    gpsData.lastDate = gps.date.value();
    gpsData.lastTime = gps.time.value();
    gpsData.lastSats = gps.satellites.value();
    gpsData.lastAge  = gps.location.age();
    
    if (gps.location.isUpdated()){
      gpsData.lastLat = gps.location.lat();
      gpsData.lastLng = gps.location.lng();
      gpsData.lastAlt = gps.altitude.meters();
    }
  }
}
