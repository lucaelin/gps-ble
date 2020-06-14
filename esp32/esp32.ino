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
#define CHARACTERISTIC_MTU 200

BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *currentGPSCharacteristic = NULL;
BLECharacteristic *historyGPSCharacteristic = NULL;
BLEAdvertising *pAdvertising = NULL;

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

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

File gps_log;
NMEAGPS gps;
const uint32_t history_length = 1000;
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
    //Serial.print("max_entries: ");
    //Serial.println(max_entries);
    //Serial.print("history_length: ");
    //Serial.println(history_length);
    //Serial.print("history_transfer_index: ");
    //Serial.println(history_transfer_index);
    uint32_t num_entries = min(max_entries, history_length-history_transfer_index);
    //Serial.print("num_entries: ");
    //Serial.println(num_entries);
    pCharacteristic->setValue((uint8_t*)&history_gps[history_transfer_index], num_entries * sizeof(GpsData));
    history_transfer_index += num_entries;
    //Serial.println("Transmission complete");
  }
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, (uint32_t) SERIAL_8N1, 14, 12);
  
  // include location error data (GST sentences)
  delay(100);
  gps.send_P( &Serial1, F("PUBX,40,GST,0,1,0,0,0,0") );
  
  delay(1000);

  //FFat.format();
  Serial.setDebugOutput(true);
  if(!FFat.begin(true)){
      Serial.println("FFat Mount Failed");
      return;
  }

  loadGpsHistory();

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
    GpsData previousGpsData = getPreviousEntry();
    GpsData gpsData = parseFix(gps.read());
   
    currentGPSCharacteristic->setValue((uint8_t*)&gpsData, sizeof(GpsData));
    currentGPSCharacteristic->notify();
    delay(3);

    NeoGPS::Location_t previousGpsData_location( previousGpsData.lat, previousGpsData.lng);
    NeoGPS::Location_t gpsData_location( gpsData.lat, gpsData.lng);
    float travelled = gpsData_location.DistanceKm( previousGpsData_location ) * 1000;
    float bearing = gpsData_location.BearingTo( previousGpsData_location );
    float travelled_lat = cos(bearing) * travelled;
    float travelled_lng = sin(bearing) * travelled;
    if (gpsData.status < 3) continue;
    if (pointInEllipse(travelled_lat, travelled_lng, gpsData.err_lat * 2, gpsData.err_lng * 2)) continue;

    storeGpsEntry(&gpsData);
    
    Serial.println("Wrote location to history");
  }
}


boolean pointInEllipse(int x, int y, int a, int b) 
{ 
  // checking the equation of 
  // ellipse with the given point 
  int p = (pow((x), 2) / pow(a, 2)) 
          + (pow((y), 2) / pow(b, 2)); 

  return p <= 1; 
} 

GpsData parseFix(gps_fix fix) {
  GpsData gpsData;
  
  gpsData.status = fix.status;

  if (fix.valid.date && fix.valid.time)
  {
    gpsData.time = (NeoGPS::clock_t) fix.dateTime;
  }

  if (fix.valid.satellites)
  {
    gpsData.sats = fix.satellites;
  }

  if (fix.valid.location)
  {
    gpsData.lat = fix.latitude();
    gpsData.lng = fix.longitude();
    gpsData.err_lat = ceil(fix.lat_err());
    gpsData.err_lng = ceil(fix.lon_err());
  }

  if (fix.valid.altitude)
  {
    gpsData.alt = fix.altitude();
  }

  return gpsData;
}

boolean isGpsLogFile( const char *string )
{
  string = strrchr(string, '.');

  if( string != NULL )
    return ( strcmp(string, ".bin") == 0 );

  return false;
}
File getLatestFile() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return root;
  }
  
  File file = root.openNextFile();
  const char *name = "/";
  while(file){
    Serial.print("Found file: ");
    Serial.print(strcmp(file.name(), name));
    Serial.println(file.name());
    if(!file.isDirectory() && isGpsLogFile(file.name())){
      if(strcmp(file.name(), name) > 0) {
        name = file.name();
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
  
  Serial.println("Found latest file: ");
  Serial.println(name);
  return FFat.open(name);
}

void loadGpsHistory() {
  File gps_log = getLatestFile();
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

void storeGpsEntry(GpsData *entry) {
  history_gps[history_index] = *entry;
  history_index = (history_index + 1) % history_length;
  
  NeoGPS::time_t time((clock_t) entry->time);
  char str[11];
  sprintf(str, "/%d.bin", time.days());

  Serial.print("Writing gps to ");
  Serial.println(str);
  
  gps_log = FFat.open(str, FILE_APPEND);
  if (!gps_log) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  gps_log.write((uint8_t*) entry, sizeof(GpsData));
  gps_log.close();
  
  Serial.printf("Free space: %10u\n", FFat.freeBytes());
}

GpsData getPreviousEntry() {
  int32_t i = history_index - 1;
  if (i<0) i += history_length;
  
  return history_gps[i];
}
