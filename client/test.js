const {StructuredDataView, ArrayOfStructuredDataViews} = structuredDataView;

import {plotPath, plotPosition, dom} from './map.js';
import {StatusData, GpsData, GpsStatus, Y2KtoDate} from './GpsData.js';

function distanceKm({lat: lat1, lng: lng1}, {lat: lat2, lng: lng2}) {
  const R = 6371e3; // metres
  const φ1 = lat1 * Math.PI/180; // φ, λ in radians
  const φ2 = lat2 * Math.PI/180;
  const Δφ = (lat2-lat1) * Math.PI/180;
  const Δλ = (lng2-lng1) * Math.PI/180;

  const a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
          Math.cos(φ1) * Math.cos(φ2) *
          Math.sin(Δλ/2) * Math.sin(Δλ/2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

  const d = R * c; // in metres
  return d / 1000;
}

function bearing({lat: lat1, lng: lng1}, {lat: lat2, lng: lng2}) {
  const φ1 = lat1 * Math.PI/180; // φ, λ in radians
  const φ2 = lat2 * Math.PI/180;
  const λ1 = lng1 * Math.PI/180;
  const λ2 = lng2 * Math.PI/180;

  const y = Math.sin(λ2-λ1) * Math.cos(φ2);
  const x = Math.cos(φ1)*Math.sin(φ2) -
            Math.sin(φ1)*Math.cos(φ2)*Math.cos(λ2-λ1);
  const θ = Math.atan2(y, x);
  const brng = (θ*180/Math.PI + 360) % 360; // in degrees
  return brng;
}

function diffAngle(a, b, c, d) {
  const bearing_ab = 180.0 - bearing(a, b);
  const bearing_cd = 180.0 - bearing(c, d);
  const bearing_diff = bearing_ab - bearing_cd;
  const bearing_mod = bearing_diff+(bearing_diff/Math.abs(bearing_diff))*(-360.0);
  const bearing_change = bearing_diff > 180.0 || bearing_diff < -180.0 ? bearing_mod : bearing_diff;
  return bearing_change;

}

function calcDeviation(a, b, c) {
  const bearing_change = diffAngle(a, b, a, c);
  const distance_ac = distanceKm(a, b) * 1000;
  const offset = Math.sin(bearing_change * Math.PI/180) * distance_ac;
  return offset;
}

async function runTest() {
  const b = await fetch('./test/7475.BIN').then(t=>t.arrayBuffer());
  const data = new ArrayOfStructuredDataViews(b, GpsData);

  const data2 = data
    .filter(l=>l.status != GpsStatus.NOT_VALID)
    .sort((a,b)=>a.time-b.time);
  console.log(data2);

  const lengthbefore = data2.length;

  const hist = [];
  let stored = data2[0];
  let previous = data2[1];
  let current = data2[2];
  let lineDeviation = 0;

  for (const l of data2.slice(3,-1)) {
    previous = current;
    current = l;

    const directionChange = diffAngle(stored, previous, previous, current);
    if (current.time - previous.time > 30 || Math.abs(directionChange) > 20) {
      hist.push(previous);
      stored = previous;
      continue;
    }

    const newDeviation = calcDeviation(stored, previous, current);
    lineDeviation += newDeviation;
    console.log('deviation:', lineDeviation);

    if (!stored.status || Math.abs(lineDeviation) > 3) {
      // save to flash
      hist.push(previous);
      stored = previous;
      lineDeviation = newDeviation;

      console.log("Wrote location to history");
    } else {
      console.log("Skipping location");
    }
  }

  console.log('plotting', hist.length, 'instead of', lengthbefore);

  plotPath(hist);
}

runTest();
