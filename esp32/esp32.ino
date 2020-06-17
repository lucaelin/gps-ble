
#include <NMEAGPS.h>
#include "FS.h"
#include "FFat.h"

#include "data.h"
#include "file.h"
#include "ble.h"

static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;

NMEAGPS gps;

// https://www.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_(GPS.G6-SW-10018)_Public.pdf?utm_source=en/images/downloads/Product_Docs/u-blox6_ReceiverDescriptionProtocolSpec_(GPS.G6-SW-10018).pdf 
// page 138
// or https://github.com/SlashDevin/NeoGPS/blob/master/examples/ubloxRate/ubloxRate.ino#L130
const unsigned char ubxSpeed10000ms[] PROGMEM = 
  { 0x06,0x08,0x06,0x00,0x10,0x27,0x01,0x00,0x01,0x00 };
const unsigned char ubxSpeed4000ms[] PROGMEM = 
  { 0x06,0x08,0x06,0x00,0xA0,0x0F,0x01,0x00,0x01,0x00 };
const unsigned char ubxSpeed2000ms[] PROGMEM = 
  { 0x06,0x08,0x06,0x00,0xd0,0x07,0x01,0x00,0x01,0x00 };
const unsigned char ubxSpeed1000ms[] PROGMEM = 
  { 0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00 };

uint32_t skippedUpdates = 0;
  
void sendUBX( const unsigned char *progmemBytes, size_t len )
{
  Serial1.write( 0xB5 ); // SYNC1
  Serial1.write( 0x62 ); // SYNC2

  uint8_t a = 0, b = 0;
  while (len-- > 0) {
    uint8_t c = pgm_read_byte( progmemBytes++ );
    a += c;
    b += a;
    Serial1.write( c );
  }

  Serial1.write( a ); // CHECKSUM A
  Serial1.write( b ); // CHECKSUM B

  delay(10);

} // sendUBX
  
void setup() {
  Serial.begin(115200);
  Serial1.begin(GPSBaud, (uint32_t) SERIAL_8N1, 14, 12);

  // include location error data (GST sentences)
  delay(100);
  gps.send_P( &Serial1, F("PUBX,40,GST,0,1,0,0,0,0") );
  delay(100);
  sendUBX( ubxSpeed2000ms, sizeof(ubxSpeed2000ms) );

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

  Serial.println("1 - Go to https://lucaelin.github.io/gps-ble");
  Serial.println("2 - Connect to neo6m");
  Serial.println("3 - Profit!");
}

void loop() {
  while (gps.available( Serial1 )) {
    Serial.print(".");
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

    skippedUpdates++;
    if (skippedUpdates == 60) sendUBX( ubxSpeed4000ms, sizeof(ubxSpeed4000ms) );
    if (skippedUpdates == 200) sendUBX( ubxSpeed10000ms, sizeof(ubxSpeed10000ms) );
    
    if (pointInEllipse(travelled_lat, travelled_lng, gpsData.err_lat * 2, gpsData.err_lng * 2)) continue;

    if (skippedUpdates > 2) sendUBX( ubxSpeed2000ms, sizeof(ubxSpeed2000ms) );
    skippedUpdates = 0;

    storeGpsEntry(&gpsData);

    Serial.println("Wrote location to history");
  }
}
