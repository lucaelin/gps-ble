import 'https://unpkg.com/leaflet-ant-path@1.3.0/dist/leaflet-ant-path.js';
import 'https://unpkg.com/@surma/structured-data-view@0.0.2/dist/structured-data-view.umd.js';
const {StructuredDataView, ArrayOfStructuredDataViews} = structuredDataView;

const connect = document.querySelector('button[name=connect]');

const dom = {
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

const ble = {
  service: "2e1d0001-cc74-4675-90a3-ec80f1037391",
  current: "2e1d0002-cc74-4675-90a3-ec80f1037391",
  history: "2e1d0003-cc74-4675-90a3-ec80f1037391",
}

const GpsData = {
  time:   StructuredDataView.Uint32({endianess: 'little'}),
  status: StructuredDataView.Uint8({endianess: 'little'}),
  sats:   StructuredDataView.Uint8({endianess: 'little'}),
  errLat: StructuredDataView.Uint8({endianess: 'little'}),
  errLng: StructuredDataView.Uint8({endianess: 'little'}),
  lat:    StructuredDataView.Float32({endianess: 'little'}),
  lng:    StructuredDataView.Float32({endianess: 'little'}),
  alt:    StructuredDataView.Float32({endianess: 'little'}),
};
console.log(GpsData);

const GpsStatus = {
  STATUS_NONE: 0,
  STATUS_EST: 1,
  STATUS_TIME_ONLY: 2,
  STATUS_STD: 3,
  STATUS_DGPS: 4,
  STATUS_RTK_FLOAT: 5,
  STATUS_RTK_FIXED: 6,
  STATUS_PPS: 7,
}

const GpsStatusLookup = Object.fromEntries(
  Object.entries(GpsStatus).map(([k,v])=>[v,k])
);

function Y2KtoDate(t) {
  const Y2K = 946684800;
  return new Date((Y2K + t) * 1000);
}

const mymap = L.map('mymap').setView([53.140, 8.23], 13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(mymap);

const currentLocation = L.ellipse([0, 0], [0, 0], 0, {}).addTo(mymap);
console.log("currentLocation", currentLocation);

const historyLine =L.polyline.antPath([], {
  delay: 2000,
}).addTo(mymap);
console.log("historyLine", historyLine);


const hopefullyDevice = new Promise((res, rej)=>{
  connect.addEventListener('click', async ()=>{

    console.log('Requesting any Bluetooth Device...');
    try {
      const device = await navigator.bluetooth.requestDevice({
        //filters: [{services: ['2e1d0001-cc74-4675-90a3-ec80f1037391']},],
        acceptAllDevices: true,
        optionalServices: Object.values(ble),
      });

      device.addEventListener('gattserverdisconnected', ()=>{
        console.warn("bluetooth connection lost");
        dom.status.textContent = "connection lost!";
      });
      res(device);
    } catch(e) {
      dom.status.textContent = e.message;
    }
  });
});


(async function init() {
  try {
    //console.log('Waiting for device selection');
    const device = await hopefullyDevice;

    dom.device.textContent = device.name;
    dom.status.textContent = 'connecting...';

    //console.log('Connecting to GATT Server...');
    const server = await device.gatt.connect();

    //console.log('Getting Service...');
    const service = await server.getPrimaryServices(ble.service).then(s=>s[0]);

    //console.log('Getting Characteristics...');
    const ccurrent = await service.getCharacteristics(ble.current).then(c=>c[0]);
    const chistory = await service.getCharacteristics(ble.history).then(c=>c[0]);

    //console.log('Loading history...');
    await downloadHistory(chistory);

    //console.log('Starting live update...');
    await startContinuousUpdates(ccurrent);
  } catch(e) {
    console.error(e);
    dom.status.textContent = e.message;
  }
})();

async function downloadHistory(chistory) {
  const historyChunks = [];
  while(true) {
    dom.status.textContent = "loading data... ("+historyChunks.length+")";
    const historyBuffer = await chistory.readValue();
    const chunk = new ArrayOfStructuredDataViews(historyBuffer.buffer, GpsData);
    //console.log(chunk, historyBuffer.buffer.byteLength);
    historyChunks.push(chunk);
    if(!chunk.length) break;
  }

  //console.log("chunks", historyChunks);
  const history = [].concat(...historyChunks);
  //console.log(history);

  const historyClean = history
    .sort((a,b)=>a.time-b.time)
    .filter(l=>l.status >= GpsStatus.STATUS_STD)

  console.log("History consists of", historyClean.length, "points");
  if(historyClean.length) console.log("since", Y2KtoDate(historyClean[0].time));

  historyClean.forEach(({lat, lng})=>historyLine.addLatLng([lat, lng]));

  const gpsData = history[history.length-1];
  if (!gpsData) return;

  dom.status.textContent = GpsStatusLookup[gpsData.status];
  if (gpsData.status < GpsStatus.STATUS_STD) return;

  currentLocation.setLatLng([gpsData.lat, gpsData.lng]);
  console.log(gpsData);
  currentLocation.setRadius([gpsData.errLng * 2, gpsData.errLat * 2]);
}

async function startContinuousUpdates(ccurrent) {
  const update = (v)=>{
    const gpsData = new StructuredDataView(v.buffer, GpsData);

    const date = Y2KtoDate(gpsData.time);

    dom.date.textContent = date.toLocaleDateString();
    dom.time.textContent = date.toLocaleTimeString();
    dom.sats.textContent = gpsData.sats;
    dom.lat.textContent = gpsData.lat.toFixed(5).padStart(9);
    dom.lng.textContent = gpsData.lng.toFixed(5).padStart(9);
    dom.alt.textContent = gpsData.alt.toFixed(1).padStart(5);
    dom.err.textContent = gpsData.errLat.toFixed(0).padStart(3) + " " + gpsData.errLng.toFixed(0).padStart(3);

    dom.status.textContent = GpsStatusLookup[gpsData.status];
    if (gpsData.status < GpsStatus.STATUS_STD) return;

    currentLocation.setLatLng([gpsData.lat, gpsData.lng]);
    currentLocation.setRadius([gpsData.errLng * 2, gpsData.errLat * 2]);
    historyLine.addLatLng([gpsData.lat, gpsData.lng]);
  }
  ccurrent.addEventListener('characteristicvaluechanged', (e)=>update(e.target.value));
  await ccurrent.startNotifications().catch(async e=>{
    console.log('fallback to interval based polling');
    update(await ccurrent.readValue());
    window.setInterval(async ()=>{
      update(await ccurrent.readValue());
    }, 5000)
  });
}
