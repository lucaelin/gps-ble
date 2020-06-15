import {GpsStatus, GpsStatusLookup, Y2KtoDate, distance, } from './GpsData.js';

export const dom = {
  device: document.querySelector('.connect'),
  status: document.querySelector('.status'),

  date: document.querySelector('.date'),
  time: document.querySelector('.time'),
  sats: document.querySelector('.sats'),
  lat: document.querySelector('.lat'),
  lng: document.querySelector('.lng'),
  alt: document.querySelector('.alt'),
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
  "#FFFF00",
  "#00FF00",
  "#00FFFF",
  "#0000FF",
  "#FF00FF",
]

const mymap = L.map('mymap').setView([53.140, 8.23], 13);
console.log("map", mymap);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(mymap);

const currentLocation = L.ellipse([0, 0], [0, 0], 0, {}).addTo(mymap);
console.log("currentLocation", currentLocation);

const paths = [];
createPath();
console.log("paths", paths);

function createPath() {
  const path = L.polyline.antPath([], {
    delay: 2000,
    color: colors[paths.length % colors.length],
  }).addTo(mymap);
  paths.unshift({path});
}

export function plotPosition(gpsData) {
  const date = Y2KtoDate(gpsData.time);
  const hasFix = gpsData.status >= GpsStatus.STATUS_STD;

  setUi('date', date.toLocaleDateString(), gpsData.time);
  setUi('time', date.toLocaleTimeString(), gpsData.time);
  setUi('sats', gpsData.sats);

  setUi('lat', gpsData.lat.toFixed(5).padStart(9), hasFix);
  setUi('lng', gpsData.lng.toFixed(5).padStart(9), hasFix);
  setUi('alt', gpsData.alt.toFixed(1).padStart(5), hasFix);

  setUi('err', gpsData.errLat.toFixed(0).padStart(3) + " " + gpsData.errLng.toFixed(0).padStart(3), hasFix && gpsData.errLat);

  setUi('status', GpsStatusLookup[gpsData.status]);

  if (!hasFix) return;

  if (
    paths[0].latest && paths[0].latest.time < gpsData.time - 10*60 ||
    paths[0].latest && distance(gpsData, paths[0].latest) > 1000
  ) createPath();

  paths[0].path.addLatLng([gpsData.lat, gpsData.lng]);
  paths[0].latest = gpsData;

  currentLocation.setLatLng([gpsData.lat, gpsData.lng]);
  currentLocation.setRadius([gpsData.errLng * 2, gpsData.errLat * 2]);

  if (mymap.distance(mymap.getCenter(), [gpsData.lat, gpsData.lng]) < 1400/mymap.getZoom()) {
    mymap.setView([gpsData.lat, gpsData.lng], mymap.getZoom());
  }
}
export function plotPath(path) {
  path.forEach((p)=>plotPosition(p));
}
