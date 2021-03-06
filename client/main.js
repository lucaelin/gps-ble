import {BufferBackedObject, ArrayOfBufferBackedObjects} from 'https://unpkg.com/buffer-backed-object@0.2.2/dist/buffer-backed-object.modern.js';

import {plotPath, plotPosition, plotStop, dom} from './map.js';
import {StatusData, GpsData, GpsStatus, Y2KtoDate} from './GpsData.js';

//import './test.js';

const connect = document.querySelector('button[name=connect]');

const ble = {
  service: "2e2d0001-cc74-4675-90a3-ec80f1037391",
  current: "2e2d0002-cc74-4675-90a3-ec80f1037391",
  history: "2e2d0003-cc74-4675-90a3-ec80f1037391",
  status:  "2e2d0004-cc74-4675-90a3-ec80f1037391",
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
    dom.status.textContent = 'loading TTN data...';
    await downloadData();
    dom.status.textContent = 'Ready to connect';
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
    const cstatus = await service.getCharacteristics(ble.status).then(c=>c[0]);

    await getStatus(cstatus);

    //console.log('Starting live update...');
    await startContinuousUpdates(ccurrent);

    //console.log('Loading history...');
    await downloadHistory(chistory);

  } catch(e) {
    console.error(e);
    dom.status.textContent = e.message;
  }
})();

async function downloadHistory(chistory) {
  let count = 0;
  while(true) {
    dom.status.textContent = "loading data... ("+count+")";
    const historyBuffer = await chistory.readValue();
    if (!historyBuffer.byteLength) break;
    const chunk = new ArrayOfBufferBackedObjects(historyBuffer.buffer, GpsData);

    plotPath(chunk
      .filter(l=>l.status != GpsStatus.NOT_VALID)
      .sort((a,b)=>a.time-b.time)
    );
    count += chunk.length;
  }
  dom.status.textContent = "loading data complete ("+count+")";
}

async function startContinuousUpdates(ccurrent) {
  const update = (v)=>{
    const gpsData = new BufferBackedObject(v.buffer, GpsData);
    plotPosition(gpsData, true);
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

async function getStatus(cstatus) {
  const {buffer} = await cstatus.readValue();
  const statusData = new BufferBackedObject(buffer, StatusData);
  console.log(JSON.stringify(statusData, null, 4));
}

async function downloadData() {
  const data = await fetch('./data').then(res=>res.json()).catch(e=>{
    console.warn('did not get data response from backend..', e);
    return [];
  });

  if(!data.length) return;
  console.log(data);
  plotPath(data);
}
