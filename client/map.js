import {GpsStatus, GpsStatusLookup, Y2KtoDate, distance, } from './GpsData.js';

export const dom = {
  device: document.querySelector('.connect'),
  status: document.querySelector('.status'),
  focus: document.querySelector('.focus'),

  date: document.querySelector('.date'),
  time: document.querySelector('.time'),
  sats: document.querySelector('.sats'),
  lat: document.querySelector('.lat'),
  lng: document.querySelector('.lng'),
  alt: document.querySelector('.alt'),
  dev: document.querySelector('.dev'),
  err: document.querySelector('.err'),
}

function setUi(name, value, valid=true) {
  const elem = dom[name];
  if (valid) {
    elem.textContent = value;
  } else {
    elem.textContent = '-';
  }
}

const colors = [
  "#FF0000",
  "#FF8800",
  "#00FF00",
  "#00FFFF",
  "#0000FF",
  "#FF00FF",
]

const mymap = L.map('mymap', {
    minZoom: 5,
    maxZoom: 20,
}).setView([53.140, 8.23], 13);
console.log("map", mymap);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(mymap);

const currentLocation = L.ellipse([0, 0], [0, 0], 0, {}).addTo(mymap);
console.log("currentLocation", currentLocation);

dom.focus.addEventListener('click', ()=>{
  if (currentLocation.getLatLng().lat === 0) return;
  mymap.setView(currentLocation.getLatLng(), mymap.getZoom());
});

const hoverLocation = L.ellipse([0, 0], [0, 0], 0, {}).addTo(mymap);
console.log("hoverLocation", hoverLocation);

const cursor = L.control.mousePosition().addTo(mymap);

const paths = [];
createPath();
console.log("paths", paths);

function createPath() {
  const ants = L.polyline.antPath([], {
    delay: 2000,
    color: colors[paths.length % colors.length],
  }).addTo(mymap);
  const path = {ants, data: []};
  paths.unshift(path);
  
  ants.on('mouseover', ()=>{
    const cursorLocation = cursor.getLatLng();

    const distances = path.data.map(point=>({distance: distance(point, cursorLocation), point: point}));
    const closest = distances.reduce((p,c)=>p.distance<c.distance?p:c);
    
    hoverLocation.setLatLng([closest.point.lat, closest.point.lng, ]);
    hoverLocation.setRadius([closest.point.errLat, closest.point.errLng, ]);
    console.log(closest.point);
  });
}

export function plotPosition(gpsData, realtime=false) {
  const date = Y2KtoDate(gpsData.time);
  const hasFix = gpsData.status != GpsStatus.NOT_VALID;

  if (realtime) {
    setUi('date', date.toLocaleDateString(), gpsData.time);
    setUi('time', date.toLocaleTimeString(), gpsData.time);
    setUi('sats', gpsData.sats);

    setUi('lat', gpsData.lat.toFixed(5).padStart(9), hasFix);
    setUi('lng', gpsData.lng.toFixed(5).padStart(9), hasFix);
    setUi('alt', gpsData.alt.toFixed(1).padStart(5), hasFix);

    setUi('err', gpsData.errLat.toFixed(0).padStart(3) + " " + gpsData.errLng.toFixed(0).padStart(3), hasFix && gpsData.errLat);

    setUi('status', GpsStatusLookup[gpsData.status]);
  }

  if (!hasFix) return;

  let path = paths.find((p)=>{
    if (!p.data.length) return false;
    const start = p.data[0];
    const end = p.data[p.data.length - 1];
    return (
      start.time < gpsData.time + 10*60 &&
      end.time > gpsData.time - 10*60
    );
  });

  if (!path) {
    createPath();
    path = paths[0];
  }

  // TODO optimize to insert at correct position instead of sorting
  if (!path.data.length) {
    path.data.push(gpsData);
  } else if (path.data[path.data.length - 1].time < gpsData.time) {
    path.data.push(gpsData);
  } else if (gpsData.time < path.data[0].time) {
    path.data.unshift(gpsData);
  } else {
    path.data.push(gpsData);
    path.data = path.data.sort((a,b)=>a.time-b.time);
  }

  path.ants.setLatLngs(
    path.data.map(({lat, lng})=>[lat, lng])
  );

  if (gpsData.status >= GpsStatus.STAY) {
    plotStop(gpsData);
  }

  if (realtime) {
    currentLocation.setLatLng([gpsData.lat, gpsData.lng]);
    currentLocation.setRadius([gpsData.errLng, gpsData.errLat]);

    if (mymap.distance(mymap.getCenter(), [gpsData.lat, gpsData.lng]) < 1400/mymap.getZoom()) {
      mymap.setView([gpsData.lat, gpsData.lng], mymap.getZoom());
    }
  }
}
export function plotPath(path) {
  path.forEach((p)=>plotPosition(p));
}

export function plotStop(gpsData) {
  const date = Y2KtoDate(gpsData.time);
  const marker = L.marker([gpsData.lat, gpsData.lng], {title: GpsStatusLookup[gpsData.status] + ' at '+date.toLocaleString()}).addTo(mymap);
  
  marker.on('mouseover', ()=>{
    hoverLocation.setLatLng([gpsData.lat, gpsData.lng, ]);
    hoverLocation.setRadius([gpsData.errLat, gpsData.errLng, ]);
    console.log(gpsData);
  });
}
