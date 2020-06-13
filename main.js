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
}

const ble = {
  service: "2e1d0001-cc74-4675-90a3-ec80f1037391",
  current: "2e1d0002-cc74-4675-90a3-ec80f1037391",
  history: "2e1d0003-cc74-4675-90a3-ec80f1037391",
}

function decodeText(data) {
  var dec = new TextDecoder('utf-8');
  var arr = new Uint8Array(data);
  return dec.decode(arr);
}
function encodeText(text) {
  var enc = new TextEncoder();
  return enc.encode(text);
}

const hopefullyDevice = new Promise((res, rej)=>{
  connect.addEventListener('click', async ()=>{

    console.log('Requesting any Bluetooth Device...');
    try {
      const device = await navigator.bluetooth.requestDevice({
        //filters: [{services: ['2e1d0001-cc74-4675-90a3-ec80f1037391']},],
        acceptAllDevices: true,
        optionalServices: Object.values(ble),
      });

      res(device);
    } catch(e) {
      dom.status.textContent = e.message;
    }
  });
});

const mymap = L.map('mymap').setView([53.140, 8.23], 13);

L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(mymap);
const currentLocation = L.marker([0, 0]).addTo(mymap);


(async function init() {
  try {
    console.log('Waiting for device selection');
    const device = await hopefullyDevice;

    dom.device.textContent = device.name;
    dom.status.textContent = 'connecting...';

    console.log('Connecting to GATT Server...');
    const server = await device.gatt.connect();

    console.log('Getting Service...');
    const service = await server.getPrimaryServices(ble.service).then(s=>s[0]);

    console.log('Getting Characteristics...');
    const ccurrent = await service.getCharacteristics(ble.current).then(c=>c[0]);
    const chistory = await service.getCharacteristics(ble.history).then(c=>c[0]);

    console.log(ccurrent, chistory);
    await ccurrent.startNotifications();
    ccurrent.addEventListener('characteristicvaluechanged', (e)=>{
      const v = e.target.value;
      const [time, sats] = new Uint32Array(v.buffer);
      const [lat, lng, alt] = new Float32Array(v.buffer).slice(2);

      // time is relative to Y2K
      const Y2K = 946684800;
      const date = new Date((Y2K + time) * 1000);

      dom.date.textContent = date.toLocaleDateString();
      dom.time.textContent = date.toLocaleTimeString();
      dom.sats.textContent = sats;
      dom.lat.textContent = lat.toFixed(5).padStart(9);
      dom.lng.textContent = lng.toFixed(5).padStart(9);
      dom.alt.textContent = alt.toFixed(1).padStart(5);
      console.log(date, time, sats);
      console.log(lat, lng, alt);
      currentLocation.setLatLng([lat,lng]);

      dom.status.textContent = lat === 0 && lng === 0 ? 'waiting for gps fix' : 'gps fix';
    });
  } catch(e) {
    console.error(e);
    dom.status.textContent = e.message;
  }
})();
