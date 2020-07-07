#include "gps.h"

#ifdef TBEAM
static const int RXPin = 34, TXPin = 12;
#else
static const int RXPin = 14, TXPin = 12;
#endif
static const uint32_t GPSBaud = 9600;

NMEAGPS gps;

// https://www.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_(GPS.G6-SW-10018)_Public.pdf?utm_source=en/images/downloads/Product_Docs/u-blox6_ReceiverDescriptionProtocolSpec_(GPS.G6-SW-10018).pdf
// page 138
// or https://github.com/SlashDevin/NeoGPS/blob/master/examples/ubloxRate/ubloxRate.ino#L130
char ubxSpeed[] =
{ 0x06, 0x08, 0x06, 0x00, 0xd0, 0x07, 0x01, 0x00, 0x01, 0x00 };

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
    Serial1.write( c ); // C
    i++;
  }

  Serial1.write( a ); // CHECKSUM A
  Serial1.write( b ); // CHECKSUM B

  delay(10);

} // sendUBX

void setUBXSpeed(uint16_t ms) {
  uint8_t* v = (uint8_t*)&ms;
  ubxSpeed[4] = v[0];
  ubxSpeed[5] = v[1];
  sendUBX(ubxSpeed, 10);
  Serial.print("Setting ubx speed to ");
  Serial.println((int) ms);
}

void setupGPS() {
  Serial1.begin(GPSBaud, (uint32_t) SERIAL_8N1, RXPin, TXPin);

  while (!Serial1) ;

  Serial.print("Setting up GPS RX ");
  Serial.print((int) RXPin);
  Serial.print(", TX ");
  Serial.print((int) TXPin);
  Serial.println("");

  // include location error data (GST sentences)
  delay(1000);
  gps.send_P( &Serial1, F("PUBX,40,GST,0,1,0,0,0,0") );
  delay(100);

  Serial1.println(F("$PUBX,40,VTG,0,0,0,0*5E")); //VTG OFF
  delay(100);
  //Serial1.println(F("$PUBX,40,GGA,0,0,0,0*5A")); //GGA OFF
  //delay(100);
  Serial1.println(F("$PUBX,40,GSA,0,0,0,0*4E")); //GSA OFF
  delay(100);
  Serial1.println(F("$PUBX,40,GSV,0,0,0,0*59")); //GSV OFF
  delay(100);
  Serial1.println(F("$PUBX,40,GLL,0,0,0,0*5C")); //GLL OFF
  delay(100);

  setUBXSpeed(2000);
}