import express from 'express';
import fetch from 'node-fetch';
import dotenv from 'dotenv';
dotenv.config();

const app = express();

const ttnBackend = {
  headers: {
    Accept: 'application/json',
    Authorization: 'key ' + process.env.TTN_ACCESS_TOKEN,
  },
  url: process.env.TTN_STORAGE_API,
}

app.get('/data', async (req, res)=>{
  const data = await fetch(ttnBackend.url, {headers: ttnBackend.headers}).then(res=>res.json());
  res.send(data);
});

app.use(express.static('client'));

app.listen(process.env.PORT, ()=>console.log('listening on port', process.env.PORT));
