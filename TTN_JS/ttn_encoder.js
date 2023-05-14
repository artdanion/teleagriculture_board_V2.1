var bytesToInt = function(bytes) {
    var i = 0;
    for (var x = 0; x < bytes.length; x++) {
      i |= +(bytes[x] << (x * 8));
    }
    return i;
  };
  
  var unixtime = function(bytes) {
    if (bytes.length !== unixtime.BYTES) {
      throw new Error('Unix time must have exactly 4 bytes');
    }
    return bytesToInt(bytes);
  };
  unixtime.BYTES = 4;
  
  var uint8 = function(bytes) {
    if (bytes.length !== uint8.BYTES) {
      throw new Error('uint8 must have exactly 1 byte');
    }
    return bytesToInt(bytes);
  };
  uint8.BYTES = 1;
  
  var uint16 = function(bytes) {
    if (bytes.length !== uint16.BYTES) {
      throw new Error('uint16 must have exactly 2 bytes');
    }
    return bytesToInt(bytes);
  };
  uint16.BYTES = 2;
  
  var uint32 = function(bytes) {
    if (bytes.length !== uint32.BYTES) {
      throw new Error('uint32 must have exactly 4 bytes');
    }
    return bytesToInt(bytes);
  };
  uint32.BYTES = 4;
  
  var latLng = function(bytes) {
    if (bytes.length !== latLng.BYTES) {
      throw new Error('Lat/Long must have exactly 8 bytes');
    }
  
    var lat = bytesToInt(bytes.slice(0, latLng.BYTES / 2));
    var lng = bytesToInt(bytes.slice(latLng.BYTES / 2, latLng.BYTES));
  
    return [lat / 1e6, lng / 1e6];
  };
  latLng.BYTES = 8;
  
  var temperature = function(bytes) {
    if (bytes.length !== temperature.BYTES) {
      throw new Error('Temperature must have exactly 2 bytes');
    }
    var isNegative = bytes[0] & 0x80;
    var b = ('00000000' + Number(bytes[0]).toString(2)).slice(-8)
          + ('00000000' + Number(bytes[1]).toString(2)).slice(-8);
    if (isNegative) {
      var arr = b.split('').map(function(x) { return !Number(x); });
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
  
  var humidity = function(bytes) {
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
    var bits = bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0];
    var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
    var e = bits>>>23 & 0xff;
    var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
    var f = sign * m * Math.pow(2, e - 150);
    return f;
  }
  rawfloat.BYTES = 4;
  
  var bitmap = function(byte) {
    if (byte.length !== bitmap.BYTES) {
      throw new Error('Bitmap must have exactly 1 byte');
    }
    var i = bytesToInt(byte);
    var bm = ('00000000' + Number(i).toString(2)).substr(-8).split('').map(Number).map(Boolean);
    return ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']
      .reduce(function(obj, pos, index) {
        obj[pos] = bm[index];
        return obj;
      }, {});
  };
  bitmap.BYTES = 1;
  
  /*
  enum ValueOrder
  {
    NOT = -1,
    VOLT,     // temp encoding
    TEMP,     // temp encoding
    HUMIDITY, // humidity encoding
    PRESSURE, // rawfloat encoding
    DISTANCE, // rawfloat encoding
    TDSv,     // uint16 encoding
    MOIS,     // uint16 encoding
    LUX,      // uint16 encoding
    AMBIENT,  // uint16 encoding
    H2v,      // uint16 encoding
    COv,      // uint16 encoding
    CO2v,     // uint16 encoding
    NO2v,     // uint16 encoding
    NH3v,     // uint16 encoding
    C4H10v,   // uint16 encoding
    C3H8v,    // uint16 encoding
    CH4v,     // uint16 encoding
    C2H5OHv,  // uint16 encoding
    ALTITUDE, // uint16 encoding
    RGB,
    ANGLE
  };
  */
  
  decoders = {
    temp: temperature,
    humidity: humidity,
    rawfloat: rawfloat,
    uint16: uint16
  };
  
  valueOrders = [];
  valueOrders.push({ // VOLT
    data_name: "Battery",
    encoding: "temp"
  });
  valueOrders.push({ // TEMP
    data_name: "temp",
    encoding: "temp"
  });
  valueOrders.push({ // HUMIDITY
    data_name: "hum",
    encoding: "humidity"
  });
  valueOrders.push({ // PRESSURE
    data_name: "press",
    encoding: "rawfloat"
  });
  valueOrders.push({ // DISTANCE
    data_name: "height",
    encoding: "rawfloat"
  });
  valueOrders.push({ // TDSv
    data_name: "TDS",
    encoding: "uint16"
  });
  valueOrders.push({ // MOIS
    data_name: "MOIS",
    encoding: "uint16"
  });
  valueOrders.push({ // LUX
    data_name: "LUX",
    encoding: "uint16"
  });
  valueOrders.push({ // AMBIENT
    data_name: "Ambient",
    encoding: "uint16"
  });
  valueOrders.push({ // H2v
    data_name: "H2",
    encoding: "uint16"
  });
  valueOrders.push({ // COv
    data_name: "CO",
    encoding: "uint16"
  });
  valueOrders.push({ // CO2v
    data_name: "CO2",
    encoding: "uint16"
  });
  valueOrders.push({ // NO2v
    data_name: "NO2",
    encoding: "uint16"
  });
  valueOrders.push({ // NH3v
    data_name: "NH3",
    encoding: "uint16"
  });
  valueOrders.push({ // C4H10v
    data_name: "C4H10",
    encoding: "uint16"
  });
  valueOrders.push({ // C3H8v
    data_name: "C3H8",
    encoding: "uint16"
  });
  valueOrders.push({ // CH4v
    data_name: "CH4",
    encoding: "uint16"
  });
  valueOrders.push({ // C2H5OHv
    data_name: "C2H5OH",
    encoding: "uint16"
  });
  valueOrders.push({ // ALTITUDE
    data_name: "alt",
    encoding: "uint16"
  });
  
  function decodeUplink(input) {
    i = 0;
    b = input.bytes;
    output = {};
    count = {};
    while (b.length > 0) {
      // read value order number
      n = uint8.BYTES;
      rv = b.slice(0,n);
      b = b.slice(n);
      von = uint8(rv);
      
      // read value
      vo = valueOrders[von];
      n = decoders[vo.encoding].BYTES;
      rv = b.slice(0,n);
      b = b.slice(n);
      v = decoders[vo.encoding](rv);
      
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
  