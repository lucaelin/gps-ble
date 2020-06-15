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
  STATUS_NONE: 0,
  STATUS_EST: 1,
  STATUS_TIME_ONLY: 2,
  STATUS_STD: 3,
  STATUS_DGPS: 4,
  STATUS_RTK_FLOAT: 5,
  STATUS_RTK_FIXED: 6,
  STATUS_PPS: 7,
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
