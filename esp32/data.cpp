#include "data.h"

bool pointInEllipse(int x, int y, int a, int b) {
  // checking the equation of
  // ellipse with the given point
  int p = (pow((x), 2) / pow(a, 2))
          + (pow((y), 2) / pow(b, 2));

  return p <= 1;
}

GpsData parseFix(gps_fix fix) {
  GpsData gpsData;

  Serial.print("status: ");
  Serial.print((int)fix.status);
  Serial.print(", err: ");
  Serial.print((int)(fix.valid.lat_err && fix.valid.lon_err));
  Serial.print((int)(fix.lat_err()));
  Serial.print(", loca: ");
  Serial.print((int)fix.valid.location);
  Serial.print("\n");
  
  bool isValid = fix.status >= 3 && fix.valid.location && fix.valid.lat_err && fix.valid.lon_err;
  gpsData.status = isValid ? SIGNIFICANT : NOT_VALID;

  if (fix.valid.date && fix.valid.time)
  {
    gpsData.time = (NeoGPS::clock_t) fix.dateTime;
  }

  if (fix.valid.location)
  {
    gpsData.lat = fix.latitude();
    gpsData.lng = fix.longitude();
    gpsData.err_lat = ceil(fix.lat_err());
    gpsData.err_lng = ceil(fix.lon_err());
  }

  return gpsData;
}

void storeGpsEntry(GpsData *entry) {
  NeoGPS::time_t time((clock_t) entry->time);
  char str[11];
  sprintf(str, "/%d.BIN", time.days());

  Serial.print("Writing gps to ");
  Serial.println(str);

  File gps_log = FFat.open(str, FILE_APPEND);
  if (!gps_log) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  gps_log.write((uint8_t*) entry, sizeof(GpsData));
  gps_log.close();

  Serial.printf("Free space: %10u\n", FFat.freeBytes());

  while (FFat.freeBytes() < sizeof(GpsData) * 1024) {
    Serial.printf("Free space below threshold, deleting old logs");
    deleteOldestFile();
  }
}
