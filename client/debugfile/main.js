import {GpsData} from '../GpsData.js';

function getLocation(point) {
  const lat = point.lat || point.location.lat;
  const lng = point.lng || point.location.lng;

  return [lat, lng];
}
function getError(point) {
  const lat = point.errorLat || point.error_lat || point.errLat || point.error.lat;
  const lng = point.errorLng || point.error_lng || point.errLng || point.error.lng;

  return [lat, lng];
}

const supportedTypes = {
  'application/json': async (file) => JSON.parse(await file.text()),
  'application/octet-stream': async (file) => {
    return [...new structuredDataView.ArrayOfStructuredDataViews(await file.arrayBuffer(), GpsData)];
  },
};

const dom = {
  fileselect: document.querySelector('input[type=file][name="select"]'),
  status: document.querySelector('.status'),
  map: document.querySelector('mymap'),
  point: document.querySelector('#point pre'),
}

const map = L.map('mymap').setView([53.140, 8.23], 10);
console.log("map", map);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

dom.fileselect.addEventListener('change', async ()=>{
  const hopefullyFiles = [...dom.fileselect.files].map(f=>{
    const handle = supportedTypes[f.type];
    if (!handle) {
      alert('unsupported file type '+f.type);
      window.location.reload();
    }
    return handle(f);
  });

  const data = (await Promise.all(hopefullyFiles)).flat();

  dom.status.textContent = `loaded ${data.length} locations`

  render(data);
}, false);

const cursor = L.control.mousePosition().addTo(map);
function render(data) {
  const path = L.polyline.antPath([], {
    delay: 2000,
  }).addTo(map);
  path.on('mouseover', ()=>{
    const {lat: cLat, lng: cLng} = cursor.getLatLng();
    const cursorLocation = [cLat, cLng];

    const distances = data.map(point=>({distance: distanceKm(getLocation(point), cursorLocation), point: point}));
    const closest = distances.sort((a,b)=>a.distance - b.distance)[0];
    renderClosest(closest.point);
  });

  path.setLatLngs(
    data.map((point)=>getLocation(point))
  );
}

const currentLocation = L.ellipse([0, 0], [0, 0], 0, {}).addTo(map);
function renderClosest(point) {
  console.log('closest', point);
  dom.point.textContent = JSON.stringify(point, null, 2);
  currentLocation.setLatLng(getLocation(point));
  currentLocation.setRadius(getError(point));
}

function distanceKm([lat1, lng1], [lat2, lng2]) {
  const R = 6371e3; // metres
  const φ1 = lat1 * Math.PI/180; // φ, λ in radians
  const φ2 = lat2 * Math.PI/180;
  const Δφ = (lat2-lat1) * Math.PI/180;
  const Δλ = (lng2-lng1) * Math.PI/180;

  const a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
          Math.cos(φ1) * Math.cos(φ2) *
          Math.sin(Δλ/2) * Math.sin(Δλ/2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

  const d = R * c; // in metres
  return d / 1000;
}
