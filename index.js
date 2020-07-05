import express from 'express';
import bodyParser from 'body-parser';
import mongodb from 'mongodb';
import fetch from 'node-fetch';
import dotenv from 'dotenv';
dotenv.config();

const uri = `mongodb+srv://${process.env.MONGODB_USER}:${process.env.MONGODB_PW}@${process.env.MONGODB_ADDRESS}/gps?retryWrites=true&w=majority`;
const client = new mongodb.MongoClient(uri, { useNewUrlParser: true });
client.connect();

const app = express();

const ttnBackend = {
  headers: {
    Accept: 'application/json',
    Authorization: 'key ' + process.env.TTN_ACCESS_TOKEN,
  },
  url: process.env.TTN_STORAGE_API,
}

app.use(bodyParser.raw({type: '*/*'}));

app.get('/data', async (req, res)=>{
  const data1 = await fetch(ttnBackend.url, {headers: ttnBackend.headers}).then(res=>res.status===204?[]:res.json());

  await client.connect();
  const collection = client.db("gps").collection("upload");
  const data2 = await collection.find({}).toArray();

  res.send([...data1, ...data2]);
});

app.all('/upload', async (req, res)=>{
  console.log(req, res);
  if (req.body instanceof Buffer) {
    await client.connect();
    const collection = client.db("gps").collection("upload");
    await collection.insertOne({
      raw: req.body.toString('base64'),
      time: new Date(),
      device_id: req.query.device_id,
    });
  }
  console.log('got gps upload');

  res.send("OK");
});

app.use(express.static('client'));

app.listen(process.env.PORT, ()=>console.log('listening on port', process.env.PORT));
