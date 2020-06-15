import 'https://unpkg.com/leaflet-ant-path@1.3.0/dist/leaflet-ant-path.js';
import 'https://unpkg.com/@surma/structured-data-view@0.0.2/dist/structured-data-view.umd.js';
const {StructuredDataView, ArrayOfStructuredDataViews} = structuredDataView;

import {plotPath, plotPosition, dom} from './map.js';
import {StatusData, GpsData, GpsStatus, Y2KtoDate} from './GpsData.js';

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
    .filter(l=>l.status >= GpsStatus.STATUS_STD)
    .sort((a,b)=>a.time-b.time)

  console.log("History consists of", historyClean.length, "points");
  if(historyClean.length) console.log("since", Y2KtoDate(historyClean[0].time));

  plotPath(historyClean);
}

async function startContinuousUpdates(ccurrent) {
  const update = (v)=>{
    const gpsData = new StructuredDataView(v.buffer, GpsData);
    plotPosition(gpsData);
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
  console.log(buffer);
  const statusData = new StructuredDataView(buffer, StatusData);
  console.log(JSON.stringify(statusData, null, 4));
}
