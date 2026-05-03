function decodeUplink(input) {
  let b = input.bytes || [];
  let output = {};
  let count = {};
  let warnings = [];

  function safeSlice(arr, n) {
    if (arr.length < n) {
      warnings.push("Payload too short");
      return null;
    }
    return [arr.slice(0, n), arr.slice(n)];
  }

  function bytesToInt(bytes) {
    var i = 0;
    for (var x = 0; x < bytes.length; x++) {
      i |= (bytes[x] << (x * 8));
    }
    return i;
  }

  function temperature(bytes) {
    if (!bytes || bytes.length !== 2) return null;

    var isNegative = bytes[0] & 0x80;

    var b =
      ('00000000' + Number(bytes[0]).toString(2)).slice(-8) +
      ('00000000' + Number(bytes[1]).toString(2)).slice(-8);

    if (isNegative) {
      var arr = b.split('').map(function (x) { return !Number(x); });
      for (var i = arr.length - 1; i > 0; i--) {
        arr[i] = !arr[i];
        if (arr[i]) break;
      }
      b = arr.map(Number).join('');
    }

    var t = parseInt(b, 2);
    if (isNegative) t = -t;

    return t / 1e2;
  }

  function humidity(bytes) {
    if (!bytes || bytes.length !== 2) return null;
    return bytesToInt(bytes) / 1e2;
  }

  function uint16(bytes) {
    if (!bytes || bytes.length !== 2) return null;
    return bytesToInt(bytes);
  }

  function rawfloat(bytes) {
    if (!bytes || bytes.length !== 4) return null;

    var bits =
      (bytes[3] << 24) |
      (bytes[2] << 16) |
      (bytes[1] << 8) |
      bytes[0];

    var sign = (bits >>> 31 === 0) ? 1.0 : -1.0;
    var e = (bits >>> 23) & 0xff;
    var m = (e === 0)
      ? (bits & 0x7fffff) << 1
      : (bits & 0x7fffff) | 0x800000;

    return sign * m * Math.pow(2, e - 150);
  }

  // Round value to appropriate decimal places based on measurement type
  function roundToMeaningful(value, dataName) {
    // Define decimal places for each measurement type
    const decimalPlaces = {
      // Temperature and humidity - 2 decimals
      'temp': 2, 'hum': 2, 'Battery': 2,
      // Pressure - 1 decimal
      'press': 1,
      // Distance/altitude - 1 decimal
      'height': 1, 'alt': 1, 'DEPTH': 1,
      // Electrical measurements - appropriate decimals
      'KOHM': 2, 'VOLT': 3,
      // pH - 2 decimals
      'PH': 2,
      // Sound - 1 decimal
      'SOUND': 1, 'DBA': 1,
      // UV - 2 decimals
      'UV_I': 2,
      // Angle - 1 decimal
      'ANGLE': 1, 'DIRECTION': 1,
      // Speed - 1 decimal
      'SPEED': 1, 'wind_spd': 1,
      // Gas concentrations - 1 decimal
      'H2': 1, 'CO': 1, 'CO2': 1, 'NO2': 1, 'NH3': 1,
      'C4H10': 1, 'C3H8': 1, 'CH4': 1, 'C2H5OH': 1,
      // Other measurements - 2 decimals default
      'OPR': 2, 'DO': 2, 'EC': 2, 'MV': 2, 'MGL': 2, 'MSCM': 2,
      // Integer values - 0 decimals (already integers from uint16)
      'TDS': 0, 'MOIS': 0, 'LUX': 0, 'Ambient': 0, 'RGB': 0,
      'red': 0, 'green': 0, 'blue': 0, 'clear': 0, 'BPM': 0
    };

    const decimals = decimalPlaces[dataName] !== undefined ? decimalPlaces[dataName] : 2;
    const factor = Math.pow(10, decimals);
    return Math.round(value * factor) / factor;
  }

  const decoders = {
    temp: { fn: temperature, size: 2 },
    humidity: { fn: humidity, size: 2 },
    uint16: { fn: uint16, size: 2 },
    rawfloat: { fn: rawfloat, size: 4 }
  };

  const valueOrders = [
    { data_name: "Battery", encoding: "temp" },
    { data_name: "temp", encoding: "temp" },
    { data_name: "hum", encoding: "humidity" },
    { data_name: "press", encoding: "rawfloat" },
    { data_name: "height", encoding: "temp" },
    { data_name: "TDS", encoding: "uint16" },
    { data_name: "MOIS", encoding: "uint16" },
    { data_name: "LUX", encoding: "uint16" },
    { data_name: "Ambient", encoding: "uint16" },
    { data_name: "H2", encoding: "uint16" },
    { data_name: "CO", encoding: "uint16" },
    { data_name: "CO2", encoding: "uint16" },
    { data_name: "NO2", encoding: "uint16" },
    { data_name: "NH3", encoding: "uint16" },
    { data_name: "C4H10", encoding: "uint16" },
    { data_name: "C3H8", encoding: "uint16" },
    { data_name: "CH4", encoding: "uint16" },
    { data_name: "C2H5OH", encoding: "uint16" },
    { data_name: "alt", encoding: "uint16" },
    { data_name: "OPR", encoding: "rawfloat" },
    { data_name: "DO", encoding: "temp" },
    { data_name: "EC", encoding: "temp" },
    { data_name: "PH", encoding: "temp" },
    { data_name: "SOUND", encoding: "temp" },
    { data_name: "DEPTH", encoding: "rawfloat" },
    { data_name: "UV_I", encoding: "temp" },
    { data_name: "RGB", encoding: "uint16" },
    { data_name: "ANGLE", encoding: "temp" },
    { data_name: "KOHM", encoding: "rawfloat" },
    { data_name: "red", encoding: "uint16" },
    { data_name: "green", encoding: "uint16" },
    { data_name: "blue", encoding: "uint16" },
    { data_name: "clear", encoding: "uint16" },
    { data_name: "wind_dir", encoding: "uint16" },
    { data_name: "wind_spd", encoding: "rawfloat" },
    { data_name: "BPM", encoding: "uint16" }
  ];

  while (b.length > 0) {
    let res = safeSlice(b, 1);
    if (!res) break;

    let idBytes = res[0];
    b = res[1];

    let von = idBytes[0];
    let vo = valueOrders[von];

    if (!vo) {
      warnings.push("Unknown value order: " + von);
      break;
    }

    let decoder = decoders[vo.encoding];
    res = safeSlice(b, decoder.size);
    if (!res) break;

    let dataBytes = res[0];
    b = res[1];

    let v = decoder.fn(dataBytes);
    if (v === null) continue;

    // Round value to meaningful decimal places
    v = roundToMeaningful(v, vo.data_name);

    if (vo.data_name in count) {
      output[vo.data_name + count[vo.data_name]] = v;
      count[vo.data_name]++;
    } else {
      output[vo.data_name] = v;
      count[vo.data_name] = 1;
    }
  }

  return {
    data: output,
    warnings: warnings,
    errors: []
  };
}