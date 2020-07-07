#include "FS.h"
#include "FFat.h"
#include <NMEAGPS.h>

#define TBEAM
//#define BLE

#include "data.h"
#include "file.h"
#include "gps.h"
#ifdef BLE
#include "ble.h"
#endif
#include "wifi.h"

GpsData lastStoredGps;
NeoGPS::Location_t lastStoredLocation;
GpsData previousGps;
NeoGPS::Location_t previousLocation;
GpsData currentGps;
NeoGPS::Location_t currentLocation;

uint32_t stationaryUpdates = 0;
float lineDeviation = 0;

#ifdef TBEAM
#include "axp20x.h"
AXP20X_Class axp;
#define I2C_SDA         21
#define I2C_SCL         22
#endif

bool isInsideGeofence(NeoGPS::Location_t a, NeoGPS::Location_t b, uint8_t err_lat, uint8_t err_lng, float rot) {
  // calculate distances
  float travelled = a.DistanceKm( b ) * 1000;
  float bearing = a.BearingTo( b ) - rot;
  float travelled_lat = cos(bearing) * travelled;
  float travelled_lng = sin(bearing) * travelled;

  // check if stationary
  return pointInEllipse(travelled_lat, travelled_lng, err_lat * 2, err_lng * 2);
}

float calcDeviation(NeoGPS::Location_t a, NeoGPS::Location_t b, NeoGPS::Location_t c) {
  //Serial.println("calcDeviation");
  float bearing_ab = PI - a.BearingTo(b);
  //Serial.print("ab: ");
  //Serial.println(bearing_ab);
  float bearing_ac = PI - a.BearingTo(c);
  //Serial.print("ac: ");
  //Serial.println(bearing_ac);
  float bearing_diff = bearing_ab - bearing_ac;
  //Serial.print("diff: ");
  //Serial.println(bearing_diff);
  float bearing_mod = bearing_diff + (bearing_diff / abs(bearing_diff)) * (-2.0 * PI);
  //Serial.print("mod: ");
  //Serial.println(bearing_mod);
  float bearing_change = bearing_diff > PI || bearing_diff < -PI ? bearing_mod : bearing_diff;
  //Serial.print("change: ");
  //Serial.println(bearing_change);
  float distance_ac = a.DistanceKm(b) * 1000;
  //Serial.print("ac_dist: ");
  //Serial.println(distance_ac);
  float offset = sin(bearing_change) * distance_ac;
  //Serial.print("offset: ");
  //Serial.println(offset);
  return offset;
}

void setup() {
  Serial.begin(115200);

#ifdef TBEAM
  Serial.println("TBEAM PMU setup");
  Wire.begin(I2C_SDA, I2C_SCL);
  axp.begin(Wire, AXP192_SLAVE_ADDRESS);
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);  // lora
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON); // maybe related to lora
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);  // gps
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON); // ?
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED pins + some other
  axp.setDCDC1Voltage(3300);

  Serial.printf("DCDC1: %s\n", axp.isDCDC1Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC2: %s\n", axp.isDCDC2Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("LDO2: %s\n", axp.isLDO2Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("LDO3: %s\n", axp.isLDO3Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC3: %s\n", axp.isDCDC3Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("Exten: %s\n", axp.isExtenEnable() ? "ENABLE" : "DISABLE");

  delay(1000);

  Serial.println("TTN setup");
  setupTTN();
  delay(100);
#endif

  Serial.println("GPS setup");
  setupGPS();
  delay(100);

  //FFat.format();
  if (!FFat.begin(true)) {
    Serial.println("FFat Mount Failed");
    return;
  }

  Serial.println("HISTORY setup");
  loadGpsHistory();
  delay(100);

#ifdef BLE
  Serial.println("BLE setup");
  setupBLE();
  delay(100);
#endif

  Serial.println("WIFI setup");
  setupWIFI();
  delay(100);

  Serial.println("1 - Go to https://gps-ble.herokuapp.com");
  Serial.println("2 - Connect to neo6m");
  Serial.println("3 - Profit!");
}

Situation processGps() {
    if (lastStoredGps.status == NOT_VALID) {
      return SIGNIFICANT;
    }

    Geofence fence = {
      .lat = 53.14021301269531,
      .lng = 8.180638313293457,
      .width = 30.0,
      .height = 30.0,
      .rot = 0.0,
    };

    NeoGPS::Location_t fenceLocation( fence.lat, fence.lng );
    boolean currentlyInside = isInsideGeofence(currentLocation, fenceLocation, fence.height, fence.width, fence.rot);
    boolean previouslyInside = isInsideGeofence(previousLocation, fenceLocation, fence.height, fence.width, fence.rot);

    // decrease update speed if stationary for a while
    stationaryUpdates++;
    if (stationaryUpdates == 60) setUBXSpeed( 4000 );
    if (stationaryUpdates == 200) {
      setUBXSpeed( 10000 );
      return currentlyInside?GEOFENCE_STAY:STAY;
    }

    if (isInsideGeofence(currentLocation, lastStoredLocation, currentGps.err_lat, currentGps.err_lng, 0.0)) {
      return STATIONARY;
    }
    // not stationary anymore

    // increase update speed again
    if (stationaryUpdates > 2) setUBXSpeed( 2000 );
    // reset counter
    stationaryUpdates = 0;

    if (currentlyInside != previouslyInside) {
      return currentlyInside?GEOFENCE_ENTER:GEOFENCE_LEAVE;
    }

    float newDeviation = calcDeviation(lastStoredLocation, previousLocation, currentLocation);
    lineDeviation += newDeviation;

    if (!lastStoredGps.status || currentGps.time - lastStoredGps.time > 60 || abs(newDeviation) > 0.5 || abs(lineDeviation) > 2.0) {
      lineDeviation = newDeviation;
      return SIGNIFICANT;
    } else {
      return MOVING_STRAIGHT;
    }
}

void loop() {
  if (gps.available( Serial1 )) {
    previousGps = currentGps;
    previousLocation = NeoGPS::Location_t( previousGps.lat, previousGps.lng );

    currentGps = parseFix(gps.read());
    currentLocation = NeoGPS::Location_t( currentGps.lat, currentGps.lng );

    if (currentGps.status == NOT_VALID || previousGps.status == NOT_VALID) {
      Serial.println("GPS not valid yet...");
      return;
    }

    Situation s = processGps();

    currentGps.status = s;

    Serial.print("GPS status: ");
    Serial.println(s);

    if (previousGps.status >= SIGNIFICANT) {
      // save to flash
      storeGpsEntry(&previousGps);

      lastStoredGps = previousGps;
      lastStoredLocation = previousLocation;
      Serial.println("Wrote location to history");

#ifdef TBEAM
      if (previousGps.status >= STAY) {
        sendTTN(previousGps);
        Serial.println("Sent location to ttn");
        uploadWIFI();
        Serial.println("Sent location to wifi");
      }
#endif
    } else {
      Serial.println("Skipping location.");
    }

#ifdef BLE
    // update possibly connected ble device
    currentGPSCharacteristic->setValue((uint8_t*)&currentGps, sizeof(GpsData));
    currentGPSCharacteristic->notify();
#endif
    delay(3);
  }

  if (Serial.available()) {
    char i = (char)Serial.read();
    while (Serial.available()) Serial.read();
    
    if (i=='h') {
      Serial.println("command h: help");
      Serial.println("command t: sendTTN");
      Serial.println("command u: uploadWIFI");
      Serial.println("command r: ESP.restart()");
    } else if (i=='t') {
      Serial.println("command t: sendTTN(currentGps)");
      sendTTN(currentGps);
    } else if (i=='u') {
      Serial.println("command u: uploadWIFI()");
      uploadWIFI();
    } else if (i=='r') {
      Serial.println("command r: ESP.restart()");
      ESP.restart();
    }
  }
}
