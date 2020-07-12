import express from 'express';
import bodyParser from 'body-parser';
import bbo from 'buffer-backed-object';
import {GpsData} from './GpsData.js';

import * as db from './db.js';

const app = express();

app.use(bodyParser.raw({type: 'application/octet-stream'}));
app.use(bodyParser.json({type: 'application/json'}));
app.use(bodyParser.text({type: 'application/base64'}));

app.get('/data', async (req, res)=>{
  res.send(await db.query());
});

app.post('/upload', async (req, res)=>{
  await db.insertUpload({
    time: new Date(),
    query: req.query,
    headers: req.headers,
    body: req.body instanceof Buffer ? req.body.toString('base64') : req.body,
  });

  if (req.query.source === 'wifi' && req.body instanceof Buffer) {
    const rawLocations = new bbo.ArrayOfBufferBackedObjects(new Uint8Array(req.body).buffer, GpsData);
    const locations = rawLocations.map(l=>({
      dev: req.query.dev_id,
      src: req.query.source,
      receivedAt: new Date(),
      ...l,
    }));
    if (locations.length) await db.insertLocations(locations);
  } else if (req.query.source === 'ttn' && typeof req.body === 'object') {
    const payload = new Uint8Array(new Buffer(req.body.payload_raw, 'base64'));
    const rawLocations = new bbo.ArrayOfBufferBackedObjects(payload.buffer, GpsData);
    const locations = rawLocations.map(l=>({
      dev: req.body.dev_id,
      src: req.query.source,
      receivedAt: new Date(),
      ...l,
    }));
    if (locations.length) await db.insertLocations(locations);
  }
  console.log('got gps upload');

  res.send("OK");
});

app.use(express.static('client'));

app.listen(process.env.PORT, ()=>console.log('listening on port', process.env.PORT));
