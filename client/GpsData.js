const {StructuredDataView} = structuredDataView;

export const StatusData = {
  flash_total:    StructuredDataView.Uint32({endianess: 'little'}),
  flash_free:     StructuredDataView.Uint32({endianess: 'little'}),
  history_length: StructuredDataView.Uint32({endianess: 'little'}),
  logFile:        StructuredDataView.UTF8String(12),
  logFileSize:    StructuredDataView.Uint32({endianess: 'little'}),
}
console.log(StatusData);

export const GpsData = {
  time:   StructuredDataView.Uint32({endianess: 'little'}),
  status: StructuredDataView.Uint8({endianess: 'little'}),
  sats:   StructuredDataView.Uint8({endianess: 'little'}),
  errLat: StructuredDataView.Uint8({endianess: 'little'}),
  errLng: StructuredDataView.Uint8({endianess: 'little'}),
  lat:    StructuredDataView.Float32({endianess: 'little'}),
  lng:    StructuredDataView.Float32({endianess: 'little'}),
  alt:    StructuredDataView.Float32({endianess: 'little'}),
};

export const GpsStatus = {
  NOT_VALID: 0,
  STATIONARY: 1,
  MOVING_STRAIGHT: 2,
  SIGNIFICANT: 3,
  /*NONE: 0,
  EST: 1,
  TIME_ONLY: 2,
  STD: 3,
  DGPS: 4,
  RTK_FLOAT: 5,
  RTK_FIXED: 6,
  PPS: 7,*/
}

for (const [k, v] of Object.entries(GpsStatus)) {
  GpsStatus[k+'+'] = v+8;
}

export const GpsStatusLookup = Object.fromEntries(
  Object.entries(GpsStatus).map(([k,v])=>[v,k])
);

export function Y2KtoDate(t) {
  const Y2K = 946684800;
  return new Date((Y2K + t) * 1000);
}

export function degreesToRadians(degrees) {
  return degrees * Math.PI / 180;
}

export function distance(gpsData1, gpsData2) {
  const earthRadiusKm = 6371;

  const dLat = degreesToRadians(gpsData2.lat-gpsData1.lat);
  const dLon = degreesToRadians(gpsData2.lon-gpsData1.lon);

  const lat1 = degreesToRadians(gpsData1.lat);
  const lat2 = degreesToRadians(gpsData2.lat);

  const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
          Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
  return earthRadiusKm * c;
}
