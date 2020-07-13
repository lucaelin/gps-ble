
import mongodb from 'mongodb';
import dotenv from 'dotenv';
dotenv.config();

const uri = `mongodb+srv://${process.env.MONGODB_USER}:${process.env.MONGODB_PW}@${process.env.MONGODB_ADDRESS}/gps?retryWrites=true&w=majority`;
const client = new mongodb.MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });
const connection = client.connect();

export async function insertLocations(locations) {
  await connection;
  if (!(locations instanceof Array)) locations = [locations];
  const collection = client.db("gps").collection("locations");
  return await collection.insertMany(locations, { upsert: true, ordered: false }).catch(e=>e.result);
}

export async function insertUpload(upload) {
  await connection;
  const collection = client.db("gps").collection("uploads");
  return await collection.insertOne(upload);
}

export async function query(q = {}) {
  await connection;
  const collection = client.db("gps").collection("locations");
  const data = await collection.find(q).toArray();

  return data;
}
