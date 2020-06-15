
#include <NMEAGPS.h>
#include "FS.h"
#include "FFat.h"

#include "data.h"
#include "file.h"
#include "ble.h"

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

NMEAGPS gps;


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
