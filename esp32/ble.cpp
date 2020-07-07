#include "ble.h"

BLEServer *pServer;
BLEService *pService;
BLEAdvertising *pAdvertising;

BLECharacteristic *currentGPSCharacteristic;
BLECharacteristic *historyGPSCharacteristic;
BLECharacteristic *statusCharacteristic;

uint32_t history_position;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("BLE connected");
    history_position = floor( getGPSLogSize() / sizeof(GpsData) ) * sizeof(GpsData);
  };

  void onDisconnect(BLEServer* pServer) {
    Serial.println("BLE disconnected");
    BLEDevice::startAdvertising();
  }
};

class GpsCallbacks: public BLECharacteristicCallbacks {
  void onRead (BLECharacteristic *pCharacteristic) {
    File history = getGPSLog();

    Serial.print("history_position: ");
    Serial.println(history_position);

    uint32_t max_chunk_size = floor(CHARACTERISTIC_MTU / sizeof(GpsData)) * sizeof(GpsData);

    if (history_position < max_chunk_size) max_chunk_size = history_position;
    history_position -= max_chunk_size;

    Serial.print("read starting at: ");
    Serial.println(history_position);

    Serial.print("read length: ");
    Serial.println(max_chunk_size);

    history.seek(history_position);

    if (!max_chunk_size) {
      pCharacteristic->setValue("");
      return;
    }

    uint8_t buffer[max_chunk_size];
    history.read(buffer, max_chunk_size);
    history.close();

    pCharacteristic->setValue(buffer, max_chunk_size);
  }
};

class StatusCallbacks: public BLECharacteristicCallbacks {
  void onRead (BLECharacteristic *pCharacteristic) {
    StatusData sts;

    Serial.printf("Total space: %10u\n", FFat.totalBytes());
    Serial.printf("Free space: %10u\n", FFat.freeBytes());
    sts.flash_total = FFat.totalBytes();
    sts.flash_free = FFat.freeBytes();

    File gpsLogFile = getGPSLog();
    Serial.print("Latest file: ");
    Serial.println(gpsLogFile.name());
    strcpy(sts.logFile, gpsLogFile.name());

    Serial.print("Latest file size: ");
    Serial.println(gpsLogFile.size());
    sts.logFileSize = gpsLogFile.size();

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
