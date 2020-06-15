#include "ble.h"

BLEServer *pServer;
BLEService *pService;
BLEAdvertising *pAdvertising;

BLECharacteristic *currentGPSCharacteristic;
BLECharacteristic *historyGPSCharacteristic;
BLECharacteristic *statusCharacteristic;

uint32_t history_transfer_index;


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

class StatusCallbacks: public BLECharacteristicCallbacks {
  void onRead (BLECharacteristic *pCharacteristic) {
    StatusData sts;
    
    Serial.printf("Total space: %10u\n", FFat.totalBytes());
    Serial.printf("Free space: %10u\n", FFat.freeBytes());
    sts.flash_total = FFat.totalBytes();
    sts.flash_free = FFat.freeBytes();

    sts.history_length = history_length;
    
    File latestLogFile = getLatestFile();
    strcpy(sts.logFile, latestLogFile.name());
    
    pCharacteristic->setValue( (uint8_t*)&sts, sizeof(StatusData) );
  }
};

void setupBLE() {
  BLEDevice::init("neo6m");
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
  
  statusCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_STS_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );
  statusCharacteristic->addDescriptor(new BLE2902());
  statusCharacteristic->setCallbacks(new StatusCallbacks());
  
  pService->start();

  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}
