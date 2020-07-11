import bbo from 'buffer-backed-object';
const {BufferBackedObject} = bbo;

export const StatusData = {
  flash_total:    BufferBackedObject.Uint32({endianess: 'little'}),
  flash_free:     BufferBackedObject.Uint32({endianess: 'little'}),
  history_length: BufferBackedObject.Uint32({endianess: 'little'}),
  logFile:        BufferBackedObject.UTF8String(12),
  logFileSize:    BufferBackedObject.Uint32({endianess: 'little'}),
}

export const GpsData = {
  time:   BufferBackedObject.Uint32({endianess: 'little'}),
  status: BufferBackedObject.Uint8({endianess: 'little'}),
  sats:   BufferBackedObject.Uint8({endianess: 'little'}),
  errLat: BufferBackedObject.Uint8({endianess: 'little'}),
  errLng: BufferBackedObject.Uint8({endianess: 'little'}),
  lat:    BufferBackedObject.Float32({endianess: 'little'}),
  lng:    BufferBackedObject.Float32({endianess: 'little'}),
  odo:    BufferBackedObject.Float32({endianess: 'little'}),
};

export const GpsStatus = {
  NOT_VALID: 0,
  STATIONARY: 1,
  MOVING_STRAIGHT: 2,
  SIGNIFICANT: 3,
  STAY: 4,
  GEOFENCE_LEAVE: 5,
  GEOFENCE_ENTER: 6,
  GEOFENCE_STAY: 7,
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
  const dLng = degreesToRadians(gpsData2.lng-gpsData1.lng);

  const lat1 = degreesToRadians(gpsData1.lat);
  const lat2 = degreesToRadians(gpsData2.lat);

  const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
          Math.sin(dLng/2) * Math.sin(dLng/2) * Math.cos(lat1) * Math.cos(lat2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
  return earthRadiusKm * c;
}
