var bytesToInt = function (bytes) {
  var i = 0;
  for (var x = 0; x < bytes.length; x++) {
    i |= +(bytes[x] << (x * 8));
  }
  return i;
};

var unixtime = function (bytes) {
  if (bytes.length !== unixtime.BYTES) {
    throw new Error('Unix time must have exactly 4 bytes');
  }
  return bytesToInt(bytes);
};
unixtime.BYTES = 4;

var uint8 = function (bytes) {
  if (bytes.length !== uint8.BYTES) {
    throw new Error('uint8 must have exactly 1 byte');
  }
  return bytesToInt(bytes);
};
uint8.BYTES = 1;

var uint16 = function (bytes) {
  if (bytes.length !== uint16.BYTES) {
    throw new Error('uint16 must have exactly 2 bytes');
  }
  return bytesToInt(bytes);
};
uint16.BYTES = 2;

var uint32 = function (bytes) {
  if (bytes.length !== uint32.BYTES) {
    throw new Error('uint32 must have exactly 4 bytes');
  }
  return bytesToInt(bytes);
};
uint32.BYTES = 4;

var latLng = function (bytes) {
  if (bytes.length !== latLng.BYTES) {
    throw new Error('Lat/Long must have exactly 8 bytes');
  }

  var lat = bytesToInt(bytes.slice(0, latLng.BYTES / 2));
  var lng = bytesToInt(bytes.slice(latLng.BYTES / 2, latLng.BYTES));

  return [lat / 1e6, lng / 1e6];
};
latLng.BYTES = 8;

var temperature = function (bytes) {
  if (bytes.length !== temperature.BYTES) {
    throw new Error('Temperature must have exactly 2 bytes');
  }
  var isNegative = bytes[0] & 0x80;
  var b = ('00000000' + Number(bytes[0]).toString(2)).slice(-8)
    + ('00000000' + Number(bytes[1]).toString(2)).slice(-8);
  if (isNegative) {
    var arr = b.split('').map(function (x) { return !Number(x); });
    for (var i = arr.length - 1; i > 0; i--) {
      arr[i] = !arr[i];
      if (arr[i]) {
        break;
      }
    }
    b = arr.map(Number).join('');
  }
  var t = parseInt(b, 2);
  if (isNegative) {
    t = -t;
  }
  return t / 1e2;
};
temperature.BYTES = 2;

var humidity = function (bytes) {
  if (bytes.length !== humidity.BYTES) {
    throw new Error('Humidity must have exactly 2 bytes');
  }

  var h = bytesToInt(bytes);
  return h / 1e2;
};
humidity.BYTES = 2;

// Based on https://stackoverflow.com/a/37471538 by Ilya Bursov
// quoted by Arjan here https://www.thethingsnetwork.org/forum/t/decode-float-sent-by-lopy-as-node/8757
function rawfloat(bytes) {
  if (bytes.length !== rawfloat.BYTES) {
    throw new Error('Float must have exactly 4 bytes');
  }
  // JavaScript bitwise operators yield a 32 bits integer, not a float.
  // Assume LSB (least significant byte first).
  var bits = bytes[3] << 24 | bytes[2] << 16 | bytes[1] << 8 | bytes[0];
  var sign = (bits >>> 31 === 0) ? 1.0 : -1.0;
  var e = bits >>> 23 & 0xff;
  var m = (e === 0) ? (bits & 0x7fffff) << 1 : (bits & 0x7fffff) | 0x800000;
  var f = sign * m * Math.pow(2, e - 150);
  return f;
}
rawfloat.BYTES = 4;

var bitmap = function (byte) {
  if (byte.length !== bitmap.BYTES) {
    throw new Error('Bitmap must have exactly 1 byte');
  }
  var i = bytesToInt(byte);
  var bm = ('00000000' + Number(i).toString(2)).substr(-8).split('').map(Number).map(Boolean);
  return ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']
    .reduce(function (obj, pos, index) {
      obj[pos] = bm[index];
      return obj;
    }, {});
};
bitmap.BYTES = 1;

const ValueOrder = {
  NOT: -1,
  VOLT: 0,
  TEMP: 1,
  HUMIDITY: 2,
  PRESSURE: 3,
  DISTANCE: 4,
  TDSv: 5,
  MOIS: 6,
  LUX: 7,
  AMBIENT: 8,
  H2v: 9,
  COv: 10,
  CO2v: 11,
  NO2v: 12,
  NH3v: 13,
  C4H10v: 14,
  C3H8v: 15,
  CH4v: 16,
  C2H5OHv: 17,
  ALTITUDE: 18,
  MV: 19,
  MGL: 20,
  MSCM: 21,
  PH: 22,
  DBA: 23,
  DEPTH: 24,
  UV_I: 25,
  RGB: 26,
  ANGLE: 27,
  KOHM: 28
};


decoders = {
  temp: temperature,
  humidity: humidity,
  rawfloat: rawfloat,
  uint16: uint16
};

const valueOrders = [
  { data_name: "Battery", encoding: "temp" },     // VOLT
  { data_name: "temp",    encoding: "temp" },     // TEMP
  { data_name: "hum",     encoding: "humidity" }, // HUMIDITY
  { data_name: "press",   encoding: "rawfloat" }, // PRESSURE
  { data_name: "height",  encoding: "temp" },     // DISTANCE (C++ nutzt addTemperature!)
  { data_name: "TDS",     encoding: "uint16" },   // TDSv
  { data_name: "MOIS",    encoding: "uint16" },   // MOIS
  { data_name: "LUX",     encoding: "uint16" },     // LUX (geändert!)
  { data_name: "Ambient", encoding: "uint16" },   // AMBIENT
  { data_name: "H2",      encoding: "uint16" },   // H2v
  { data_name: "CO",      encoding: "uint16" },   // COv
  { data_name: "CO2",     encoding: "uint16" },   // CO2v
  { data_name: "NO2",     encoding: "uint16" },   // NO2v
  { data_name: "NH3",     encoding: "uint16" },   // NH3v
  { data_name: "C4H10",   encoding: "uint16" },   // C4H10v
  { data_name: "C3H8",    encoding: "uint16" },   // C3H8v
  { data_name: "CH4",     encoding: "uint16" },   // CH4v
  { data_name: "C2H5OH",  encoding: "uint16" },   // C2H5OHv
  { data_name: "alt",     encoding: "uint16" },   // ALTITUDE
  { data_name: "OPR",     encoding: "rawfloat" }, // MV
  { data_name: "DO",      encoding: "temp" },     // MGL
  { data_name: "EC",      encoding: "temp" },     // MSCM
  { data_name: "PH",      encoding: "temp" },     // PH
  { data_name: "SOUND",   encoding: "temp" },     // DBA
  { data_name: "DEPTH",   encoding: "rawfloat" }, // DEPTH
  { data_name: "UV_I",    encoding: "temp" },     // UV_I
  { data_name: "RGB",     encoding: "uint16" },   // RGB
  { data_name: "ANGLE",   encoding: "temp" },     // ANGLE
  { data_name: "KOHM",    encoding: "rawfloat" }      // KOHM (geändert!)
];

function decodeUplink(input) {
  let i = 0;
  let b = input.bytes;
  let output = {};
  let count = {};
  while (b.length > 0) {
    // read value order number
    let n = uint8.BYTES;
    let rv = b.slice(0, n);
    b = b.slice(n);
    let von = uint8(rv);

    // read value
    let vo = valueOrders[von];
    n = decoders[vo.encoding].BYTES;
    rv = b.slice(0, n);
    b = b.slice(n);
    let v = decoders[vo.encoding](rv);

    // add to output data
    if (vo.data_name in count) {
      output[vo.data_name + count[vo.data_name]] = v;
      count[vo.data_name] += 1;
    } else {
      output[vo.data_name] = v;
      count[vo.data_name] = 1;
    }
  }
  return {
    data: output,
    warnings: [],
    errors: []
  };
}

