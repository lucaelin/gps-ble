#include <NMEAGPS.h>
#include "FS.h"
#include "FFat.h"

#include "data.h"
#include "file.h"
#include "ble.h"

#define TBEAM

#ifdef TBEAM
static const int RXPin = 34, TXPin = 12;
#else
static const int RXPin = 14, TXPin = 12;
#endif
static const uint32_t GPSBaud = 9600;

NMEAGPS gps;

GpsData lastStoredGps;
NeoGPS::Location_t lastStoredLocation;
GpsData previousGps;
NeoGPS::Location_t previousLocation;
GpsData currentGps;
NeoGPS::Location_t currentLocation;

#ifdef TBEAM
#include "axp20x.h"
AXP20X_Class axp;
#define I2C_SDA         21
#define I2C_SCL         22
#endif

// https://www.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_(GPS.G6-SW-10018)_Public.pdf?utm_source=en/images/downloads/Product_Docs/u-blox6_ReceiverDescriptionProtocolSpec_(GPS.G6-SW-10018).pdf
// page 138
// or https://github.com/SlashDevin/NeoGPS/blob/master/examples/ubloxRate/ubloxRate.ino#L130
char ubxSpeed[] =
{ 0x06, 0x08, 0x06, 0x00, 0xd0, 0x07, 0x01, 0x00, 0x01, 0x00 };

void setUBXSpeed(uint16_t ms) {
  uint8_t* v = (uint8_t*)&ms;
  ubxSpeed[4] = v[0];
  ubxSpeed[5] = v[1];
  sendUBX(ubxSpeed, 10);
  Serial.print("Setting ubx speed to ");
  Serial.println((int) ms);
}

uint32_t stationaryUpdates = 0;
float lineDeviation = 0;

void sendUBX( char *bytes, size_t len )
{
  Serial1.write( 0xB5 ); // SYNC1
  Serial1.write( 0x62 ); // SYNC2

  uint8_t a = 0, b = 0;
  uint8_t i = 0;
  while (i < len) {
    uint8_t c = bytes[i];
    a += c;
    b += a;
    i++;
  }

  Serial1.write( a ); // CHECKSUM A
  Serial1.write( b ); // CHECKSUM B

  delay(10);

} // sendUBX

bool isStationary(NeoGPS::Location_t a, NeoGPS::Location_t b, uint8_t err_lat, uint8_t err_lng) {
  // calculate distances
  float travelled = a.DistanceKm( b ) * 1000;
  float bearing = a.BearingTo( b );
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
  setupTTN();
#endif


  Serial1.begin(GPSBaud, (uint32_t) SERIAL_8N1, RXPin, TXPin);

  // include location error data (GST sentences)
  delay(1000);
  gps.send_P( &Serial1, F("PUBX,40,GST,0,1,0,0,0,0") );
  delay(100);
  /*
    Serial1.println(F("$PUBX,40,VTG,0,0,0,0*5E")); //VTG OFF
    delay(100);
    Serial1.println(F("$PUBX,40,GGA,0,0,0,0*5A")); //GGA OFF
    delay(100);
    Serial1.println(F("$PUBX,40,GSA,0,0,0,0*4E")); //GSA OFF
    delay(100);
    Serial1.println(F("$PUBX,40,GSV,0,0,0,0*59")); //GSV OFF
    delay(100);
    Serial1.println(F("$PUBX,40,GLL,0,0,0,0*5C")); //GLL OFF
    delay(100);
  */
  setUBXSpeed(2000);

  delay(1000);

  //FFat.format();
  Serial.setDebugOutput(true);
  if (!FFat.begin(true)) {
    Serial.println("FFat Mount Failed");
    return;
  }

  loadGpsHistory();

  delay(100);
  setupBLE();
  delay(100);

  Serial.println("1 - Go to https://lucaelin.github.io/gps-ble");
  Serial.println("2 - Connect to neo6m");
  Serial.println("3 - Profit!");
}

Situation processGps() {
    // decrease update speed if stationary for a while
    stationaryUpdates++;
    if (stationaryUpdates == 60) setUBXSpeed( 4000 );
    if (stationaryUpdates == 200) {
      setUBXSpeed( 10000 );
      #ifdef TBEAM
      sendTTN(currentGps);
      #endif

      return SIGNIFICANT;
    }

    if (lastStoredGps.status != NOT_VALID && isStationary(lastStoredLocation, currentLocation, currentGps.err_lat, currentGps.err_lng)) {
      return STATIONARY;
    }
    // not stationary anymore

    // increase update speed again
    if (stationaryUpdates > 2) setUBXSpeed( 2000 );
    // reset counter
    stationaryUpdates = 0;

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
  while (gps.available( Serial1 )) {
    previousGps = currentGps;
    previousLocation = NeoGPS::Location_t( previousGps.lat, previousGps.lng );

    currentGps = parseFix(gps.read());
    currentLocation = NeoGPS::Location_t( currentGps.lat, currentGps.lng );

    if (currentGps.status == NOT_VALID || previousGps.status == NOT_VALID) continue;

    Situation s = processGps();

    currentGps.status = s;

    if (s == SIGNIFICANT) {
      // save to flash
      storeGpsEntry(&previousGps);

      lastStoredGps = previousGps;
      lastStoredLocation = previousLocation;
      Serial.println("Wrote location to history");
    } else {
      Serial.print(s);
      Serial.println(", skipping location.");
    }
    // update possibly connected ble device
    currentGPSCharacteristic->setValue((uint8_t*)&currentGps, sizeof(GpsData));
    currentGPSCharacteristic->notify();
    delay(3);

  }
}
