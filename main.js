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
    const device = await navigator.bluetooth.requestDevice({
      //filters: [{services: ['6e400001-b5a3-f393-e0a9-e50e24dcca9e']},],
      acceptAllDevices: true,
      optionalServices: ['6e400001-b5a3-f393-e0a9-e50e24dcca9e'],
    });

    res(device);
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

    // Note that we could also get all services that match a specific UUID by
    // passing it to getPrimaryServices().
    console.log('Getting Services...');
    const services = await server.getPrimaryServices();

    console.log('Getting Characteristics...');
    const hopefullyChars = services.map(async s=>await s.getCharacteristics());
    const characteristics = await Promise.all(hopefullyChars);

    await Promise.all(characteristics.flat().map(async c=>{
      const v = await c.readValue();
      const [date, time, sats, age] = new Uint32Array(v.buffer);
      const [lat, lng, alt] = new Float64Array(v.buffer).slice(2);

      dom.date.textContent = date.toString().padStart(6, '0').match(/.{1,2}/g).join('.');
      dom.time.textContent = time.toString().padStart(8, '0').match(/.{1,2}/g).join(':');
      dom.sats.textContent = sats;
      dom.lat.textContent = lat;
      dom.lng.textContent = lng;
      dom.alt.textContent = alt;
      console.log(date, time, sats, age);
      console.log(lat, lng, alt);
      currentLocation.setLatLng([lat,lng]);

      dom.status.textContent = age > 1500 ? 'waiting for gps fix' : 'gps fix';
    }));
  } catch(e) {
    console.error(e);
    dom.status.textContent = 'error...'
  }

  window.setTimeout(init, 5000);
})();
